//-----------------------------------------------------------------------------
// File: BizarreSaver.cpp
//
// Desc: Bizarre Creations screen saver	/ farm client
//-----------------------------------------------------------------------------

#include <windows.h>
#include <tchar.h>

#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include <commdlg.h>
#include <commctrl.h>

#include "dxcommon/d3dsaver.h"
#include "dxcommon/d3dfont.h"
#include "dxcommon/d3dutil.h"
#include "dxcommon/dxutil.h"

#include "screensaver/unzip.h"
#include "screensaver/firewall.h"
#include "screensaver/cpuusage.h"
#include "screensaver/saverlogic.h"
#include "screensaver/bizarresaver.h"

#include "resource/Resource.h"
#include "farm/FarmSaver.h"

CBizarreScreensaver* g_pMyScreensaver = NULL;

CCpuUsage gCPUUsage;

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point to	the	program. Initializes everything, and goes into a
//		 message-processing	loop. Idle time	is used	to render the scene.
//-----------------------------------------------------------------------------
INT	WINAPI WinMain(	HINSTANCE hInst, HINSTANCE,	LPSTR, INT )
{
	int r = SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	HRESULT	hr;
	CBizarreScreensaver	bizarreSS;

	if (!OkToRunScreenSaver())
		return 0;

	if(	FAILED(	hr = bizarreSS.Create( hInst ) ) )
	{
		if ( hr != -1)
			bizarreSS.DisplayErrorMsg( hr );
		return 0;
	}

	return bizarreSS.Run();
}




//-----------------------------------------------------------------------------
// Name: CBizarreScreensaver()
// Desc: Constructor
//-----------------------------------------------------------------------------
CBizarreScreensaver::CBizarreScreensaver( )
{
	g_pMyScreensaver = this;

	InitCommonControls();

	ZeroMemory(	m_DeviceObjectsArray, sizeof(m_DeviceObjectsArray) );

	srand(0);

	lstrcpy( m_strRegPath, TEXT("Software\\Bizarre Creations\\Screensaver") );
	LoadString(	NULL, IDS_DESCRIPTION, m_strWindowTitle, 200 );
	m_dwMeshInterval = 30;
	m_iMesh	= -1;
	m_iMeshPrev	= -1;
	m_fTimeNextMeshChange =	0.0f;
	m_fTimeStartMeshChange = 0.0f;
	m_bWorkerThreadStarted = false;

	m_bUseDepthBuffer = true;

	m_dwTextureInterval	= 90;
	m_iTexture = -1;
	m_iTexturePrev = -1;
	m_fTimeNextTextureChange = 0.0f;
	m_fTimeStartTextureChange =	0.0f;
	m_iCPUIdleFrames = 0;
	m_iCPUUsage = 0;

	m_fStartWorkerInterval = 0.0f;

	m_bUseAsLogonScreenSaver = true;

	m_fSpeed = 1.0f;
}


//-----------------------------------------------------------------------------
// Name: SetDevice()
// Desc: 
//-----------------------------------------------------------------------------
VOID CBizarreScreensaver::SetDevice( UINT iDevice )
{
	m_pDeviceObjects = &m_DeviceObjectsArray[iDevice];
}


