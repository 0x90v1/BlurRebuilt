#ifndef DISPLAY_MANAGER_D3D9_D3D9_MANAGER_H
#define DISPLAY_MANAGER_D3D9_D3D9_MANAGER_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Manager/ManagerBase.h"

namespace Display
{
namespace D3D9
{

//! C_Multisample

struct C_Multisample
{
	bool 			m_Windowed;
	bool 			m_FullScreen;
	E_Format 		m_ColourFormat;
	E_Format 		m_DepthStencilFormat;
	E_Multisample	m_Multisample;
};

//! C_D3D9Manager

class C_D3D9Manager : public C_ManagerBase
{
public:

	C_D3D9Manager();
	~C_D3D9Manager();
	
	IDirect3D9* GetD3DObject();

	bool IsInitialised();
	bool Initialise();
	void Shutdown();

	const S32 GetMultisampleCount() const;
	const C_Multisample& GetMultisample(const S32 multisampleIndex) const;

	const S32 GetVideoModeCount() const;
	const C_VideoMode& GetVideoMode(const S32 videoModeIndex);

	const S32 FindMultisample(const E_Multisample multisample) const;
	const S32 FindVideoMode(const U32 width, const U32 height, const E_Format format);

	bool GetVideoMode(C_VideoMode* pVideoMode);

private:

	bool AddVideoMode(const U32 width, const U32 height, const E_Format format, const bool wideScreen, const bool hiDef);

	bool CheckMultisample(const E_Multisample multisample, const E_Format format, const bool windowed);

protected:

	Ut::Vector<C_Multisample>	m_MultisampleList;
	Ut::Vector<C_VideoMode>		m_VideoModeList;
	static IDirect3D9*			s_pd3dObject;
};

}; // namespace D3D9
}; // namespace Display

#include "Display/Manager/D3D9/D3D9Manager.inl"

#endif // DISPLAY_MANAGER_D3D9_D3D9_MANAGER_H
