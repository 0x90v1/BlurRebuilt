// [Blur1Multi] Included at the end of the overlay copy of
// renderingdata/source/noninterface/target/archive.cpp, where the scene
// reader's templates are defined: instantiates them for the Xbox 360 scene
// converter's archive and registers its streamers. See SceneTranscode.h.

#include "SceneTranscode.h"

namespace RenderingData
{
namespace Target
{
template void Deserialise(Arch::X360SceneTranscoder& ar, Scene& t);
template void Deserialise(Arch::X360SceneTranscoder& ar, ResourceBlock& t);
}
}

namespace X360Convert
{
namespace
{
	typedef Arch::Streamer<Arch::X360SceneTranscoder> T_Streamer;

	// Streams a RenderListNode like Arch::Streamer_T, then resolves the index
	// buffer of every draw range read inside it through the node's
	// IndexBufferSource, which the node reads after its primitives.
	class C_RenderListNodeStreamer : public T_Streamer
	{
	public:
		C_RenderListNodeStreamer()
		{
			Arch::SerialisationHandlerManager<T_Streamer>::Register(this,
				Arch::SerialisationIdManager::FindKey(Arch::SerialisationInfo<RenderingData::Target::RenderListNode>::GetGUID()));
		}

		void Stream(Arch::X360SceneTranscoder& ar, void* p)
		{
			SceneState& state = ar.State();
			state.nodes.push_back(std::vector<size_t>());

			RenderingData::Target::RenderListNode& node = *(RenderingData::Target::RenderListNode*) p;
			ar & node;

			const std::vector<size_t>& records = state.nodes.back();
			for (size_t i = 0; i < records.size(); ++i)
			{
				SceneState::LodRecord& r = state.lods[records[i]];
				if (r.listIndex >= 0 && r.listIndex < node.GetNumIndexBuffers())
				{
					RenderingData::ResourceIndex source = node.GetIndexBufferSource(r.listIndex);
					r.buffer = SceneState::BufferId(source.GetBlock(), source.GetIndex());
				}
			}
			state.nodes.pop_back();
		}
	};
}

void RegisterSceneTranscodeStreamers()
{
	// The GUIDs are registered with the default handlers.
	RenderingData::ActivateHandlers();
	RenderingData::Target::ActivateHandlers2();

	SERIALISATION_STREAMER(RenderingData::Node, Arch::X360SceneTranscoder)
	SERIALISATION_STREAMER(RenderingData::CullNode, Arch::X360SceneTranscoder)
	SERIALISATION_STREAMER(RenderingData::GroupNode, Arch::X360SceneTranscoder)
	SERIALISATION_STREAMER(RenderingData::LodNode, Arch::X360SceneTranscoder)
	SERIALISATION_STREAMER(RenderingData::RenderNode, Arch::X360SceneTranscoder)
	SERIALISATION_STREAMER(RenderingData::MaterialNode, Arch::X360SceneTranscoder)
	static C_RenderListNodeStreamer s_renderListNode;
}
}
