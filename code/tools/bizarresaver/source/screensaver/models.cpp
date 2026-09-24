
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "screensaver/models.h"
#include "screensaver/camera.h"

// -----------------------------------------------------------------------------------
// model loader
// -----------------------------------------------------------------------------------

CModelInfo::CModelInfo()
{
	m_pszName[0]		= 0;
	m_pMaterials		= NULL;
	m_pAdjacencyBuffer  = NULL;
	m_pMesh				= NULL;
	m_nMaterials		= NULL;
	m_rgpfxAttributes	= NULL;
	m_pMeshTextures		= NULL;
	m_rgEffectInfo		= NULL;
}


int	tgtfvf=D3DFVF_XYZ |	D3DFVF_NORMAL |	D3DFVF_TEX1;

bool CModelInfo::Load()
{
	// .X file model loading code
	LPD3DXMESH				pMesh, pTempMesh;
	D3DXWELDEPSILONS		Epsilons;
	LPD3DXBUFFER			pMtrlBuffer;
	D3DXMATERIAL			*d3dxMaterials;
	DWORD					dw32BitFlag;
	LPD3DXBUFFER			pbufEffectInstances;
	CD3DXCrackDecl1			cd;
    D3DVERTEXELEMENT9		pDecl[MAX_FVF_DECL_SIZE];
	LPD3DXEFFECTINSTANCE	rgEffects = NULL;
	HRESULT					hr;

	if(	FAILED(	D3DXLoadMeshFromX( m_pszName, D3DXMESH_MANAGED, gpD3DDevice,
		&m_pAdjacencyBuffer, &pMtrlBuffer, &pbufEffectInstances, 
		(DWORD*)&m_nMaterials, &pMesh ) ) )
	{
		// error
		return false;
	}

	// Allocate	material/texture arrays
	d3dxMaterials	= (D3DXMATERIAL*)pMtrlBuffer->GetBufferPointer();
	m_pMaterials	= new D3DMATERIAL9[m_nMaterials];
	m_pMeshTextures	= new LPDIRECT3DTEXTURE9[m_nMaterials];

	// Copy	the	materials and load the textures
	for( int i=0; i<m_nMaterials; i++ )
	{
		m_pMaterials[i] = d3dxMaterials[i].MatD3D;
		m_pMaterials[i].Ambient = m_pMaterials[i].Diffuse;

		return D3DXCreateTextureFromFileEx( gpD3DDevice, m_pszName, 
			D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
			D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &m_pTexture ) == S_OK;

		ASSETHANDLE h = g_AssetManager2->GetImageFromName(d3dxMaterials[i].pTextureFilename);

		if (h != ASSETHANDLE_INVALID)
			m_pMeshTextures[i] = ((CImageInfo*)g_AssetManager2->Asset(h))->m_pTexture;
		else
			m_pMeshTextures[i] = NULL;
	}
	pMtrlBuffer->Release();

	// get fx info from the .x file
	if (pbufEffectInstances)
		rgEffects = (LPD3DXEFFECTINSTANCE)pbufEffectInstances->GetBufferPointer();

	LPD3DXEFFECTINSTANCE pOldFXI = NULL;

	m_rgpfxAttributes = new LPD3DXEFFECT[m_nMaterials];
	m_rgEffectInfo	  = new SEffectInfo[m_nMaterials];

    if (rgEffects != NULL)
	{
		// load the effects reference by the model
		for (int iMaterial=0; iMaterial<m_nMaterials; iMaterial++)
		{
			m_rgpfxAttributes[iMaterial] = NULL;

			// if we don't have any default, copy the previous defaults - HACK to fix DX9 bug
			if (rgEffects[iMaterial].pEffectFilename)
			{
				if (rgEffects[iMaterial].NumDefaults == 0 && pOldFXI)
				{
					rgEffects[iMaterial].NumDefaults = pOldFXI->NumDefaults;
					rgEffects[iMaterial].pDefaults   = pOldFXI->pDefaults;					
				}
				else
					pOldFXI = rgEffects+iMaterial;
			}

			D3DXCreateEffectInstance(&rgEffects[iMaterial], gpD3DDevice, &m_rgpfxAttributes[iMaterial], NULL);

			GenerateEffectInfo(m_rgpfxAttributes[iMaterial], &m_rgEffectInfo[iMaterial]);
			m_rgEffectInfo[iMaterial].pNormalMapTex = gpNormalMapTex;
			for (int iut = 0; iut < MAX_USER_TEXTURES; iut++)
				m_rgEffectInfo[iMaterial].hUserTexture[iut] = ghUserTexture[iut];
		}
	}

	SAFE_RELEASE(pbufEffectInstances);

	pMesh = CalculateTangentSpace(pMesh);

	m_pMesh = pMesh;

	return true;
}