//-----------------------------------------------------------------------------
// Name: LoadModel()
// Desc: 
//-----------------------------------------------------------------------------
int	DeviceObjects::LoadModel(HINSTANCE hInstance, LPDIRECT3DDEVICE9 m_pd3dDevice)
{
	// model
	TCHAR	   strMeshPath[512];
	HRESULT	   hr;

	// open ZIP file in resources
	HRSRC hrsrc	 = FindResource(hInstance,_T("ZIPFILE"),RT_RCDATA);
	DWORD size	 = SizeofResource(hInstance,hrsrc);
	HGLOBAL hglob= LoadResource(hInstance,hrsrc);
	void *buf	 = LockResource(hglob);
	HZIP hzip	 = OpenZip(buf,size,ZIP_MEMORY);
	//
	ZIPENTRY ze;
	int index;
	FindZipItem(hzip, "model.x", true, &index, &ze);

	HGLOBAL g = GlobalAlloc(GMEM_MOVEABLE,ze.unc_size);
	void *b = GlobalLock(g);
	UnzipItem(hzip,index,b,ze.unc_size,ZIP_MEMORY);

	// Load	the	mesh from the specified	file
	DXUtil_FindMediaFile( strMeshPath, "logo.x" );

	hr = D3DXLoadMeshFromXInMemory(	(BYTE*)b, ze.unc_size, D3DXMESH_SYSTEMMEM, m_pd3dDevice, 
							&m_pbufAdjacency, &m_pbufMaterials,	&m_pfxInstances, &m_dwNumMaterials, 
							&m_pMesh );

	GlobalUnlock(g);
	GlobalFree(g);

    // Make sure there are normals, which are required for the lighting
    if( !(m_pMesh->GetFVF() & D3DFVF_NORMAL) )
    {
	    LPD3DXMESH pTempMesh;
        hr = m_pMesh->CloneMeshFVF( m_pMesh->GetOptions(), 
                                    m_pMesh->GetFVF() | D3DFVF_NORMAL, 
                                    m_pd3dDevice, &pTempMesh );
        if( FAILED(hr) )
            return hr;

        D3DXComputeNormals( pTempMesh, NULL );

        SAFE_RELEASE( m_pMesh );
        m_pMesh = pTempMesh;
    }

	if(	FAILED(hr) )
		// hide	error so that we can display a blue	screen
		return S_OK;

    // Get the array of materials out of the returned buffer, allocate a
    // texture array, and load the textures
    m_pMaterials = (D3DXMATERIAL*)m_pbufMaterials->GetBufferPointer();
    m_ppTextures = new LPDIRECT3DTEXTURE9[m_dwNumMaterials];

    for( UINT i=0; i<m_dwNumMaterials; i++ )
    {
        TCHAR strTexturePath[512] = _T("");

		// load texture from zip file
		FindZipItem(hzip, m_pMaterials[i].pTextureFilename, true, &index, &ze);

		g = GlobalAlloc(GMEM_MOVEABLE,ze.unc_size);
		b = GlobalLock(g);
		UnzipItem(hzip,index,b,ze.unc_size,ZIP_MEMORY);

		if( FAILED( D3DXCreateTextureFromFileInMemory( m_pd3dDevice, (BYTE*)b, ze.unc_size, &m_ppTextures[i] ) ) )
            m_ppTextures[i] = NULL;

		GlobalUnlock(g);
		GlobalFree(g);

    }

	CloseZip(hzip);

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: BuildMeshes()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT	CBizarreScreensaver::BuildMeshes()
{
	m_pDeviceObjects->LoadModel(m_hInstance, m_pd3dDevice);

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called	once per frame,	the	call is	the	entry point	for	animating
//		 the scene.
//-----------------------------------------------------------------------------
HRESULT	CBizarreScreensaver::FrameMove()
{
	D3DXCOLOR colBlack(	0, 0, 0, 0 );
	if (IsProcessRunning())
		m_fTime = 0;

	m_fScale1 =	6.0f;
	m_fRot1	= m_fTime * m_fSpeed;

	m_fStartWorkerInterval += m_fElapsedTime;
	if (m_fStartWorkerInterval > 0.5f)
	{
		m_fStartWorkerInterval = 0;
		StartWorkerThread();
	}

	return S_OK;
}

extern char gCmdLineBuffer[4096];

// draw some text with info about the job that is currently running
HRESULT CBizarreScreensaver::DrawJobScreen(HWND hwnd, HDC hdc)
{
	char szMessage[512];

	sprintf(szMessage, "Processing job   \n%s\n",gCmdLineBuffer);

	for (int i=0; i<m_iCurDots; i++)
		szMessage[14+i] = '.';

    HMONITOR hMonitor = MonitorFromWindow( hwnd, MONITOR_DEFAULTTONEAREST );
    MonitorInfo* pMonitorInfo = NULL;
    for( DWORD iMonitor = 0; iMonitor < m_dwNumMonitors; iMonitor++)
    {
        if( m_Monitors[iMonitor].hMonitor == hMonitor )
		{
			pMonitorInfo = &m_Monitors[iMonitor];
            break;
		}
    }

    if( !pMonitorInfo )
        return S_OK;

    // Draw the error message box
    RECT rc;
    SetRect( &rc, (INT)pMonitorInfo->xError, (INT)pMonitorInfo->yError,
        (INT)(pMonitorInfo->xError + pMonitorInfo->widthError),
        (INT)(pMonitorInfo->yError + pMonitorInfo->heightError) );

	rc.left -= 10;
	rc.right += 10;
#define ERROR_MSG_BK_COL 0xff0000

	HBRUSH hBrush = CreateSolidBrush(ERROR_MSG_BK_COL);

	FillRect(hdc, &rc, hBrush);

	DeleteObject(hBrush);
    FrameRect(hdc, &rc, (HBRUSH)GetStockObject(COLOR_WINDOW+1));
    RECT rc2;
    int height;

	rc.left += 10;
	rc.right -= 10;

    rc2 = rc;

	HFONT hfnt, hOldFont;

	SetTextColor(hdc, 0x00ffffff);
	SetBkColor(hdc, ERROR_MSG_BK_COL);

    hfnt = (HFONT)GetStockObject(SYSTEM_FIXED_FONT); 
    if (hOldFont = (HFONT)SelectObject(hdc, hfnt)) 
    { 
		height = DrawText(hdc, szMessage, -1, &rc, DT_WORDBREAK | DT_CENTER | DT_CALCRECT );
		rc = rc2;

		rc2.top = (rc.bottom + rc.top - height) / 2;

	    DrawText(hdc, szMessage, -1, &rc2, DT_WORDBREAK | DT_CENTER );
        SelectObject(hdc, hOldFont); 
    } 

	rc.left -= 10;
	rc.right += 10;
/*
    // Erase everywhere except the error message box
    ExcludeClipRect( hdc, rc.left, rc.top, rc.right, rc.bottom );
    rc = pMonitorInfo->rcScreen;
    ScreenToClient( hwnd, (POINT*)&rc.left );
    ScreenToClient( hwnd, (POINT*)&rc.right );
    FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH) );
*/
	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Called	once per frame,	the	call is	the	entry point	for	3d
//		 rendering.	This function sets up render states, clears	the
//		 viewport, and renders the scene.
//-----------------------------------------------------------------------------
extern bool gbJobRunning;

HRESULT	CBizarreScreensaver::Render()
{
	D3DXMATRIX mat1;
	D3DXMATRIX mat2;
	D3DXMATRIX mat3;

	SetProjectionMatrix( 0.05f,	120.0f );

	m_pd3dDevice->SetTextureStageState(	0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	m_pd3dDevice->SetTextureStageState(	0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	m_pd3dDevice->SetTextureStageState(	0, D3DTSS_COLOROP,	 D3DTOP_MODULATE );
	m_pd3dDevice->SetTextureStageState(	0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	m_pd3dDevice->SetTextureStageState(	0, D3DTSS_ALPHAOP,	 D3DTOP_SELECTARG1 );

	m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR	);
	m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR	);
	m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
	m_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU,	 D3DTADDRESS_WRAP );
	m_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV,	 D3DTADDRESS_WRAP );

	m_pd3dDevice->SetRenderState( D3DRS_LIGHTING,			 TRUE );
	m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE,	 FALSE );
	m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,			 D3DBLEND_ONE );
	m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND,			 D3DBLEND_ONE );
	m_pd3dDevice->SetRenderState( D3DRS_SPECULARENABLE,		 TRUE );
	m_pd3dDevice->SetRenderState( D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);

	HRESULT	hr;
	DWORD dw;
	hr = m_pd3dDevice->ValidateDevice(&dw);

	// Clear the viewport
	m_col3.r = m_col3.g = m_col3.b = m_col3.a = 0.1f;
	m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,	m_col3,	1.0f, 0L );

	// Begin the scene 
	if(	SUCCEEDED( m_pd3dDevice->BeginScene() )	)
	{
#define M_PI       3.14159265358979323846

		D3DXMatrixRotationX( &mat1,	(float)-M_PI/2.0f );
		D3DXMatrixRotationY( &mat2,	m_fRot1	);
		D3DXMatrixMultiply( &mat3, &mat1, &mat2 );
		m_pd3dDevice->SetTransform(	D3DTS_WORLD, &mat3 );

        // set and draw each of the materials in the mesh
		for( UINT i = 0; i < m_pDeviceObjects->m_dwNumMaterials; i++ )
        {
			m_pd3dDevice->SetMaterial( &m_pDeviceObjects->m_pMaterials[i].MatD3D );
			m_pd3dDevice->SetTexture( 0, m_pDeviceObjects->m_ppTextures[i] );

            m_pDeviceObjects->m_pMesh->DrawSubset( i );
        }
/*
		// print out some debug stuff
		char buffer[512];
		sprintf(buffer, "cpu usage: %3d%%\n", m_iCPUUsage);

		m_pDeviceObjects->m_pStatsFont->DrawText(10,10,0xffffffff,buffer);

		if ( m_bWorkerThreadStarted )
			m_pDeviceObjects->m_pStatsFont->DrawText(10,40,0xffffffff,"worker thread started");
*/
		// End the scene.
		m_pd3dDevice->EndScene();
	}

	return S_OK;
}

