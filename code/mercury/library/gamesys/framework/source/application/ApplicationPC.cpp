#include "FrameworkPch.h"
#include <system/debug/Blur2PCAutomation.h>	// [Blur2PC]
#include "Framework/Application/ApplicationPC.h"
#include "Framework/Layer/Layer.h"

#include <System/Serialise.h>
#include <System/perf/ProfilerBackend.h>

#include <Input/DeviceManager.h>
#include <Input/Input.h>
#include <Input/Device.h>
#include <Input/MousePC.h>

#include <Render/Utils/Globals.h>

#include <atlbase.h>
#include <atlwin.h>
#include <wtsapi32.h>

#if defined(USER_LLWRIGHT)
#define DEBUG_OUTPUT_WINDOWS_MESSAGES 0
#endif

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
#endif

LRESULT CALLBACK ApplicationWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#if !CFG_SPEED
extern S32 g_d3dOcclusionQueryCount;
#endif

namespace Framework
{

// C_AppClass

C_AppClass::C_AppClass() :
	m_WndClassAtom(0),
	m_WndClassName("Default Class Name")
{
}

C_AppClass::~C_AppClass()
{
}

// C_AppWindow

C_AppWindow::C_AppWindow() :
	m_Name("Default Window Name"),
	m_Style(0),
	m_X(0),
	m_Y(0),
	m_W(640),
	m_H(480),
	m_hDC(NULL),
	m_hWnd(NULL),
	m_hMenu(NULL)
{
}

C_AppWindow::~C_AppWindow()
{
}

// C_Application

C_Application* C_Application::s_pInstance = NULL;

C_Application::C_Application() :
	m_Active(true),
	m_HasFocus(true),
	m_Windowed(true),
	m_ClipCursor(true),
	m_ShowCursor(true),
	m_CameraLook(false),
	m_hInstance(NULL)
{
	assert(s_pInstance == NULL);
	s_pInstance = this;
}

C_Application::~C_Application()
{
	assert(s_pInstance == this);
	s_pInstance = NULL;
}

HRESULT C_Application::PlatformInitialise(const C_ApplicationParams& params)
{
	m_hInstance = params.m_PC.m_hInstance;

	if(MyRegisterClass(params) != S_OK)
		return S_FALSE;

	if(MyCreateWindow(params) != S_OK)
		return S_FALSE;

	InitMouse();

	return S_OK;
}

void C_Application::PlatformShutdown()
{
}

void C_Application::SetupDeviceParams()
{
	C_ApplicationBase::SetupDeviceParams();

	m_DisplayDeviceParams.m_hDC			= m_Window.m_hDC;
	m_DisplayDeviceParams.m_hWnd			= m_Window.m_hWnd;
	m_DisplayDeviceParams.m_hInstance		= m_hInstance;
	m_DisplayDeviceParams.m_Windowed		= m_Windowed;
}

bool C_Application::CanRender()
{
	assert(m_pDisplayDevice);
	
	bool forceReset = false;

	if(m_Windowed)
	{
		RECT rect;
		GetClientRect(m_Window.m_hWnd, &rect);

		U32 newWidth = rect.right - rect.left;
		U32 newHeight = rect.bottom - rect.top;
		
		if(newWidth != m_DisplayDeviceParams.m_Width || newHeight != m_DisplayDeviceParams.m_Height)
		{
			forceReset = true;

			m_DisplayDeviceParams.m_Width = newWidth;
			m_DisplayDeviceParams.m_Height = newHeight;
		}
		
		if (newWidth <= 0 || newHeight <= 0)
			return false;
	}

	if(m_pDisplayDevice->GetLost() || forceReset)
	{
		if(m_pDisplayDevice->CanReset())
		{
			m_pDisplayDevice->DestroyRenderersVolatile();
			m_pDisplayDevice->DestroyVolatile();

			Render::DestroyGlobalColourBuffer();
			Render::DestroyGlobalDepthStencilBuffer();

			#if !CFG_SPEED
			BIZ_ASSERT(g_d3dOcclusionQueryCount == 0);
			#endif
			
			if(m_pDisplayDevice->CreateVolatile(m_DisplayDeviceParams))
			{
				if(m_pDisplayDevice->CreateRenderersVolatile())
				{
					Render::CreateGlobalColourBuffer();
					Render::CreateGlobalDepthStencilBuffer();

					const Display::C_BackBuffer& backBuffer = m_pDisplayDevice->GetBackBuffer();

					for(S32 viewIndex = 0; viewIndex < m_SplitscreenViews.Size(); ++viewIndex)
					{
						C_View& splitscreenView = m_SplitscreenViews[viewIndex];
						splitscreenView.m_View.Setup(splitscreenView.m_ViewSetup, backBuffer.GetWidth(), backBuffer.GetHeight());
					}

					m_LayerManager.SendMessageToLayers(E_LayerMessage_Resize);

					#if defined(USER_LLWRIGHT)
					LOGINFO(g_Log, "DisplayDevice : Reset 1\n");
					#endif
				}
				else
				{
					#if defined(USER_LLWRIGHT)
					LOGINFO(g_Log, "DisplayDevice : Reset 0\n");
					#endif

					LOGWARN(g_Log, "Failed to create volatile display device renderers\n");
				}
			}
			else
			{
				#if defined(USER_LLWRIGHT)
				LOGINFO(g_Log, "DisplayDevice : Reset 0\n");
				#endif

				LOGWARN(g_Log, "Failed to create volatile display device\n");
				return false;
			}
		}

		if(m_pDisplayDevice->CanRender())
		{
			#if defined(USER_LLWRIGHT)
			LOGINFO(g_Log, "DisplayDevice : Reset 1 Render 1\n");
			#endif

			return true;
		}
		else
		{
			#if defined(USER_LLWRIGHT)
			LOGINFO(g_Log, "DisplayDevice : Reset 1 Render 0\n");
			#endif

			return false;
		}
	}

	return true;
}

HRESULT C_Application::MyRegisterClass(const C_ApplicationParams& params)
{
	HICON hIconLarge = LoadIcon(m_hInstance, MAKEINTRESOURCE(params.m_PC.m_IconResourceLarge));
	HICON hIconSmall = LoadIcon(m_hInstance, MAKEINTRESOURCE(params.m_PC.m_IconResourceSmall));

	memset(&m_Class.m_WndClassEx, 0, sizeof(m_Class.m_WndClassEx));

	m_Class.m_WndClassEx.cbSize        		= sizeof(m_Class.m_WndClassEx); 
	m_Class.m_WndClassEx.style         		= 0;
	m_Class.m_WndClassEx.lpfnWndProc   		= (WNDPROC)ApplicationWindowProc;
	m_Class.m_WndClassEx.cbClsExtra    		= 0;
	m_Class.m_WndClassEx.cbWndExtra    		= 0;
	m_Class.m_WndClassEx.hInstance     		= m_hInstance;
	m_Class.m_WndClassEx.hIcon         		= hIconLarge;
	m_Class.m_WndClassEx.hCursor       		= LoadCursor(0, IDC_ARROW);
	m_Class.m_WndClassEx.hbrBackground 		= (HBRUSH)(COLOR_WINDOW+1);
	m_Class.m_WndClassEx.lpszMenuName  		= NULL;
	m_Class.m_WndClassEx.lpszClassName 		= m_Class.m_WndClassName.Chars();
	m_Class.m_WndClassEx.hIconSm			= hIconSmall;

	m_Class.m_WndClassAtom = RegisterClassEx(&m_Class.m_WndClassEx);

	if(m_Class.m_WndClassAtom != 0)
	{
		return S_OK;
	}
	else
	{
		return S_FALSE;
	}
}

HRESULT C_Application::MyCreateWindow(const C_ApplicationParams& params)
{
	m_Window.m_Style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | (m_Windowed ? WS_OVERLAPPEDWINDOW : WS_POPUP);

	m_Window.m_hWnd = CreateWindow(
		m_Class.m_WndClassName.Chars(),
		m_Window.m_Name.Chars(),
		m_Window.m_Style,
		m_Window.m_X,
		m_Window.m_Y,
		m_Window.m_W,
		m_Window.m_H,
		NULL,
		m_Window.m_hMenu,
		m_hInstance,
		(LPVOID)&m_Window);

	if(!m_Window.m_hWnd)
		return S_FALSE;

	ShowWindow(m_Window.m_hWnd, m_Windowed ? SW_SHOW : SW_HIDE);

	UpdateWindow(m_Window.m_hWnd);

	m_Window.m_hDC = GetDC(m_Window.m_hWnd);

	return S_OK;
}

void C_Application::MyKeyDown(const U8 key)
{
	switch(key)
	{
	case VK_ESCAPE:
		/*if(m_ClipCursor)
		{
			m_ClipCursor = false;
		}
		else*/
		{
			SendMessage(m_Window.m_hWnd, WM_CLOSE, 0, 0);
		}
		break;
	}
}

void C_Application::MyKeyUp(const U8 key)
{
}

HRESULT C_Application::Run()
{
	Display::Device()->ReleaseThreadOwnership();

	MSG  msg;
	msg.message = WM_NULL;
	PeekMessage(&msg, NULL, 0U, 0U, PM_NOREMOVE);

	while(msg.message != WM_QUIT)
	{
		bool gotMsg = PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0 ? true : false;

		if(gotMsg)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			IdleLoop();
		}
	}

