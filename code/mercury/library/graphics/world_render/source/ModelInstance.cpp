
#include "stdafx.h"

#include <world_render/ModelInstance.h>
#include <world_render/WorldRender.h>
#include <world_render/RenderModel.h>
#include <world_render/RenderModelData.h>
#include <world_render/RenderModelResourceUsage.h>
#include <SceneGraph/renderingdata/ModelInstanceCullNode.h>
#include <SceneGraph/renderingdata/PortalCullNode.h>
#include <system/Memory.h>
#include <renderingdata/RenderingData.h>
#include <SceneGraph/SceneGraph.h>

namespace WorldRender
{

#if PLATFORM_PC
// [Blur2PC] Render interpolation for decoupled update/render (see C_Application::IdleLoop, PC).
// The game updates at 60 Hz; the renderer may run faster. Every element world matrix written during
// an update tick is recorded with the value it had on the previous tick. Before a render, each element
// that moved on the latest tick is written as a blend of the two, and the true value is written back
// after the render, so game code never sees the blended matrix.
}	// namespace WorldRender
#include <map>
#include <windows.h>
namespace WorldRender
{
namespace
{
	struct S_InterpEntry
	{
		Maths::HMat43	prev;
		Maths::HMat43	cur;
		U32				tick;
		bool			hasPrev;
		bool			written;		// blended value written for the current render
		bool			overridden;		// set during the render by other code: do not restore
	};
	typedef std::pair<const C_ModelInstance*, S32> T_InterpKey;
	typedef std::map<T_InterpKey, S_InterpEntry> T_InterpMap;

	bool				g_InterpEnabled = false;
	bool				g_InterpInRender = false;
	U32					g_InterpTick = 1;
	F32					g_InterpRenderAlpha = 1.0f;
	T_InterpMap*		g_pInterpMap = NULL;
	CRITICAL_SECTION	g_InterpLock;

	T_InterpMap& InterpMap()
	{
		if (!g_pInterpMap)
		{
			InitializeCriticalSection(&g_InterpLock);
			g_pInterpMap = new T_InterpMap;
		}
		return *g_pInterpMap;
	}

