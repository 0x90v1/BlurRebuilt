#pragma once

#include "WndMath.h"

class Object
{
public:
	inline Object() { }
	inline virtual ~Object() { }

private:
	inline Object(const Object&) { }
	inline Object& operator=(const Object&) { return *this; }
};

class Window : public Object
{
public:
	inline Window() : m_Wnd(0), m_Style(WS_OVERLAPPEDWINDOW), m_StyleEx(WS_EX_OVERLAPPEDWINDOW) { }
	inline virtual ~Window() { Destroy(); }

public:
	bool Create(const WinRect& Position = WinRect(CW_USEDEFAULT, CW_USEDEFAULT, 0, 0), const HWND Parent = 0);
	void Destroy();

public:
	inline virtual LPCTSTR GetWindowTitle() { return _T(""); }
	inline operator HWND() { return m_Wnd; }

public:
	inline void Attach(const HWND Wnd)
	{
		SetWindowLong(Wnd, GWL_USERDATA, (LONG)this);
		m_Wnd = Wnd;
	}

	inline HWND Detach()
	{
		HWND Wnd = m_Wnd;
		SetWindowLong(m_Wnd, GWL_USERDATA, 0);
		m_Wnd = 0;
		return Wnd;
	}

protected:
	inline virtual LRESULT WndProc(UINT Msg, WPARAM wParam, LPARAM lParam) { return DefWindowProc(m_Wnd, Msg, wParam, lParam); }

private:
	LPCTSTR GetWindowClassImpl();
	static LRESULT WINAPI WndProcImpl(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam);

protected:
	HWND	m_Wnd;
	DWORD	m_Style;
	DWORD	m_StyleEx;
};