	return S_OK;
}

void C_Application::SetActive(const bool active)
{
	if(active)
	{
		//m_ClipCursor = true;

		if(!m_Active)
		{
			#if DEBUG_OUTPUT_WINDOWS_MESSAGES
			LOGINFO(g_Log, "Setting ACTIVE\n");
			#endif

			m_Active = true;

			UpdateCursor();
		}
	}
	else
	{
		//m_ClipCursor = false;

		if(m_Active)
		{
			#if DEBUG_OUTPUT_WINDOWS_MESSAGES
			LOGINFO(g_Log, "Setting INACTIVE\n");
			#endif
			
			m_Active = false;

			UpdateCursor();
		}
	}
}

void C_Application::InitMouse()
{
	RAWINPUTDEVICE rid[1];
    rid[0].usUsagePage	= HID_USAGE_PAGE_GENERIC; 
    rid[0].usUsage		= HID_USAGE_GENERIC_MOUSE; 
    rid[0].dwFlags		= RIDEV_INPUTSINK;   
    rid[0].hwndTarget	= m_Window.m_hWnd;
    BOOL result = RegisterRawInputDevices(rid, 1, sizeof(rid[0]));
	if(!result)
	{
		LOGWARN( Framework::g_Log, "Failed to initialise raw input mouse device" );
	}
}