	Maths::HMat43 BlendMatrixImpl(const Maths::HMat43& a, const Maths::HMat43& b, F32 t)
	{
		const F32 sax = (F32)a.AX().Length(), say = (F32)a.AY().Length(), saz = (F32)a.AZ().Length();
		const F32 sbx = (F32)b.AX().Length(), sby = (F32)b.AY().Length(), sbz = (F32)b.AZ().Length();
		if (sax < 1e-6f || say < 1e-6f || saz < 1e-6f || sbx < 1e-6f || sby < 1e-6f || sbz < 1e-6f)
			return b;
		Maths::HMat43 na = a, nb = b;
		na.AX(a.AX() / sax); na.AY(a.AY() / say); na.AZ(a.AZ() / saz);
		nb.AX(b.AX() / sbx); nb.AY(b.AY() / sby); nb.AZ(b.AZ() / sbz);
		const Maths::C_Quaternion qa(na.Rotation()), qb(nb.Rotation());
		const Maths::HVec3 pos = a.AW() + (b.AW() - a.AW()) * t;
		Maths::HMat43 r(qa.ShortestSlerp(qb, t), pos);
		r.AX(r.AX() * (sax + (sbx - sax) * t));
		r.AY(r.AY() * (say + (sby - say) * t));
		r.AZ(r.AZ() * (saz + (sbz - saz) * t));
		return r;
	}
}

void Blur2PCInterpEnable(bool enable)	{ g_InterpEnabled = enable; }
Maths::HMat43 Blur2PCBlendMatrix(const Maths::HMat43& a, const Maths::HMat43& b, F32 t) { return BlendMatrixImpl(a, b, t); }
U32 Blur2PCInterpCurrentTick()			{ return g_InterpTick; }
F32 Blur2PCInterpRenderAlpha()			{ return g_InterpRenderAlpha; }
void Blur2PCInterpTick()				{ ++g_InterpTick; }

void Blur2PCInterpBeginRender(F32 alpha)
{
	if (!g_InterpEnabled)
		return;
	g_InterpRenderAlpha = alpha;
	T_InterpMap& map = InterpMap();
	EnterCriticalSection(&g_InterpLock);
	for (T_InterpMap::iterator it = map.begin(); it != map.end(); ++it)
	{
		S_InterpEntry& e = it->second;
		e.written = false;
		e.overridden = false;
		if (!e.hasPrev || e.tick != g_InterpTick || alpha >= 1.0f)
			continue;
		const_cast<C_ModelInstance*>(it->first.first)->WriteElementMatrix(it->first.second, BlendMatrixImpl(e.prev, e.cur, alpha));
		e.written = true;
	}
	g_InterpInRender = true;
	LeaveCriticalSection(&g_InterpLock);
}

void Blur2PCInterpEndRender()
{
	if (!g_InterpEnabled)
		return;
	T_InterpMap& map = InterpMap();
	g_InterpRenderAlpha = 1.0f;
	EnterCriticalSection(&g_InterpLock);
	g_InterpInRender = false;
	for (T_InterpMap::iterator it = map.begin(); it != map.end(); ++it)
	{
		S_InterpEntry& e = it->second;
		if (e.written && !e.overridden)
			const_cast<C_ModelInstance*>(it->first.first)->WriteElementMatrix(it->first.second, e.cur);
		e.written = false;
	}
	LeaveCriticalSection(&g_InterpLock);
}

static void InterpRecord(const C_ModelInstance* pInstance, S32 elementIndex, const Maths::HMat43& m)
{
	T_InterpMap& map = InterpMap();
	EnterCriticalSection(&g_InterpLock);
	const T_InterpKey key(pInstance, elementIndex);
	T_InterpMap::iterator it = map.find(key);
	if (g_InterpInRender)
	{
		if (it != map.end())
			it->second.overridden = true;
	}
	else if (it == map.end())
	{
		S_InterpEntry e;
		e.prev = m; e.cur = m; e.tick = g_InterpTick; e.hasPrev = false; e.written = false; e.overridden = false;
		map.insert(std::make_pair(key, e));
	}
	else
	{
		S_InterpEntry& e = it->second;
		if (e.tick == g_InterpTick)
			e.cur = m;
		else
		{
			const bool consecutive = (e.tick + 1 == g_InterpTick);
			e.prev = consecutive ? e.cur : m;
			e.cur = m;
			e.tick = g_InterpTick;
			// a jump of more than 20 m in one tick is a teleport (reset, respawn): no blend
			e.hasPrev = consecutive && (F32)(m.AW() - e.prev.AW()).Length() < 20.0f;
		}
	}
	LeaveCriticalSection(&g_InterpLock);
}

static void InterpForget(const C_ModelInstance* pInstance)
{
	if (!g_pInterpMap)
		return;
	EnterCriticalSection(&g_InterpLock);
	T_InterpMap::iterator it = g_pInterpMap->lower_bound(T_InterpKey(pInstance, -0x7fffffff));
	while (it != g_pInterpMap->end() && it->first.first == pInstance)
		it = g_pInterpMap->erase(it);
	LeaveCriticalSection(&g_InterpLock);
}
#endif

// C_ModelInstance

C_ModelInstance::C_ModelInstance( const C_RenderModelData* pRenderModelData, const S32 modelIndex, const SceneGraph::C_SceneGraph* pSceneGraph ) :
	SceneGraph::C_ModelInstance(pSceneGraph, &pRenderModelData->GetScene(), pRenderModelData->GetModel(modelIndex).GetRenderListNode())
{
	if(pRenderModelData)
	{
		m_pRenderModel = &pRenderModelData->GetModel(modelIndex);
	
		MEMORY_TRACKER_PUSH("worldrender.modelinstance.materiallist");
		// copy material output data structure from render model
		if(m_pRenderListNode)
		{
			m_MaterialIndex = m_pRenderModel->GetMaterialIndex();
			m_pMaterialListData->CopyData(m_pRenderModel->GetMaterialList());
			m_pMaterialListTypes = &m_pRenderModel->GetMaterialList();
			m_pMaterialListDmaData = m_pMaterialListData->GetDmaData();
		}
		MEMORY_TRACKER_POP("worldrender.modelinstance.materiallist");

		BuildDefaultDynamicMaterialOutput();

		m_pszFriendlyName = m_pRenderModel->GetFriendlyName();
	}
	else
	{
		m_pRenderModel = NULL;
	}
}

C_ModelInstance::~C_ModelInstance()
{
#if PLATFORM_PC
	InterpForget(this);	// [Blur2PC]
#endif
	return;
}

void C_ModelInstance::BuildDefaultDynamicMaterialOutput()
{
	Maths::HMat43 matrix;

	matrix.Translation(Maths::HVec3(0.0f, 0.0f, 0.0f));

	for(S32 i = 0; i < m_pRenderModel->GetNumElements(); ++i)
	{
		SetElementMatrix(i, matrix);
	}

	m_pRenderModel->SetDefaultValues(m_pMaterialListData);
}

void C_ModelInstance::SetWorldSphere(const Maths::C_Sphere& sphere)
{
	if(GetCullNode())
	{
		GetCullNode()->SetSphere(sphere);
		/*if(GetCullNode()->GetType() == SceneGraph::I_ModelInstanceCullNode::MODELINSTANCECULLNODE_TYPE_PORTAL)
		{
			SceneGraph::C_PortalCullNode* pCullNode = (SceneGraph::C_PortalCullNode*)GetCullNode();
			assert(pCullNode);	// Sanity check.

			// [Oscar] temporary workaround until we get an official interface for disabling portal culling on specific model instances...
			if (pCullNode->GetSpatialProbeCount() > 0)
				pCullNode->SetSpatialProbeAtIndex(0, sphere.Centre());
		}

		// Flag the movement of the node to the SG.
		FlagMovement();*/
	}
	else
	{
		// Stop spam & slowdown:
		static S32 count = 0;
		if (count++ < 10)
			LOGWARN(g_Log, "ModelInstance: SetWorldSphere called but object not in SceneGraph");
	}
}

void C_ModelInstance::SetWorldSpherePosition(const Maths::HVec3& position)
{
	if(GetCullNode())
	{
		GetCullNode()->SetSpherePosition(position);
		/*if(GetCullNode()->GetType() == SceneGraph::I_ModelInstanceCullNode::MODELINSTANCECULLNODE_TYPE_PORTAL)
		{
			SceneGraph::C_PortalCullNode* pCullNode = (SceneGraph::C_PortalCullNode*)GetCullNode();
			assert(pCullNode);	// Sanity check.
			pCullNode->SetSpatialProbeAtIndex(0, position);
		}

		// Flag the movement of the node to the SG.
		FlagMovement();*/
	}
	else
	{
		// Stop spam & slowdown:
		static S32 count = 0;
		if (count++ < 10)
			LOGWARN(g_Log, "ModelInstance: SetWorldSphere called but object not in SceneGraph");
	}
}

Maths::C_Sphere C_ModelInstance::GetWorldSphere() const
{
	if(m_pCullNode)
		return m_pCullNode->GetSphere();
	else
		return Maths::C_Sphere();
}

void C_ModelInstance::SetNearFarDistance(const F32 nearDistance, const F32 farDistance)
{
	if(m_pCullNode)
		m_pCullNode->SetNearFarDistance(nearDistance, farDistance);
	else
		LOGWARN(g_Log, "ModelInstance: SetNearFarDistance called but object not in SceneGraph");
}


void C_ModelInstance::SetElementMatrix( const S32 elementIndex, const Maths::HMat43 PCREF m )
{
#if PLATFORM_PC
	if (g_InterpEnabled && m_pRenderModel)
		InterpRecord(this, elementIndex, m);	// [Blur2PC] render interpolation
#endif
	WriteElementMatrix(elementIndex, m);
}

// [Blur2PC] the original SetElementMatrix body: stores the matrix into the material data
void C_ModelInstance::WriteElementMatrix( const S32 elementIndex, const Maths::HMat43 PCREF m )
{
	if(!m_pRenderModel)
		return;

	Maths::HMat44 mat44(m);

	if(elementIndex < 0)
	{
		for(S32 j = 0, count = m_pRenderModel->GetNumElements(); j < count; ++j)
		{
			const MaterialParameter* element = m_pRenderModel->GetWorldMatrixParameter(j);
			
			if (element != NULL)
			{
				if (element->index >= 0)
				{
					RenderingData::Material& material = m_pMaterialListData->GetMaterial(element->index);
					Maths::Store(mat44, (MathsStorage::MatrixRow44A16*)material.GetData(element->offset));
				}
			}			
		}
	}
	else
	{
		if(elementIndex < m_pRenderModel->GetNumElements())
		{
			const MaterialParameter* element = m_pRenderModel->GetWorldMatrixParameter(elementIndex);

			if (element != NULL)
			{
				if(element->index >= 0 && element->index < m_pMaterialListData->GetNumMaterials())
				{
					RenderingData::Material& material = m_pMaterialListData->GetMaterial(element->index);
					Maths::Store(mat44, (MathsStorage::MatrixRow44A16*)material.GetData(element->offset));
				}
			}
		}
	}
}

Maths::HMat43 C_ModelInstance::GetElementMatrix( const S32 elementIndex ) const
{
	Maths::HMat43 mat;
	mat.SetIdentity();
	return mat;
}

void C_ModelInstance::SetParameter(const MaterialParameter& parameter, const void* pData)
{
	if (parameter.index < 0 || parameter.index >= m_pMaterialListData->GetNumMaterials())
		return;
	RenderingData::Material& material = m_pMaterialListData->GetMaterial(parameter.index);
	material.SetData(parameter.offset, pData, parameter.size);
}

U8* C_ModelInstance::GetParameter(const MaterialParameter& parameter)
{
	if (parameter.index < 0 || parameter.index >= m_pMaterialListData->GetNumMaterials())
		return 0;

	RenderingData::Material& material = m_pMaterialListData->GetMaterial(parameter.index);
	return material.GetData(parameter.offset);
}

const U8* C_ModelInstance::GetParameter(const MaterialParameter& parameter) const
{
	RenderingData::ConstMaterialReference material = m_pMaterialListData->RenderingData::MaterialListData::GetMaterial(parameter.index);
	return material.GetData(parameter.offset);
}

S32 C_ModelInstance::GetNumElements() const
{
	if(m_pRenderModel)
		return m_pRenderModel->GetNumElements();
	else
		return 0;
}	

void C_ModelInstance::AccumulateResourceUsage(RenderModelResourceUsage& ioResourceUsage) const
{
	RenderingData::Target::TextureInfoList renderingData;
	GetScene()->AccumulateTextureInfo(renderingData);
	ioResourceUsage.AccumulateRenderingDataTextures(renderingData);
}

} // namespace WorldRender
