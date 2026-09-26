#include "farmpch.h"
#include "WndCore.h"

bool Window::Create(const WinRect& Position, const HWND Parent)
{
	return (CreateWindowEx(
			m_StyleEx,
			GetWindowClassImpl(), GetWindowTitle(),
			m_Style,
			Position.left, Position.top, Position.Width(), Position.Height(),
			Parent, 0, GetModuleHandle(0),
			this
		) != 0);
}

void Window::Destroy()
{
	if (m_Wnd != INVALID_HANDLE_VALUE)
		DestroyWindow(m_Wnd);
}

LPCTSTR Window::GetWindowClassImpl()
{
	LPCTSTR WindowClassName = _T("WindowClassImpl");
	static bool Setup = false;

	if (!Setup)
	{
		WNDCLASSEX Wcx;

		ZeroMemory(&Wcx, sizeof(Wcx));

		Wcx.cbSize			= sizeof(Wcx);
		Wcx.lpszClassName	= WindowClassName;
		Wcx.lpfnWndProc		= WndProcImpl;
		Wcx.hCursor			= LoadCursor(0, IDC_ARROW);
		Wcx.hInstance		= GetModuleHandle(0);
		Wcx.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;

		Setup = (RegisterClassEx(&Wcx) != NULL);
	}

	return WindowClassName;
}

LRESULT Window::WndProcImpl(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	Window* WndClass = (Window*)GetWindowLong(Wnd, GWL_USERDATA);

	if ((WndClass == 0) && (Msg == WM_NCCREATE))
		(WndClass = (Window*)(((CREATESTRUCT*)lParam)->lpCreateParams))->Attach(Wnd);

	LRESULT Result(WndClass ? WndClass->WndProc(Msg, wParam, lParam) : 0);

	if ((WndClass != 0) && (Msg == WM_NCDESTROY))
		WndClass->Detach();

	return Result;
}