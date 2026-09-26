#ifndef FRAMEWORK_APPLICATION_PC_H
#define FRAMEWORK_APPLICATION_PC_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Framework/Application/ApplicationBase.h"

namespace Framework
{

//! C_AppClass. PC application class.

class C_AppClass
{
public:

	//! Constructor.
	C_AppClass();

	//! Destructor.
	~C_AppClass();

public:

	WNDCLASSEX			m_WndClassEx;
	ATOM				m_WndClassAtom;
	Util::C_String		m_WndClassName;
};

//! C_AppWindow. PC application window.

class C_AppWindow
{
public:

	//! Constructor.
	C_AppWindow();

	//! Destructor.
	~C_AppWindow();

public:

	Util::C_String		m_Name;
	U32					m_Style;
	S32					m_X;
	S32					m_Y;
	S32					m_W;
	S32					m_H;
	HDC					m_hDC;
	HWND				m_hWnd;
	HMENU				m_hMenu;
};

//! C_Application. PC application.

class C_Application : public C_ApplicationBase
{
public:

	//! Constructor.
	C_Application();

	//! Destructor.
	virtual ~C_Application();

	//! Gets the PC application instane.
	static C_Application* GetInstance()
	{
		return s_pInstance;
	}

	bool IsActive()
	{
		return m_Active;
	}

	void SetActive(const bool active);

	//! PC application run.
	virtual HRESULT Run();

	//! PC window message handler.
	LRESULT MyWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual bool CanRender();
	bool LoadingScreenEnabledState() const;

protected:

	//! Platform specific application initialise.
	virtual HRESULT PlatformInitialise(const C_ApplicationParams& params);

	//! Platform specific application shutdown.
	virtual void PlatformShutdown();

	//! Platform specific device params setup.
	virtual void SetupDeviceParams();

	virtual void MyKeyDown(const U8 key);

	virtual void MyKeyUp(const U8 key);

	HRESULT MyRegisterClass(const C_ApplicationParams& params);
	
	HRESULT MyCreateWindow(const C_ApplicationParams& params);

	void InitMouse();

	void IdleLoop();

	void OnMouseButton(const U32 button, const bool state);

	void OnMouseMoveAbsolute(const S32 x, const S32 y);
	
	void OnMouseMoveRelative(const S32 x, const S32 y);

	void MyShowCursor(const bool show);

	void UpdateCursor();

	void FocusChange(const bool gotFocus);

	bool IsCursorInWindow();

protected:

	C_AppClass					m_Class;
	C_AppWindow					m_Window;
	bool						m_Active;
	bool						m_Windowed;
	bool						m_HasFocus;
	bool						m_ClipCursor;
	bool						m_ShowCursor;
	bool						m_CameraLook;
	HINSTANCE					m_hInstance;

private:
	
	static C_Application*		s_pInstance;
};

}; // namespace Framework

#endif // FRAMEWORK_APPLICATION_PC_H
