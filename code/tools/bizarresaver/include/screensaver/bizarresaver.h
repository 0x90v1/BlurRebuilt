// File: BizarreSaver.h
#pragma once

#define MAX_DEVICE_OBJECTS 10

struct DeviceObjects
{
    CD3DFont* m_pStatsFont;

    LPD3DXMESH          m_pMesh;			// mesh
    D3DXMATERIAL*       m_pMaterials;       // pointer to material info in m_pbufMaterials
    LPDIRECT3DTEXTURE9* m_ppTextures;       // array of textures, entries are NULL if no texture specified
    LPD3DXBUFFER        m_pbufMaterials;    // contains both the materials data and the filename strings
	LPD3DXBUFFER        m_pfxInstances;     // contains the effect instance data loaded with the mesh
    LPD3DXBUFFER        m_pbufAdjacency;    // Contains the adjacency info loaded with the mesh
	DWORD				m_dwNumMaterials;

	int LoadModel(HINSTANCE, LPDIRECT3DDEVICE9 m_pd3dDevice);
};


class   CBizarreScreensaver : public CD3DScreensaver
{
protected:
    DeviceObjects  m_DeviceObjectsArray[MAX_DEVICE_OBJECTS];
    DeviceObjects* m_pDeviceObjects;

    DWORD   m_dwNumVerticesArray[3];

    DWORD   m_dwMeshInterval;
    INT     m_iMesh;
    INT     m_iMeshPrev;
    FLOAT   m_fTimeNextMeshChange;
    FLOAT   m_fTimeStartMeshChange;
	BOOL	m_bWorkerThreadStarted;

    DWORD   m_dwTextureInterval;
    INT     m_iTexture;
    INT     m_iTexturePrev;
    FLOAT   m_fTimeNextTextureChange;
    FLOAT   m_fTimeStartTextureChange;

	int		m_iCPUUsage;
	int		m_iCPUIdleFrames;

	bool	m_bUseAsLogonScreenSaver;
    
    FLOAT   m_fScale1, m_fScale2;
    FLOAT   m_fRot1, m_fRot2;
	FLOAT	m_fStartWorkerInterval;

    D3DXCOLOR m_col1;
    D3DXCOLOR m_col2;
    D3DXCOLOR m_col3;

    FLOAT   m_fSpeed;

protected:
    virtual VOID    DoConfig();
    virtual VOID    ReadSettings();

    virtual VOID    SetDevice( UINT iDevice );
    virtual HRESULT Render();
    virtual HRESULT FrameMove();
    virtual HRESULT RestoreDeviceObjects();
    virtual HRESULT InvalidateDeviceObjects();
	virtual HRESULT DrawJobScreen(HWND hWnd, HDC hDC);

	virtual HRESULT OneTimeSceneInit();
	virtual HRESULT FinalCleanup();
	void	StartWorkerThread();

    VOID    Randomize( INT* piNum, INT iMax );
    HRESULT BuildTextures();
    HRESULT BuildMeshes();
    HRESULT LoadDDSTextureFromResource( LPDIRECT3DDEVICE9 pd3dDevice, 
        TCHAR* strRes, LPDIRECT3DTEXTURE9* ppTex );
    VOID    GenerateColors( INT iColorScheme, BOOL bBright, 
        D3DXCOLOR* pcol1, D3DXCOLOR* pcol2, D3DXCOLOR* pcol3 );
    HRESULT ReadRegistry();
    HRESULT WriteRegistry();


public:
    CBizarreScreensaver();

    // Override from CD3DScreensaver
    INT_PTR CALLBACK ConfigureDialogProc( HWND hwndDlg, UINT uMsg, 
                                          WPARAM wParam, LPARAM lParam );
};