void C_Application::IdleLoop()
{
	UpdateCursor();

	CHECK_MEMORY_INTEGRITY();

	OnUpdate();

	Display::Device()->AcquireThreadOwnership();
	OnRender();
	Display::Device()->ReleaseThreadOwnership();

	Perf::ProfilerCommon::Instance().EndFrame();

	if (m_RunState != E_RunState_Run)
	{
		SendMessage(m_Window.m_hWnd, WM_CLOSE, 0, 0);
	}
}

bool C_Application::IsCursorInWindow()
{
	RECT rect;
	GetWindowRect(m_Window.m_hWnd, &rect);

	POINT point;
	BOOL result = GetCursorPos(&point);

	if(point.x < rect.left)
		return false;
	else
	if(point.x > rect.right)
		return false;
	else
	if(point.y < rect.top)
		return false;
	else
	if(point.y > rect.bottom)
		return false;
	else
		return true;
}

void C_Application::OnMouseButton(const U32 button, const bool state)
{
	if(button == 0)
	{
		bool isCursorInWindow = IsCursorInWindow();

		if(isCursorInWindow)
			m_CameraLook = state;
		else
			m_CameraLook = false;
	}

	for(U32 i=0; i<E_UserInputIndex_Count; i++)
	{
		if(m_pMouseList[i])
		{
			((Input::C_MousePC*)m_pMouseList[i])->SetButtonValue(button, state ? 1.0f : 0.0f);
		}	
	}
}

void C_Application::OnMouseMoveAbsolute(const S32 x, const S32 y)
{
	if(!m_CameraLook)
		return;

	for(U32 i=0; i<E_UserInputIndex_Count; i++)
	{
		if(m_pMouseList[i])
		{
			((Input::C_MousePC*)m_pMouseList[i])->SetAxisValue(Input::C_Mouse::E_Axis_XPos, (F32)x);
			((Input::C_MousePC*)m_pMouseList[i])->SetAxisValue(Input::C_Mouse::E_Axis_YPos, (F32)y);
		}	
	}
}

void C_Application::OnMouseMoveRelative(const S32 x, const S32 y)
{
	if(!m_CameraLook)
		return;

	for(U32 i=0; i<E_UserInputIndex_Count; i++)
	{
		if(m_pMouseList[i])
		{
			((Input::C_MousePC*)m_pMouseList[i])->SetAxisValue(Input::C_Mouse::E_Axis_DeltaX, (F32)x);
			((Input::C_MousePC*)m_pMouseList[i])->SetAxisValue(Input::C_Mouse::E_Axis_DeltaY, (F32)-y);
		}	
	}
}

void C_Application::MyShowCursor(const bool show)
{
	return;

	if(show)
	{
		if(!m_ShowCursor)
		{
			::ShowCursor(true);
			m_ShowCursor = true;
		}
	}
	else
	{
		if(m_ShowCursor)
		{
			::ShowCursor(false);
			m_ShowCursor = false;
		}
	}
}