// free one model
void CModelInfo::Free()
{
	SAFE_RELEASE( m_pAdjacencyBuffer );
	SAFE_RELEASE( m_pMesh );

	for (int i=0; i<m_nMaterials; i++)
	{
		if (m_rgpfxAttributes && m_rgpfxAttributes[i])	SAFE_RELEASE(m_rgpfxAttributes[i]);
//		if (m_pMeshTextures && m_pMeshTextures[i])		SAFE_RELEASE(m_pMeshTextures[i]); // ALREADY FREED in CImageInfo
	}

	if (m_rgpfxAttributes)	delete [] m_rgpfxAttributes;		m_rgpfxAttributes = NULL;
	if (m_pMaterials)		delete [] m_pMaterials;				m_pMaterials = NULL;
	if (m_pMeshTextures)	delete [] m_pMeshTextures;			m_pMeshTextures = NULL;
	if (m_rgEffectInfo)		delete [] m_rgEffectInfo;			m_rgEffectInfo = NULL;

	m_nMaterials = 0;
}


// draw a model
void CModelInfo::Draw(CModelDrawInfo *pMDI)
{
	// TODO: set more renderstates etc.
	g_Camera->SetFixedPipelineMatrices();

	// loop through the materials and draw them
	for (int iMaterial = 0; iMaterial<m_nMaterials; iMaterial++)
	{
		int nPasses;

		LPD3DXEFFECT pFX;
		SEffectInfo *pFXI;
		
		if (pMDI && pMDI->m_pEffect)
		{
			pFX = pMDI->m_pEffect->m_rgpfxAttributes;
			pFXI = &pMDI->m_pEffect->m_rgEffectInfo;
		}
		else
		{
			pFX	= m_rgpfxAttributes[iMaterial];
			pFXI = &m_rgEffectInfo[iMaterial];
		}

		if (!pMDI || !pMDI->m_bSetLightmapTextures)
		{
			gpD3DDevice->SetMaterial( &m_pMaterials[iMaterial] );
			gpD3DDevice->SetTexture( 0, m_pMeshTextures[iMaterial]);
		}
		else
		{
			// manually set the lightmap textures for the lightmap shader
			gpD3DDevice->SetTexture( 0, m_rgEffectInfo[iMaterial].pNormalMapTex);
				// TODO: --- program something for when there's no normal map
			gpD3DDevice->SetTexture( 1, m_pLightmapTextures[0][0]);
			gpD3DDevice->SetTexture( 2, m_pLightmapTextures[1][0]);
			gpD3DDevice->SetTexture( 3, m_pLightmapTextures[2][0]);

			gpD3DDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			gpD3DDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			gpD3DDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
			gpD3DDevice->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			gpD3DDevice->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			gpD3DDevice->SetSamplerState( 1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
			gpD3DDevice->SetSamplerState( 2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			gpD3DDevice->SetSamplerState( 2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			gpD3DDevice->SetSamplerState( 2, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
			gpD3DDevice->SetSamplerState( 3, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			gpD3DDevice->SetSamplerState( 3, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			gpD3DDevice->SetSamplerState( 3, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
		}

		// skip bits of geometry that have no material assigned
		if (pMDI)
		{
			if (pFX)
			{
				pFX->SetTechnique(CEffect::m_pszPassNames[pMDI->m_iPass]);

				g_Camera->SetEffectMatrices(pFX, pFXI);
				if (pMDI->m_iPass == RENDERPASS_LIGHTING)
				{
					// set light parameters from light entity to shader
					if (pFXI->iLightPos)
						pFX->SetVector(pFXI->iLightPos, (D3DXVECTOR4*)&g_pCurrentLight->m_vPos);
					if (pFXI->iInvLightScale)
					{
						float f = 1.0f/g_pCurrentLight->m_fRadius;
						pFX->SetVector(pFXI->iInvLightScale, (D3DXVECTOR4*)&f);
					}
					if (pFXI->iLightColour)
						pFX->SetVector(pFXI->iLightColour, (D3DXVECTOR4*)&g_pCurrentLight->m_vColour);
				}

				if ( pMDI->pTextureStage0 && pFXI->hUserTexture[0])
					pFX->SetTexture( pFXI->hUserTexture[0], pMDI->pTextureStage0);

				pFX->Begin((UINT*)&nPasses,0);

				for (int iPass=0; iPass<nPasses; iPass++)
				{
					pFX->Pass(iPass);

					m_pMesh->DrawSubset(iMaterial);
				}

				pFX->End();
				continue;
			}
		}
		m_pMesh->DrawSubset(iMaterial);
	}
}



// ---------------------------------

const char *CEffect::m_pszPassNames[2]=
{
	"Material",
	"Lighting",
};

CEffect::CEffect(char *pszName)
{
	D3DXEFFECTINSTANCE inst;

	inst.pEffectFilename = pszName;
	inst.NumDefaults = 0;
	inst.NumDefaults = NULL;

	m_rgpfxAttributes = NULL;

	D3DXCreateEffectInstance(&inst, gpD3DDevice, &m_rgpfxAttributes, NULL);

	GenerateEffectInfo(m_rgpfxAttributes, &m_rgEffectInfo);
}

CEffect::~CEffect()
{
	SAFE_RELEASE(m_rgpfxAttributes);
}


void CEffect::SetPass(int iPassType)
{
	m_rgpfxAttributes->SetTechnique(m_pszPassNames[iPassType]);
}


/*
 * Code for adding tangent space vectors to a model
 */
bool CModelInfo::CheckNeedsTangents()
{
    D3DXTECHNIQUE_DESC TechniqueDesc;
    D3DXPASS_DESC PassDesc;
    D3DXHANDLE hPass;
    D3DXHANDLE hTechnique;
	int iMaterial;

	for (iMaterial = 0; iMaterial < m_nMaterials; iMaterial++)
	{
		if (!m_rgpfxAttributes[iMaterial]) continue;

		hTechnique = m_rgpfxAttributes[iMaterial]->GetCurrentTechnique();
		m_rgpfxAttributes[iMaterial]->GetTechniqueDesc(hTechnique, &TechniqueDesc);
		for( UINT iPass = 0; iPass < TechniqueDesc.Passes; iPass++ )
		{
			hPass = m_rgpfxAttributes[iMaterial]->GetPass( hTechnique, iPass );
			m_rgpfxAttributes[iMaterial]->GetPassDesc( hPass, &PassDesc );

			UINT NumVSSemanticsUsed;
			D3DXSEMANTIC pVSSemantics[MAXD3DDECLLENGTH];

			if( !PassDesc.pVertexShaderFunction ||
				FAILED( D3DXGetShaderInputSemantics( PassDesc.pVertexShaderFunction, pVSSemantics, &NumVSSemanticsUsed ) ) )
			{
				continue;
			}

			for( UINT iSem = 0; iSem < NumVSSemanticsUsed; iSem++ )
			{
				if( pVSSemantics[iSem].Usage == D3DDECLUSAGE_TANGENT )
				{
					return true;
				}
			}
		}
	}
	return false;
}

// calculating tangent/binormal vectors = ARSEACHE
LPD3DXMESH CModelInfo::CalculateTangentSpace(LPD3DXMESH	pMesh)
{
	CD3DXCrackDecl1			cd;
    D3DVERTEXELEMENT9		pDecl[MAX_FVF_DECL_SIZE];
	HRESULT					hr;
	DWORD					dw32BitFlag;
	LPD3DXMESH				pTempMesh;

    // create a crackdecl with the vertex declaration of this mesh
	pMesh->GetDeclaration(pDecl);
    cd.SetDeclaration(pDecl);

	// maybe we don't need them, or maybe we already have them - if so, exit
    if (!CheckNeedsTangents() || (cd.GetSemanticElement(D3DDECLUSAGE_TANGENT, 0))) return pMesh;

    LPD3DXMESH pMeshTemp = pMesh;

    AddTangent(cd, pDecl);

    // clone in the space for the new normal
	dw32BitFlag	= (pMesh->GetOptions() & D3DXMESH_32BIT);
    hr = pMeshTemp->CloneMesh(pMeshTemp->GetOptions(), pDecl, gpD3DDevice, &pMesh);

    SAFE_RELEASE(pMeshTemp);

	// USE NVIDIA NVMESHMENDER

	// clone to compatible format
	int tgtfvf = D3DFVF_XYZ |
					D3DFVF_NORMAL |
					D3DFVF_TEX3 |
					D3DFVF_TEXCOORDSIZE2( 0 ) |
					D3DFVF_TEXCOORDSIZE3( 1 ) |
					D3DFVF_TEXCOORDSIZE3( 2 );

	hr = pMesh->CloneMeshFVF( dw32BitFlag|D3DXMESH_MANAGED,	tgtfvf,	
		gpD3DDevice, &pTempMesh );

	SAFE_RELEASE(pMesh);
	pMesh=pTempMesh;

	MeshMender mender;
	void *pVertices;
	unsigned short *pFaces;

	// get the vertex data
	// Lock	the	vertex buffer, to generate a simple	bounding sphere
	LPDIRECT3DVERTEXBUFFER9	pVertexBuffer =	NULL;
	LPDIRECT3DINDEXBUFFER9 pIndexBuffer = NULL;
	hr = pMesh->GetVertexBuffer( &pVertexBuffer	);
	hr = pVertexBuffer->Lock( 0, 0,	&pVertices,	D3DLOCK_NOSYSLOCK );

	std::vector< MeshMender::Vertex > theVerts;
	std::vector< unsigned int > theIndices;

	theVerts.resize(pMesh->GetNumVertices());
	memcpy(&theVerts[0], pVertices, sizeof(MeshMender::Vertex) * pMesh->GetNumVertices());
	pVertexBuffer->Release();

	// copy face index data
	hr = pMesh->GetIndexBuffer( &pIndexBuffer );
	hr = pIndexBuffer->Lock( 0, 0, (void**)&pFaces, D3DLOCK_NOSYSLOCK);

	if (!pFaces[1] && !pFaces[3] && !pFaces[5])
	{
		unsigned int *pFacesX = (unsigned int*)pFaces;
		for (int i=0; i<3*(int)pMesh->GetNumFaces(); i++)
			theIndices.push_back(pFacesX[i]);
	}
	else
	{
		for (int i=0; i<3*(int)pMesh->GetNumFaces(); i++)
			theIndices.push_back(pFaces[i]);
	}
	pIndexBuffer->Release();

	std::vector< unsigned int > mappingNewToOldVert;

	// MEND!
	mender.Mend(theVerts, theIndices, mappingNewToOldVert, 1,1,1,0,MeshMender::DONT_CALCULATE_NORMALS,MeshMender::DONT_RESPECT_SPLITS);

	// create new mesh with these datas
    D3DVERTEXELEMENT9 decl[] =
    {
        {0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        {0, 24, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        {0, 32, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0},
		{0, 44, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0},
        D3DDECL_END()
    };

	pTempMesh = pMesh;

	D3DXCreateMesh((DWORD)(theIndices.size()/3), (DWORD)(theVerts.size()), D3DXMESH_MANAGED, decl, gpD3DDevice, &pMesh);

	// copy in the verts
	hr = pMesh->GetVertexBuffer( &pVertexBuffer	);
	hr = pVertexBuffer->Lock( 0, 0,	&pVertices,	D3DLOCK_NOSYSLOCK );

	memcpy(pVertices, &theVerts[0], sizeof(MeshMender::Vertex) * pMesh->GetNumVertices());
	pVertexBuffer->Release();

	// copy face index data
	hr = pMesh->GetIndexBuffer( &pIndexBuffer );
	hr = pIndexBuffer->Lock( 0, 0, (void**)&pFaces, D3DLOCK_NOSYSLOCK);

	for (int i=0; i<3*(int)pMesh->GetNumFaces(); i++)
		pFaces[i]=theIndices[i];
	pIndexBuffer->Release();

	// copy attribute table
	DWORD *piAttributesOld, *piAttributes;
	hr = pMesh->LockAttributeBuffer(D3DLOCK_NOSYSLOCK, &piAttributes);
	hr = pTempMesh->LockAttributeBuffer(D3DLOCK_NOSYSLOCK, &piAttributesOld);

	memcpy(piAttributes, piAttributesOld, sizeof(DWORD)*pMesh->GetNumFaces());
	pMesh->UnlockAttributeBuffer();
	pTempMesh->UnlockAttributeBuffer();

	SAFE_RELEASE(pTempMesh);

	// DONE!
	return pMesh;
}