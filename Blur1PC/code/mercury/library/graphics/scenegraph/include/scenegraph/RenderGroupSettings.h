#ifndef SCENE_GRAPH_RENDER_GROUP_SETTINGS_H
#define SCENE_GRAPH_RENDER_GROUP_SETTINGS_H

#ifdef _MSC_VER
#pragma once
#endif

namespace Display
{
	class C_View;
}

// temporary until I can forwar declare these
#include <Display/Surface/Surface.h>
#include <Display/Texture/Texture2D.h>
#include <SceneGraph/Enums.h>

namespace SceneGraph
{

class C_RenderGroupSettings
{
public:
	C_RenderGroupSettings() :	 m_WindowIndex(-1), 
								m_pColourSurface(NULL),
								m_pDepthStencilSurface(NULL),
								m_pColourTexture(NULL),
								m_pDepthStencilTexture(0),
								m_Workers(-1),
								m_Subtechnique(0),
								m_DistanceSubtechnique(0),
								m_Multisample(Display::E_Multisample_None),
								m_Priority(E_RenderPriority_Medium) {}
	~C_RenderGroupSettings() {}

	void	SetWindow(const S32 windowIndex, const bool aa);

	void	SetColourDepthStencilSurface(const Display::C_Surface* pColourSurface, const Display::C_Surface* pDepthStencilSurface);
	void	SetColourDepthStencilTexture(const Display::C_Texture2D* pColourTexture, const Display::C_Texture2D* pDepthStencilTexture);
	void	SetNumWorkers(const S32 workers) { m_Workers = workers; }
	void	SetSubtechnique(const U32 subtechnique) { m_Subtechnique = subtechnique; }
	void	SetDistanceSubtechnique(const U32 distanceSubtechnique) { m_DistanceSubtechnique = distanceSubtechnique; }
	void	SetMultisample(const Display::E_Multisample multisample) { m_Multisample = multisample; }
	void	SetPriority(const E_RenderPriority renderPriority) { m_Priority = renderPriority; }

	const S32						GetWindowIndex() const { return m_WindowIndex; }
	const Display::C_Surface*		GetColourSurface() const { return m_pColourSurface; }
	const Display::C_Surface*		GetDepthStencilSurface() const { return m_pDepthStencilSurface; }
	const Display::C_Texture2D*		GetColourTexture() const { return m_pColourTexture; }
	const Display::C_Texture2D*		GetDepthStencilTexture() const { return m_pDepthStencilTexture; }
	const S32						GetNumWorkers() const { return m_Workers; }
	const U32						GetSubtechnique() const { return m_Subtechnique; }
	const U32						GetDistanceSubtechnique() const { return m_DistanceSubtechnique; }
	const Display::E_Multisample	GetMultisample() const { return m_Multisample; }
	const E_RenderPriority			GetPriority() const { return m_Priority; }

private:
	S32							m_WindowIndex;
	const Display::C_Surface*	m_pColourSurface;
	const Display::C_Surface*	m_pDepthStencilSurface;
	const Display::C_Texture2D*	m_pColourTexture;
	const Display::C_Texture2D*	m_pDepthStencilTexture;
	S32							m_Workers;
	U32							m_Subtechnique;
	U32							m_DistanceSubtechnique;
	Display::E_Multisample		m_Multisample;
	E_RenderPriority			m_Priority;
};

INLINE void C_RenderGroupSettings::SetColourDepthStencilSurface(const Display::C_Surface* pColourSurface, const Display::C_Surface* pDepthStencilSurface)
{
	m_pColourSurface = pColourSurface;
	m_pDepthStencilSurface = pDepthStencilSurface;
}

INLINE void C_RenderGroupSettings::SetColourDepthStencilTexture(const Display::C_Texture2D* pColourTexture, const Display::C_Texture2D* pDepthStencilTexture)
{
	m_pColourTexture = pColourTexture;
	m_pDepthStencilTexture = pDepthStencilTexture;
}

} // namespace SceneGraph

#endif // SCENE_GRAPH_RENDER_TASK_H