void C_Application::UpdateCursor()
{
	return;

	RECT rcClip;
	GetWindowRect(m_Window.m_hWnd, &rcClip); 

	if(m_ClipCursor)
	{
		MyShowCursor(false);

		POINT point;
		BOOL result = GetCursorPos(&point);

		bool inWindow = true;
		if(point.x < rcClip.left)
			inWindow = false;
		if(point.x > rcClip.right)
			inWindow = false;
		if(point.y < rcClip.top)
			inWindow = false;
		if(point.y > rcClip.bottom)
			inWindow = false;

		if(!inWindow)
		{
			point.x = Maths::Max(point.x, rcClip.left);
			point.x = Maths::Min(point.x, rcClip.right);
			point.y = Maths::Max(point.y, rcClip.top);
			point.y = Maths::Min(point.y, rcClip.bottom);

			SetCursorPos(point.x, point.y);
		}

		::ClipCursor(&rcClip);
	}
	else
	{
		::ClipCursor(NULL); 

		MyShowCursor(true);
	}
}

void C_Application::FocusChange(const bool gotFocus)
{
	// [Blur2PC] unattended scripted runs must not pause when someone uses another window
	if (!gotFocus && Blur2PCAutomation::IsActive())
		return;
	if(m_HasFocus)
	{
		if(!gotFocus)
		{
			m_HasFocus = false;
		
			SetActive(false);
		}
	}
	else
	{
		if(gotFocus)
		{
			m_HasFocus = true;
		
			SetActive(true);
		}
	}
}