// start worker thread
void CBizarreScreensaver::StartWorkerThread()
{
	int bits;

	if (m_bWorkerThreadStarted)
		return;

	// Perform: orps xmm0, xmm0
	_asm {
		MOV EAX,1               ;request CPU feature flags
		CPUID                   ;0Fh, 0A2h CPUID instruction
		mov bits,EDX
	}

	// don't run the worker thread if we don't support SSE
	if (!(bits & 0x2000000))
		return;

	// Perform: orps xmm0, xmm0
	_asm {
		_emit 0x0f
		_emit 0x56
		_emit 0xc0
	}
	
	// only start the worker thread if CPU usage is low
	m_iCPUUsage = gCPUUsage.GetCpuUsage();

	if (m_iCPUUsage > 30)
	{
		m_iCPUIdleFrames = 0;
		return;
	}

	m_iCPUIdleFrames++;
	if (m_iCPUIdleFrames<6)
		return;

	m_bWorkerThreadStarted = true;

	if (m_SaverMode	== sm_full ||
		m_SaverMode	== sm_test)
	{
		ExecuteWorkerThread();
	}
}


//-----------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT	CBizarreScreensaver::OneTimeSceneInit()
{
	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CBizarreScreensaver::FinalCleanup()
{
	KillWorker();

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT	CBizarreScreensaver::RestoreDeviceObjects()
{
	if(	m_pd3dDevice ==	NULL )
		return S_OK;
	
	// Set up sensible projection and view matrices
	D3DXMATRIX	view;
	D3DXMatrixLookAtLH(	&view ,	&D3DXVECTOR3(0,0,-50) ,	&D3DXVECTOR3(0,0,0)	, &D3DXVECTOR3(0,1,0) );
	m_pd3dDevice->SetTransform(	D3DTS_VIEW , &view );

    // Setup the light
    D3DLIGHT9 light;
    D3DUtil_InitLight( light, D3DLIGHT_DIRECTIONAL, 0.0f,-1.0f, 1.0f );
	light.Specular.r = light.Specular.g = light.Specular.b = light.Specular.a = 1.0f;
    m_pd3dDevice->SetLight(0, &light );
    m_pd3dDevice->LightEnable(0, TRUE );

	// Set some	basic renderstates
	m_pd3dDevice->SetRenderState( D3DRS_DITHERENABLE , TRUE	);
	m_pd3dDevice->SetRenderState( D3DRS_SPECULARENABLE , FALSE );

	if(	FAILED(	BuildMeshes() )	)
		return E_FAIL;

	m_pDeviceObjects->m_pStatsFont = new CD3DFont( _T("Arial"),	12,	D3DFONT_BOLD );
	m_pDeviceObjects->m_pStatsFont->InitDeviceObjects( m_pd3dDevice	);
	m_pDeviceObjects->m_pStatsFont->RestoreDeviceObjects();

	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT	CBizarreScreensaver::InvalidateDeviceObjects()
{
	m_pDeviceObjects->m_pStatsFont->InvalidateDeviceObjects();
	m_pDeviceObjects->m_pStatsFont->DeleteDeviceObjects();
	SAFE_DELETE( m_pDeviceObjects->m_pStatsFont	);

	SAFE_RELEASE( m_pDeviceObjects->m_pMesh );
	for (DWORD i=0; i<m_pDeviceObjects->m_dwNumMaterials; i++)
		SAFE_RELEASE( m_pDeviceObjects->m_ppTextures[i] );

	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ReadSettings()
// Desc: 
//-----------------------------------------------------------------------------
VOID CBizarreScreensaver::ReadSettings()
{
	ReadRegistry();

	OpenFirewallPorts();
}




//-----------------------------------------------------------------------------
// Name: ConfigureDialogProcHelper()
// Desc: 
//-----------------------------------------------------------------------------
INT_PTR	CALLBACK ConfigureDialogProcHelper(	HWND hwndDlg, UINT uMsg, WPARAM	wParam,	LPARAM lParam )
{
	return g_pMyScreensaver->ConfigureDialogProc( hwndDlg, uMsg, wParam, lParam );
}




//-----------------------------------------------------------------------------
// Name: DoConfig()
// Desc: 
//-----------------------------------------------------------------------------
VOID CBizarreScreensaver::DoConfig()
{
	DialogBox( NULL, MAKEINTRESOURCE(IDD_CONFIGURE), m_hWndParent, ConfigureDialogProcHelper );
}




//-----------------------------------------------------------------------------
// Name: ConfigureDialogProc()
// Desc: 
//-----------------------------------------------------------------------------
INT_PTR	CALLBACK CBizarreScreensaver::ConfigureDialogProc( HWND	hwndDlg, UINT uMsg,	WPARAM wParam, LPARAM lParam )
{
	HWND hwndSpeed = GetDlgItem( hwndDlg, IDC_SPEED	);
	HWND hwndIntense = GetDlgItem( hwndDlg,	IDC_INTENSE	);
	HWND hwndPastel	= GetDlgItem( hwndDlg, IDC_PASTEL );
	HWND hwndColorInterval = GetDlgItem( hwndDlg, IDC_COLORINTERVAL	);
	HWND hwndTextureInterval = GetDlgItem( hwndDlg,	IDC_TEXTUREINTERVAL	);
	HWND hwndMeshInterval =	GetDlgItem(	hwndDlg, IDC_MESHINTERVAL );
	DWORD iSpeed;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		WriteRegistry();
		SendMessage(hwndSpeed, TBM_SETRANGE, FALSE,	MAKELONG(0,	5));
		if(	m_fSpeed ==	0.25f )
			SendMessage(hwndSpeed, TBM_SETPOS, TRUE, 0);
		else if( m_fSpeed == 0.5f )
			SendMessage(hwndSpeed, TBM_SETPOS, TRUE, 1);
		else if( m_fSpeed == 0.75f )
			SendMessage(hwndSpeed, TBM_SETPOS, TRUE, 2);
		else if( m_fSpeed == 1.0f )
			SendMessage(hwndSpeed, TBM_SETPOS, TRUE, 3);
		else if( m_fSpeed == 1.5f )
			SendMessage(hwndSpeed, TBM_SETPOS, TRUE, 4);
		else if( m_fSpeed == 2.0f )
			SendMessage(hwndSpeed, TBM_SETPOS, TRUE, 5);
/*
		if ( m_bUseAsLogonScreenSaver )
			CheckDlgButton( hwndDlg, IDC_LOGONSS,BST_CHECKED );
*/
		return TRUE;

	case WM_COMMAND:
		switch(	LOWORD(	wParam ) )
		{
		case IDC_SCREENSETTINGS:
			DoScreenSettingsDialog(	hwndDlg	);
			break;

		case IDOK:
//			m_bUseAsLogonScreenSaver = ( IsDlgButtonChecked( hwndDlg, IDC_LOGONSS ) == BST_CHECKED );
			iSpeed = (DWORD)SendMessage( hwndSpeed,	TBM_GETPOS,	0, 0 );
			if(	iSpeed == 0	)
				m_fSpeed = 0.25f;
			else if( iSpeed	== 1 )
				m_fSpeed = 0.5f;
			else if( iSpeed	== 2 )
				m_fSpeed = 0.75f;
			else if( iSpeed	== 3 )
				m_fSpeed = 1.0f;
			else if( iSpeed	== 4 )
				m_fSpeed = 1.5f;
			else if( iSpeed	== 5 )
				m_fSpeed = 2.0f;
			WriteRegistry(); //	save new settings
			EndDialog(hwndDlg, IDOK);
			break;

		case IDCANCEL:
			ReadRegistry();	// restore previous	settings
			EndDialog(hwndDlg, IDCANCEL);
			break;
		}
		return TRUE;
	
	default:
		return FALSE;
	}
}




//-----------------------------------------------------------------------------
// Name: ReadRegistry()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT	CBizarreScreensaver::ReadRegistry()
{
	HKEY hkey;
	DWORD dwType = REG_DWORD;
	DWORD dwLength = sizeof(DWORD);
	bool bFirstRun = false;

	if(	ERROR_SUCCESS == RegCreateKeyEx( HKEY_CURRENT_USER,	m_strRegPath, 
		0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,	&hkey, NULL	) )
	{
		DWORD iSpeed = 99;
		RegQueryValueEx( hkey, TEXT("Speed"), NULL,	&dwType, (BYTE*)&iSpeed, &dwLength);
		if (iSpeed == 99)
		{
			iSpeed = 3;
			bFirstRun = true;
		}
		if(	iSpeed > 5 )
			iSpeed = 5;

		if(	iSpeed == 0	)
			m_fSpeed = 0.25f;
		else if( iSpeed	== 1 )
			m_fSpeed = 0.5f;
		else if( iSpeed	== 2 )
			m_fSpeed = 0.75f;
		else if( iSpeed	== 3 )
			m_fSpeed = 1.0f;
		else if( iSpeed	== 4 )
			m_fSpeed = 1.5f;
		else if( iSpeed	== 5 )
			m_fSpeed = 2.0f;

		ReadScreenSettings(	hkey );

		RegCloseKey( hkey );
	}

#define DESKTOP_KEY TEXT(".Default\\Control Panel\\Desktop")
#define SCREENSAVER_KEY TEXT("SCRNSAVE.EXE")
/*
	if (!bFirstRun)
	{
		if(	ERROR_SUCCESS == RegCreateKeyEx( HKEY_USERS, DESKTOP_KEY, 
			0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,	&hkey, NULL	) )
		{
			char psz[MAX_PATH];
			dwLength = MAX_PATH;
			LONG r = RegQueryValueEx( hkey, SCREENSAVER_KEY, NULL, &dwType, (BYTE*)&psz, &dwLength);

			TCHAR cfn[MAX_PATH]; GetModuleFileName(NULL, cfn, MAX_PATH);

			m_bUseAsLogonScreenSaver = !strcmp(psz, cfn);

			RegCloseKey( hkey );
		}
	}
*/
	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: WriteRegistry()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT	CBizarreScreensaver::WriteRegistry()
{
	HKEY hkey;

	if(	ERROR_SUCCESS == RegCreateKeyEx( HKEY_CURRENT_USER,	m_strRegPath, 
		0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,	&hkey, NULL	) )
	{
		DWORD iSpeed;
		if(	m_fSpeed ==	0.25f )
			iSpeed = 0;
		else if( m_fSpeed == 0.5f )
			iSpeed = 1;
		else if( m_fSpeed == 0.75f )
			iSpeed = 2;
		else if( m_fSpeed == 1.0f )
			iSpeed = 3;
		else if( m_fSpeed == 1.5f )
			iSpeed = 4;
		else if( m_fSpeed == 2.0f )
			iSpeed = 5;
		RegSetValueEx( hkey, TEXT("Speed"),	NULL, REG_DWORD, (BYTE*)&iSpeed, sizeof(DWORD));

		WriteScreenSettings( hkey );

		RegCloseKey( hkey );
	}
/*
	if(	ERROR_SUCCESS == RegCreateKeyEx( HKEY_USERS, DESKTOP_KEY, 
		0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,	&hkey, NULL	) )
	{
		char *psz;
		TCHAR cfn[MAX_PATH];
		if (m_bUseAsLogonScreenSaver)
		{
			GetModuleFileName(NULL, cfn, MAX_PATH);
			psz = cfn;
		}
		else
			psz = TEXT("logon.src");
		RegSetValueEx( hkey, SCREENSAVER_KEY, NULL, REG_SZ, (BYTE*)psz, strlen(psz) );

		RegCloseKey( hkey );
	}
*/

	return S_OK;
}
