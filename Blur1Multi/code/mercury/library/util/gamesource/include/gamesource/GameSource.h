// [Blur1Multi] Game source: the directory the game's data comes from and the
// platform that wrote it. Phase 2 of Blur2PC (see AGENTS.md "Project
// stages/layers" and docs/decisions.md D10/D11). This header has no engine
// dependencies so it can be used from the earliest point of the boot.
//
// A source is a directory laid out like a Blur install or disc: cache/*.pak,
// levels/, gamedata/... PC retail data is little-endian and is served by the
// engine's own file system with the source as the working directory (layer 0
// of the overlay file system, exactly as retail). Xbox 360 and PS3 data is
// big-endian and needs the platform data layer (D09), which is not written
// yet: Select() reports such a source and the caller stops.
#pragma once

#include <stddef.h>

namespace GameSource
{
	enum E_Platform
	{
		E_Platform_Unknown = 0,
		E_Platform_PC,
		E_Platform_Xbox360,
		E_Platform_PS3,
	};

	enum E_Endian
	{
		E_Endian_Unknown = 0,
		E_Endian_Little,
		E_Endian_Big,
	};

	struct C_SourceInfo
	{
		char		m_Root[512];		// absolute directory that holds cache/, levels/, ...
		E_Platform	m_Platform;
		E_Endian	m_PakEndian;		// byte order of cache/root.pak (its "PAK2" magic)
		unsigned	m_PakVersion;		// version word after the magic (retail PC: 2, retail X360: 1)
		bool		m_HasRootPak;		// cache/root.pak present
		bool		m_HasXex;			// default.xex present (Xbox 360 disc root)
		bool		m_HasPs3Game;		// PS3_GAME/USRDIR present (PS3 disc root; m_Root is moved inside it)
		char		m_Reason[256];		// one line saying how the platform was decided
	};

	const char* PlatformName(E_Platform platform);
	const char* EndianName(E_Endian endian);

	// Look at 'dir' (absolute or relative to the working directory) and fill 'out'.
	// Returns false when the directory holds no Blur data at all (no cache/root.pak).
	bool Detect(const char* dir, C_SourceInfo& out);

	// Pick the source for this run: the first of "--source=<dir>" on the command
	// line, the BLUR_SOURCE environment variable, or the working directory (the
	// retail behaviour). A PC source becomes the working directory. Returns the
	// value of Detect().
	bool Select(C_SourceInfo& out);

	// One line for the log: platform, byte order, pak version, root, reason.
	void Describe(const C_SourceInfo& info, char* buf, size_t bufSize);

	// Stop the process for a source the build cannot serve yet: shows 'text' in a
	// message box unless BLUR2PC_AUTOMATION is set, then exits with code 3.
	void FailUnsupported(const C_SourceInfo& info, const char* text);
}
