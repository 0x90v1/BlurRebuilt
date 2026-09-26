#ifndef GAME_WORLD_VIEW_MAIN_H
#define GAME_WORLD_VIEW_MAIN_H

#ifdef _MSC_VER
#pragma once
#endif

#include <SceneGraph/RenderGroupSettings.h>
#include <fx/Fx.h>
#include <Render/ReducedRender/Enums.h>

namespace SceneGraph
{
	class C_SceneGraph;
	class C_View;
}

namespace Framework
{
	class C_View;
}

namespace Game
{

class C_View;
class C_ViewManager;

class C_MainView
{
public:
	C_MainView(C_ViewManager* pViewManager) : m_pViewManager(pViewManager), m_pMain(NULL), m_pMainPrepass(NULL), m_InstanceDrawDistanceScale(1.0f), m_ModelInstanceDrawDistanceScale(1.0f) {}
	~C_MainView() {}

	void	Create(const C_View& gameView);
	void	Destroy();

	// main view stuff
	void	UpdateView(const C_View& gameView);
	
	void	Gather(C_View& gameView);
	void	Schedule(const C_View& gameView);
	
	void	RenderPrepass1(C_View& gameView);
	void	RenderPrepass2(C_View& gameView);
	void	RenderWithAA(C_View& gameView);
	void	RenderWithoutAA(C_View& gameView);
	void	SetDynamicDetail(F32 detail);

	friend class C_MultiView;
	friend class C_View;

private:
	void	SetupPrepassLightSpheres() const;

	C_ViewManager*						m_pViewManager;
	SceneGraph::C_View*					m_pMain;
	SceneGraph::C_View*					m_pMainPrepass;
	SceneGraph::C_RenderGroupSettings	m_PrepassSettings;
	SceneGraph::C_RenderGroupSettings	m_MainWithAASettings;
	SceneGraph::C_RenderGroupSettings	m_MainWithoutAASettings;

	Render::Reduced::E_Downsample		m_ReduceRenderAlpha;
	F32									m_InstanceDrawDistanceScale;
	F32									m_ModelInstanceDrawDistanceScale;
	FX::SamplerStateHandle				m_MirrorLinear, m_ClampPoint, m_ClampLinear;
	Display::C_Texture2D				m_BlackTexture;
};

} // namespace Game

#endif // GAME_WORLD_VIEW_MAIN_H
