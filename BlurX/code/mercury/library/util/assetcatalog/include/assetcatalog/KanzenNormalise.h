// [BlurX] Kanzen (Blur 2 prototype, Generation B) -> Blur 1 (Generation A) file
// normalisation, still in Xbox 360 byte order. BlurX's catalog layer applies it
// to files served from a Generation B disc; Blur1Multi's Xbox 360 converters then
// turn the result into PC data as they do for retail Blur 1, so no Blur 2 code
// enters phase 2. No engine dependencies (also built into
// code/tools/kanzen-verify).
//
// Every normaliser reads the whole Kanzen file with the layout Blur2Test
// established, writes the Blur 1 layout the engine's reader expects, and checks
// that the output walks to its end under that layout; anything else is a
// failure, never a guess. Fields Blur 1 has no place for are dropped and listed
// at each normaliser.
#pragma once

#include <string>
#include <vector>

namespace KanzenNormalise
{
	enum E_Result
	{
		E_Result_NotHandled = 0,	// not a Kanzen-only layout (or not a type handled here): serve as it is
		E_Result_Normalised,		// 'out' holds the Blur 1 layout
		E_Result_Failed,			// a Kanzen layout that could not be read; 'error' says why
	};

	// 'path' decides the type by extension. 'in' is the file as stored on the disc.
	E_Result Normalise(const char* path, const unsigned char* in, size_t size, std::vector<unsigned char>& out, std::string& error);

	// True for the extensions some normaliser handles (the layer only reads those whole).
	bool Handles(const char* path);

	// .cycle: Kanzen root class 0x10/0x11 (cycle versions 16/17) -> 0x0E/0x0F (14/15).
	E_Result NormaliseCycle(const unsigned char* in, size_t size, std::vector<unsigned char>& out, std::string& error);

	// .skeleton: Kanzen root class 8 -> C_Skeleton version 7 (E_SkeletonVersion_JointTransformWithScale).
	E_Result NormaliseSkeleton(const unsigned char* in, size_t size, std::vector<unsigned char>& out, std::string& error);

	// .model: drops the Model block's Kanzen-only SmpVol child (Blur 1's reader has no place for it).
	E_Result NormaliseModel(const unsigned char* in, size_t size, std::vector<unsigned char>& out, std::string& error);

	// .levelrender: light samples version 3 -> 1, CubePt3 -> CubePt2, Weather dropped (T030);
	// the Scene archive is left to BlurX's scene transcoder.
	E_Result NormaliseLevelRender(const unsigned char* in, size_t size, std::vector<unsigned char>& out, std::string& error);
}
