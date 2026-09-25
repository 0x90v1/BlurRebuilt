// [Blur1Multi] Xbox 360 -> PC conversion of a level's scene and its levelstream (T072).
//
// A level's render file holds the scene (the rendering-data archive); its
// StreamData lists the chunks of level.levelstream by offset and size, and each
// chunk is an archive of resource-block lods (vertex/index buffers, nodes,
// textures) that StreamData::UpdateAsynch reads (LodReader<Resources> per item).
// Draw ranges in any node may index buffers in any chunk, so a level converts
// as a whole: Prepare reads the scene and every chunk once to collect the draw
// ranges, converts every chunk once more to lay the PC stream out, and converts
// the scene with its chunk table rewritten. The PC stream is never held whole
// (retail PC levelstreams reach 650 MB): ReadStream converts the chunks a read
// covers when it is asked for them.
//
// No RenderingData internals here, so the file-system layer can include it.
#pragma once

#include <string>
#include <vector>

namespace X360Convert
{
	class I_ChunkSource
	{
	public:
		virtual ~I_ChunkSource() {}
		// The 360 levelstream bytes [offset, offset + size).
		virtual bool Read(unsigned long long offset, unsigned size, std::vector<unsigned char>& out) = 0;
	};

	class C_LevelConversion
	{
	public:
		C_LevelConversion();
		~C_LevelConversion();

		// 'scene' is the 360 levelrender Scene block payload; 'source' stays in use
		// for ReadStream and is owned by this object.
		bool Prepare(const unsigned char* scene, size_t sceneSize, I_ChunkSource* source, std::string& error);

		// The converted Scene block payload.
		const std::vector<unsigned char>& Scene() const;

		unsigned long long StreamLength() const;

		// The converted levelstream bytes [offset, offset + size); bytes between
		// chunks read as zero. Thread-safe.
		bool ReadStream(unsigned long long offset, void* dst, unsigned size, std::string& error);

	private:
		C_LevelConversion(const C_LevelConversion&);
		C_LevelConversion& operator=(const C_LevelConversion&);
		struct Impl;
		Impl* m_pImpl;
	};
}
