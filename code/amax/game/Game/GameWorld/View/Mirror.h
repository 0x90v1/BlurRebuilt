#ifndef GAME_WORLD_VIEW_MIRROR_H
#define GAME_WORLD_VIEW_MIRROR_H

#ifdef _MSC_VER
#pragma once
#endif

#include <SceneGraph/RenderStates.h>
#include <SceneGraph/RenderGroupSettings.h>

namespace SceneGraph
{
	class C_SceneGraph;
	class C_View;
}

namespace GameView
{
	class C_OffscreenView;
}

namespace Framework
{
	class C_View;
}

namespace Game
{

class C_View;
class C_ViewManager;

class C_MirrorView
{
public:
	C_MirrorView(C_ViewManager* pViewManager) : m_pViewManager(pViewManager), m_pView(NULL), m_pMirror(NULL) {}
	~C_MirrorView() {}

	void	Create();
	void	Destroy();

	void	Gather(C_View& gameView);
	void	Schedule(C_View& gameView);
	void	Render(C_View& gameView);

	void	SetupMirror(const Maths::HVec3 position, const Maths::HVec3 view, const Maths::HVec3 up, const F32 fieldOfView);
	void	SetDynamicDetail(F32 detail);
#if PLATFORM_PC
	void	SetGraphicalDetail(const U32 graphicalDetail);	// [Blur2PC] retail 0x004295f0
#endif

	const Display::C_RenderTexture2D*	GetRenderTexture() const;

private:
	SceneGraph::C_View*				m_pView;
	GameView::C_OffscreenView*		m_pMirror;			
	SceneGraph::RenderStates		m_renderStates;
	SceneGraph::RenderStates		m_MirrorRefractAlphaRenderStates;
	C_ViewManager*						m_pViewManager;
	SceneGraph::C_RenderGroupSettings	m_RenderGroupSettings;
	F32									m_DetailFactor;
};

} // namespace Game

#endif // GAME_WORLD_VIEW_MAIN_H
