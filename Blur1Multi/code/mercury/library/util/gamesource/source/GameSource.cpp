// [Blur1Multi] Game source detection and selection. See GameSource.h.
//
// Facts this relies on (docs/analysis.md "File system and paks"):
// - every Blur data set has cache/root.pak; its first four bytes are the
//   'PAK2' magic written as a native U32, so the byte order of the magic is
//   the byte order of the data: "2KAP" on PC (little-endian), "PAK2" on
//   Xbox 360 and PS3 (big-endian). The U32 after it is the pak version
//   (retail PC 2, retail Xbox 360 1).
// - an Xbox 360 disc root has its .xex (default.xex on retail) beside cache/; a PS3 disc root has
//   PS3_GAME/USRDIR/ with the data inside it.

#include <gamesource/GameSource.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace GameSource
{
	namespace
	{
		bool FileExists(const char* path)
		{
			DWORD attr = GetFileAttributesA(path);
			return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
		}

		bool DirExists(const char* path)
		{
			DWORD attr = GetFileAttributesA(path);
			return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
		}

		void Join(char* out, size_t n, const char* dir, const char* rel)
		{
			_snprintf_s(out, n, _TRUNCATE, "%s\\%s", dir, rel);
		}

		// Reads the 8-byte pak pre-header: magic and version.
		bool ReadPakHead(const char* path, unsigned char head[8])
		{
			FILE* f = NULL;
			if (fopen_s(&f, path, "rb") != 0 || !f)
				return false;
			size_t got = fread(head, 1, 8, f);
			fclose(f);
			return got == 8;
		}

		void SetReason(C_SourceInfo& out, const char* text)
		{
			strncpy_s(out.m_Reason, sizeof out.m_Reason, text, _TRUNCATE);
		}

		C_SourceInfo g_current;
	}

	const C_SourceInfo& Current()
	{
		return g_current;
	}

	const char* PlatformName(E_Platform platform)
	{
		switch (platform)
		{
		case E_Platform_PC:      return "PC";
		case E_Platform_Xbox360: return "Xbox 360";
		case E_Platform_PS3:     return "PS3";
		default:                 return "unknown";
		}
	}

	const char* EndianName(E_Endian endian)
	{
		switch (endian)
		{
		case E_Endian_Little: return "little-endian";
		case E_Endian_Big:    return "big-endian";
		default:              return "unknown byte order";
		}
	}

	bool Detect(const char* dir, C_SourceInfo& out)
	{
		memset(&out, 0, sizeof out);
		if (!dir || !*dir)
			dir = ".";
		if (!GetFullPathNameA(dir, sizeof out.m_Root, out.m_Root, NULL))
			strncpy_s(out.m_Root, sizeof out.m_Root, dir, _TRUNCATE);
		// strip a trailing separator so joins are clean
		size_t len = strlen(out.m_Root);
		while (len > 3 && (out.m_Root[len - 1] == '\\' || out.m_Root[len - 1] == '/'))
			out.m_Root[--len] = 0;

		char path[1024];

		// PS3 disc root: the data lives under PS3_GAME/USRDIR
		Join(path, sizeof path, out.m_Root, "PS3_GAME\\USRDIR");
		if (DirExists(path))
		{
			out.m_HasPs3Game = true;
			strncpy_s(out.m_Root, sizeof out.m_Root, path, _TRUNCATE);
		}

		// an Xbox 360 disc root holds its title executable: default.xex on retail discs,
		// <name>.xex on the prototypes (Kanzen_Speed.xex on Aug 31)
		Join(path, sizeof path, out.m_Root, "*.xex");
		{
			WIN32_FIND_DATAA fd;
			HANDLE h = FindFirstFileA(path, &fd);
			out.m_HasXex = h != INVALID_HANDLE_VALUE;
			if (h != INVALID_HANDLE_VALUE)
				FindClose(h);
		}

		Join(path, sizeof path, out.m_Root, "cache\\root.pak");
		unsigned char head[8];
		out.m_HasRootPak = ReadPakHead(path, head);
		if (!out.m_HasRootPak)
		{
			SetReason(out, "no cache/root.pak: not a Blur data directory");
			return false;
		}

		if (memcmp(head, "2KAP", 4) == 0)
		{
			out.m_PakEndian = E_Endian_Little;
			out.m_PakVersion = head[4] | (head[5] << 8) | (head[6] << 16) | (head[7] << 24);
		}
		else if (memcmp(head, "PAK2", 4) == 0)
		{
			out.m_PakEndian = E_Endian_Big;
			out.m_PakVersion = (head[4] << 24) | (head[5] << 16) | (head[6] << 8) | head[7];
		}
		else
		{
			SetReason(out, "cache/root.pak has no PAK2 magic");
			return false;
		}

		if (out.m_PakEndian == E_Endian_Little)
		{
			out.m_Platform = E_Platform_PC;
			SetReason(out, "little-endian root.pak");
		}
		else if (out.m_HasPs3Game)
		{
			out.m_Platform = E_Platform_PS3;
			SetReason(out, "big-endian root.pak under PS3_GAME/USRDIR");
		}
		else if (out.m_HasXex)
		{
			out.m_Platform = E_Platform_Xbox360;
			SetReason(out, "big-endian root.pak beside an .xex");
		}
		else
		{
			out.m_Platform = E_Platform_Unknown;
			SetReason(out, "big-endian root.pak with neither an .xex nor PS3_GAME: 360 or PS3 data copied without its disc root");
		}
		return true;
	}

	bool Select(C_SourceInfo& out)
	{
		const char* dir = NULL;
		static char fromArg[1024];
		for (int i = 1; i < __argc; ++i)
		{
			const char* a = __argv[i];
			if (strncmp(a, "--source=", 9) == 0 && a[9])
			{
				strncpy_s(fromArg, sizeof fromArg, a + 9, _TRUNCATE);
				dir = fromArg;
				break;
			}
			if (strcmp(a, "--source") == 0 && i + 1 < __argc)
			{
				strncpy_s(fromArg, sizeof fromArg, __argv[i + 1], _TRUNCATE);
				dir = fromArg;
				break;
			}
		}
		if (!dir)
		{
			const char* env = getenv("BLUR_SOURCE");
			if (env && *env)
				dir = env;
		}
		bool ok = Detect(dir ? dir : ".", out);
		if (ok)
			SetCurrentDirectoryA(out.m_Root);
		g_current = out;
		return ok;
	}

	void Describe(const C_SourceInfo& info, char* buf, size_t bufSize)
	{
		if (info.m_HasRootPak)
			_snprintf_s(buf, bufSize, _TRUNCATE, "%s data (%s, pak v%u) at %s: %s",
				PlatformName(info.m_Platform), EndianName(info.m_PakEndian), info.m_PakVersion,
				info.m_Root, info.m_Reason);
		else
			_snprintf_s(buf, bufSize, _TRUNCATE, "no Blur data at %s: %s", info.m_Root, info.m_Reason);
	}

	void FailUnsupported(const C_SourceInfo& info, const char* text)
	{
		char msg[2048];
		_snprintf_s(msg, sizeof msg, _TRUNCATE,
			"%s\n\nThis build cannot play %s data yet: its data layer is incomplete "
			"(Blur2PC phase 2, see docs/BOARD.md). Point --source or BLUR_SOURCE at a Blur PC install.",
			text, PlatformName(info.m_Platform));
		OutputDebugStringA(msg);
		OutputDebugStringA("\n");
		if (!getenv("BLUR2PC_AUTOMATION"))
			MessageBoxA(NULL, msg, "Blur2PC (Blur1Multi) - unsupported game source", MB_OK | MB_ICONERROR);
		ExitProcess(3);
	}
}
