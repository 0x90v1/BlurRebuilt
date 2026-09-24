#ifndef WORLD_RENDER_MODEL_INSTANCE_H
#define WORLD_RENDER_MODEL_INSTANCE_H

#ifdef _MSC_VER
#pragma once
#endif

#include <SceneGraph/RenderingData/ModelInstanceNode.h>

namespace SceneGraph
{
	class C_SceneGraph;
}

namespace WorldRender
{

struct MaterialParameter;
struct RenderModelResourceUsage;
class C_RenderModelData;
class C_Model;

class C_ModelInstance : public SceneGraph::C_ModelInstance
{
public:
	C_ModelInstance( const C_RenderModelData* pRenderModelData, const S32 modelIndex, const SceneGraph::C_SceneGraph* pSceneGraph );
	~C_ModelInstance();

	void				SetWorldSphere( const Maths::C_Sphere& sphere);
	void				SetWorldSpherePosition( const Maths::HVec3& position);
	Maths::C_Sphere		GetWorldSphere() const;
	void				SetNearFarDistance(const F32 nearDistance, const F32 farDistance);

	// setting and getting of 
	void				SetElementMatrix( const S32 elementIndex, const Maths::HMat43 PCREF m );
	Maths::HMat43		GetElementMatrix( const S32 elementIndex ) const;

	// set data to model parameter
	void				SetParameter(const MaterialParameter& parameter, const void* pData);
	U8*					GetParameter(const MaterialParameter& parameter);
	const U8*			GetParameter(const MaterialParameter& parameter) const;

	S32					GetNumElements() const;
	const C_Model*		GetRenderModel() const { return m_pRenderModel; }

	void				AccumulateResourceUsage(RenderModelResourceUsage& ioResourceUsage) const;

private:
	void				BuildDefaultDynamicMaterialOutput();
	
	const C_Model*		m_pRenderModel;

	// other data
};

} // namespace WorldRender


#endif // WORLD_RENDER_MODEL_INSTANCE_H

