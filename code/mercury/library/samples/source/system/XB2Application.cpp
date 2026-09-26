#include "StdAfx.h"
#include <Samples/System/XB2Application.h>
#include <Samples/System/AtgInput.h>
#include <System/Artery/Artery.h>
#include <system/Perf/ProfilerBackend.h>
#include <system/memory/memorytracker.h>

//int PreStaticInit()
//{
//	return 0;
//}

namespace Samples
{

C_XB2Application::C_XB2Application() :
	m_Quit(false)
{
	m_DeviceParams.m_LargeHeapSize		= 1024*1024*400;
	m_DeviceParams.m_SmallHeapSize		= 1024*1024*5;
	m_DeviceParams.m_DeviceMemorySize	= 1024*1024*20;

	Sys::C_FileSystem::Instance().SetWorkingDirectory("GAME:");

	m_Name = "MercuryTestAppXB2";
}

C_XB2Application::~C_XB2Application()
{
}

bool C_XB2Application::Create()
{
	LinkToDebugView(m_Name.Chars());

	if(C_Application::Create())
	{
		Display::CreateTexture2DFromFile("Textures/xb2.dds", &m_LogoTexture);

		return true;
	}
	else
	{
		return false;
	}
}

void C_XB2Application::Update()
{
	m_Input.Reset();

	ATG::GAMEPAD* pGamepad = ATG::Input::GetMergedInput();

	if(pGamepad->bRightTrigger)
		m_GameViews[0].m_Camera.m_Speed = 4.0f;
	else
	if(pGamepad->bLeftTrigger)
		m_GameViews[0].m_Camera.m_Speed = 0.25f;
	else
		m_GameViews[0].m_Camera.m_Speed = 1.0f;

	m_Input.SetButtonState(E_Button_A,			(pGamepad->wButtons & XINPUT_GAMEPAD_A)? 1.0f : 0.0f);
	m_Input.SetButtonState(E_Button_B,			(pGamepad->wButtons & XINPUT_GAMEPAD_B)? 1.0f : 0.0f);
	m_Input.SetButtonState(E_Button_X,			(pGamepad->wButtons & XINPUT_GAMEPAD_X)? 1.0f : 0.0f);
	m_Input.SetButtonState(E_Button_Y,			(pGamepad->wButtons & XINPUT_GAMEPAD_Y)? 1.0f : 0.0f);
	m_Input.SetButtonState(E_Button_Start,		(pGamepad->wButtons & XINPUT_GAMEPAD_START)? 1.0f : 0.0f);
	m_Input.SetButtonState(E_Button_Back,		(pGamepad->wButtons & XINPUT_GAMEPAD_BACK)? 1.0f : 0.0f);
	m_Input.SetButtonState(E_Button_UpPad,		(pGamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP)? 1.0f : 0.0f);
	m_Input.SetButtonState(E_Button_DnPad,		(pGamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)? 1.0f : 0.0f);
	m_Input.SetButtonState(E_Button_LtPad,		(pGamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)? 1.0f : 0.0f);
	m_Input.SetButtonState(E_Button_RtPad,		(pGamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)? 1.0f : 0.0f);
	m_Input.SetButtonState(E_Button_LtShoulder,	(pGamepad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)? 1.0f : 0.0f);
	m_Input.SetButtonState(E_Button_RtShoulder,	(pGamepad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)? 1.0f : 0.0f);
	m_Input.SetButtonState(E_Button_LtShoulder,	(pGamepad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)? 1.0f : 0.0f);
	m_Input.SetButtonState(E_Button_RtShoulder,	(pGamepad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)? 1.0f : 0.0f);
	m_Input.SetButtonState(E_Button_RtTrigger,	(float)(pGamepad->bRightTrigger) / 255.0f);
	m_Input.SetButtonState(E_Button_LtTrigger,	(float)(pGamepad->bLeftTrigger) / 255.0f);

	if(pGamepad->wPressedButtons & XINPUT_GAMEPAD_A)
		m_Input.ButtonDn(E_Button_A);
	if(pGamepad->wPressedButtons & XINPUT_GAMEPAD_B)
		m_Input.ButtonDn(E_Button_B);
	if(pGamepad->wPressedButtons & XINPUT_GAMEPAD_X)
		m_Input.ButtonDn(E_Button_X);
	if(pGamepad->wPressedButtons & XINPUT_GAMEPAD_Y)
		m_Input.ButtonDn(E_Button_Y);
	if(pGamepad->wPressedButtons & XINPUT_GAMEPAD_START)
		m_Input.ButtonDn(E_Button_Start);
	if(pGamepad->wPressedButtons & XINPUT_GAMEPAD_BACK)
		m_Input.ButtonDn(E_Button_Back);
	if(pGamepad->wPressedButtons & XINPUT_GAMEPAD_DPAD_UP)
		m_Input.ButtonDn(E_Button_UpPad);
	if(pGamepad->wPressedButtons & XINPUT_GAMEPAD_DPAD_DOWN)
		m_Input.ButtonDn(E_Button_DnPad);
	if(pGamepad->wPressedButtons & XINPUT_GAMEPAD_DPAD_LEFT)
		m_Input.ButtonDn(E_Button_LtPad);
	if(pGamepad->wPressedButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
		m_Input.ButtonDn(E_Button_RtPad);
	if(pGamepad->wPressedButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
		m_Input.ButtonDn(E_Button_LtShoulder);
	if(pGamepad->wPressedButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
		m_Input.ButtonDn(E_Button_RtShoulder);

#if defined(USER_ORENAULT) || defined(USER_DPEARCE) || defined(USER_DGILL) || defined(USER_PEREID) || defined(USER_WBROWN)
	if(	(pGamepad->wLastButtons & XINPUT_GAMEPAD_START) && 
		(pGamepad->wLastButtons & XINPUT_GAMEPAD_BACK))
		m_pSection->ExitSection();
#else
	if(m_Input.IsButtonDn(E_Button_LtPad))
		m_pSection->ExitSection();
#endif
	if(m_Input.IsButtonDn(E_Button_RtPad))
		m_pSection->EnterSection();
	if(m_Input.IsButtonDn(E_Button_UpPad))
		m_pSection->PrevSection();
	if(m_Input.IsButtonDn(E_Button_DnPad))
		m_pSection->NextSection();

	if(pGamepad->wPressedButtons & XINPUT_GAMEPAD_START)
		m_Timer.TogglePaused();

	if(pGamepad->bRightTrigger)
		m_Input.ButtonDn(E_Button_RtTrigger);
	else
		m_Input.ButtonUp(E_Button_RtTrigger);

	if(pGamepad->bLeftTrigger)
		m_Input.ButtonDn(E_Button_LtTrigger);
	else
		m_Input.ButtonUp(E_Button_LtTrigger);

	m_Input.AxisValue( E_Axis_LX, pGamepad->fX1 );
	m_Input.AxisValue( E_Axis_LY, pGamepad->fY1 );
	m_Input.AxisValue( E_Axis_RX, pGamepad->fX2 );
	m_Input.AxisValue( E_Axis_RY, pGamepad->fY2 );

	if(m_Input.IsButtonDn(E_Button_B))
		if(m_GameViewCount > 0)
			m_GameViews[0].m_Camera.Save();

	if(m_pSection->InSection())
	{
		for(S32 i = 0; i < m_GameViewCount; i++)
		{
			if(pGamepad->fX1 != 0.f)
				m_GameViews[i].m_Camera.EnableAction(E_CameraAction_Tx, pGamepad->fX1);

			if(pGamepad->fY1 != 0.f)
				m_GameViews[i].m_Camera.EnableAction(E_CameraAction_Tz, pGamepad->fY1);

			if(pGamepad->fX2 != 0.f)
				m_GameViews[i].m_Camera.EnableAction(E_CameraAction_Ax, -pGamepad->fX2);

			if(pGamepad->fY2 != 0.f)
				m_GameViews[i].m_Camera.EnableAction(E_CameraAction_Az, -pGamepad->fY2);
		}
	}

	if(m_pSection)
		m_pSection->InputSection(m_Input, m_Timer);

	C_Application::Update();
	
	for(S32 i = 0; i < m_GameViewCount; i++)
	{
		m_GameViews[i].m_Camera.Reset();
	}
}

void C_XB2Application::Run()
{
	while(!m_Quit)
	{
		//CPU_TIMING(0, Maths::RGBA(255, 255, 255, 255), "Total");

		Debug::NetworkUpdate();

		#ifdef USE_CPU_TIMINGS
		Perf::C_CPUTimingStats::UpdateStats();
		#endif

		#ifdef USE_GPU_TIMINGS
		Display::C_GPUTimingStats::UpdateStats();
		#endif

		Update();
		Render();

		if(m_EnterSection)
		{
			m_pSection->EnterSection();
			m_EnterSection = false;
		}
	}

	Destroy();
}

}; // namespace Samples

void GameLoadDebugVariables()
{
}

void GameSaveDebugVariables()
{
}

void LinkToDebugView(const char* name)
{
	XNetStartupParams xnsp;
	ZeroMemory(&xnsp, sizeof(xnsp));
	xnsp.cfgSizeOfStruct = sizeof(xnsp);
	xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY; 
	XNetStartup(&xnsp);
	
	WSADATA WsaData;
	WSAStartup(WINSOCK_VERSION, &WsaData);

	ArteryStartup(true, name);

	Debug::NetworkStart();

	Memory::InitialiseNetworkConnection();

	Perf::InitialiseNetworkConnection();
}