LRESULT C_Application::MyWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(hWnd == m_Window.m_hWnd)
	{
		switch(uMsg)
		{
			case WM_PAINT:
			{
				#if DEBUG_OUTPUT_WINDOWS_MESSAGES
				LOGINFO(g_Log, "WindowsMessage : WM_PAINT\n");
				#endif
			}
			break;

			case WM_ENTERSIZEMOVE:
			{
				#if DEBUG_OUTPUT_WINDOWS_MESSAGES
				LOGINFO(g_Log, "WindowsMessage : WM_ENTERSIZEMOVE\n");
				#endif

				SetFocusPaused(true);
			}
			break;

			case WM_EXITSIZEMOVE:
			{
				#if DEBUG_OUTPUT_WINDOWS_MESSAGES
				LOGINFO(g_Log, "WindowsMessage : WM_EXITSIZEMOVE\n");
				#endif

				SetFocusPaused(false);
			}
			break;

			case WM_ENTERMENULOOP:
			{
				#if DEBUG_OUTPUT_WINDOWS_MESSAGES
				LOGINFO(g_Log, "WindowsMessage : WM_ENTERMENULOOP\n");
				#endif

				SetFocusPaused(true);
			}
			break;

			case WM_EXITMENULOOP:
			{
				#if DEBUG_OUTPUT_WINDOWS_MESSAGES
				LOGINFO(g_Log, "WindowsMessage : WM_EXITMENULOOP\n");
				#endif

				SetFocusPaused(false);
			}
			break;

			case WM_SYSCOMMAND:
			{
				#if DEBUG_OUTPUT_WINDOWS_MESSAGES
				LOGINFO(g_Log, "WindowsMessage : WM_SYSCOMMAND\n");
				#endif
			}
			break;

			case WM_LBUTTONDOWN:
			{
				#if DEBUG_OUTPUT_WINDOWS_MESSAGES
				LOGINFO(g_Log, "WindowsMessage : WM_LBUTTONDOWN\n");
				#endif

				SetActive(true);
			}
			break;

			/*case WM_ACTIVATEAPP:
			{
				#if DEBUG_OUTPUT_WINDOWS_MESSAGES
				LOGINFO(g_Log, "WindowsMessage : WM_ACTIVATEAPP\n");
				#endif

				switch(wParam)
				{
					case 0:
					{
						SetActive(true);
					}
					break;
					
					default:
					{
						SetActive(false);
					}
					break;
				}
			}
			break;*/

			case WM_ACTIVATE:
			{
				#if DEBUG_OUTPUT_WINDOWS_MESSAGES
				LOGINFO(g_Log, "WindowsMessage : WM_ACTIVATE\n");
				#endif

				switch(wParam)
				{
					case WA_INACTIVE:
					{
						FocusChange(false);
					}
					break;

					case WA_ACTIVE:
					{
						FocusChange(true);
					}
					break;

					case WA_CLICKACTIVE:
					{
						FocusChange(true);
					}
					break;
				}
			}
			break;

			case WM_SETFOCUS:
			{
				#if DEBUG_OUTPUT_WINDOWS_MESSAGES
				LOGINFO(g_Log, "WindowsMessage : WM_SETFOCUS\n");
				#endif

				FocusChange(true);
			}
			break;

			case WM_KILLFOCUS:
			{
				#if DEBUG_OUTPUT_WINDOWS_MESSAGES
				LOGINFO(g_Log, "WindowsMessage : WM_KILLFOCUS\n");
				#endif

				FocusChange(false);
			}
			break;

			case WM_SIZE:
			{
				#if DEBUG_OUTPUT_WINDOWS_MESSAGES
				LOGINFO(g_Log, "WindowsMessage : WM_SIZE\n");
				#endif

				if(wParam == SIZE_MINIMIZED)
				{
				}
				else
				{
					RECT rect;
					GetClientRect(m_Window.m_hWnd, &rect);
					if(rect.top == 0 && rect.bottom == 0)
					{
					}
					else
					if(wParam == SIZE_MAXIMIZED)
					{
					}
					else
					if(wParam == SIZE_RESTORED)
					{
					}
				}
			}
			break;

			case WM_DESTROY:
			{
				#if DEBUG_OUTPUT_WINDOWS_MESSAGES
				LOGINFO(g_Log, "WindowsMessage : WM_DESTROY\n");
				#endif
				
				::ClipCursor(NULL);

				PostQuitMessage(0);
			}
			break;

			case WM_KEYDOWN:
			{
				#if DEBUG_OUTPUT_WINDOWS_MESSAGES
				LOGINFO(g_Log, "WindowsMessage : WM_KEYDOWN\n");
				#endif

				MyKeyDown((U8)wParam);
			}
			break;

			case WM_KEYUP:
			{
				#if DEBUG_OUTPUT_WINDOWS_MESSAGES
				LOGINFO(g_Log, "WindowsMessage : WM_KEYUP\n");
				#endif

				MyKeyUp((U8)wParam);
			}
			break;

			case WM_CHAR:
			{
				#if DEBUG_OUTPUT_WINDOWS_MESSAGES
				LOGINFO(g_Log, "WindowsMessage : WM_CHAR\n");
				#endif
			}
			break;

			case WM_MOUSEMOVE:
			{
				#if DEBUG_OUTPUT_WINDOWS_MESSAGES
				//LOGINFO(g_Log, "WindowsMessage : WM_MOUSEMOVE\n");
				#endif

				const S32 xPos = GET_X_LPARAM(lParam); 
				const S32 yPos = GET_Y_LPARAM(lParam);

				OnMouseMoveAbsolute(xPos, yPos);
			}
			break;

			case WM_INPUT: 
			{
				#if DEBUG_OUTPUT_WINDOWS_MESSAGES
				//LOGINFO(g_Log, "WindowsMessage : WM_INPUT\n");
				#endif

				UINT dataSize = sizeof(RAWINPUT);
				BYTE data[sizeof(RAWINPUT)];
	    		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, data, &dataSize, sizeof(RAWINPUTHEADER));
	    		
				RAWINPUT* pRawInput = (RAWINPUT*)data;

				if(pRawInput->header.dwType == RIM_TYPEMOUSE) 
				{
					const S32 xDelta = pRawInput->data.mouse.lLastX;
					const S32 yDelta = pRawInput->data.mouse.lLastY;
					
					if(pRawInput->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN)
						OnMouseButton(0, true);
					if(pRawInput->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP)
						OnMouseButton(0, false);
					
					if(pRawInput->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN)
						OnMouseButton(1, true);
					if(pRawInput->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP)
						OnMouseButton(1, false);
					
					if(pRawInput->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN)
						OnMouseButton(2, true);
					if(pRawInput->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_3_UP)
						OnMouseButton(2, false);
					
					if(pRawInput->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN)
						OnMouseButton(3, true);
					if(pRawInput->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP)
						OnMouseButton(3, false);
					
					if(pRawInput->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN)
						OnMouseButton(4, true);
					if(pRawInput->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP)
						OnMouseButton(4, false);
					
					OnMouseMoveRelative(xDelta, yDelta);
				} 
			}
			break;
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

bool C_Application::LoadingScreenEnabledState() const
{
	return false;
}

}; // namespace Framework

LRESULT CALLBACK ApplicationWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(Framework::C_Application::GetInstance())
	{
		return Framework::C_Application::GetInstance()->MyWindowProc(hWnd, uMsg, wParam, lParam);
	}
	else
	{
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}
