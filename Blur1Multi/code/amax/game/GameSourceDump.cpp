// [Blur1Multi] Verification hook for the platform data layer: reads files
// through the engine's own file system (the overlay of pak layers that
// CreateFsLayers mounted) and writes them to disk, so what the engine sees can
// be compared byte for byte with an independent extraction (Blur2Test's
// _work/disc_cache/<disc>/<archive>).
//
//   BLUR1MULTI_PAKDUMP=<list file>   one virtual path per line ("#" comments)
//
// Output: <list file dir>\dump\<path> per file and <list file dir>\dump\report.txt
// with "ok <size> <path>", "missing <path>" or "short <got>/<size> <path>".

#include "StdAfx.h"

#include <system/serialise/I_FileSystem.h>
#include <system/serialise/I_File.h>
#include <gamesource/GameSource.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace GameSource
{
	namespace
	{
		void MakeParentDirs(const char* path)
		{
			char dir[1024];
			strncpy_s(dir, sizeof dir, path, _TRUNCATE);
			for (char* p = dir + 3; *p; ++p)
			{
				if (*p == '\\' || *p == '/')
				{
					char c = *p;
					*p = 0;
					CreateDirectoryA(dir, NULL);
					*p = c;
				}
			}
		}
	}

	int DumpListedFiles(Sys::I_FileSystem& fs)
	{
		const char* list = getenv("BLUR1MULTI_PAKDUMP");
		if (!list || !*list)
			return -1;
		FILE* in = NULL;
		if (fopen_s(&in, list, "rt") != 0 || !in)
			return -1;

		char outRoot[1024];
		strncpy_s(outRoot, sizeof outRoot, list, _TRUNCATE);
		char* slash = strrchr(outRoot, '\\');
		char* fslash = strrchr(outRoot, '/');
		if (fslash > slash)
			slash = fslash;
		if (slash)
			slash[1] = 0;
		else
			outRoot[0] = 0;
		strcat_s(outRoot, sizeof outRoot, "dump\\");

		char reportPath[1024];
		_snprintf_s(reportPath, sizeof reportPath, _TRUNCATE, "%sreport.txt", outRoot);
		MakeParentDirs(reportPath);
		FILE* report = NULL;
		fopen_s(&report, reportPath, "wt");

		int ok = 0;
		char line[1024];
		while (fgets(line, sizeof line, in))
		{
			size_t n = strlen(line);
			while (n && (line[n - 1] == '\n' || line[n - 1] == '\r' || line[n - 1] == ' '))
				line[--n] = 0;
			if (!n || line[0] == '#')
				continue;

			Sys::I_File* file = fs.Open(Sys::C_FilePath(line), Sys::E_FileAccess_Read);
			if (!file)
			{
				if (report)
					fprintf(report, "missing %s\n", line);
				continue;
			}
			const U32 size = (U32)file->Length();
			U8* data = (U8*)malloc(size ? size : 1);
			const bool read = size == 0 || file->Read(data, size);
			delete file;

			char outPath[1024];
			_snprintf_s(outPath, sizeof outPath, _TRUNCATE, "%s%s", outRoot, line[0] == '/' || line[0] == '\\' ? line + 1 : line);
			for (char* p = outPath; *p; ++p)
				if (*p == '/')
					*p = '\\';
			MakeParentDirs(outPath);
			FILE* out = NULL;
			if (read && fopen_s(&out, outPath, "wb") == 0 && out)
			{
				fwrite(data, 1, size, out);
				fclose(out);
				++ok;
				if (report)
					fprintf(report, "ok %u %s\n", size, line);
			}
			else if (report)
				fprintf(report, "short ?/%u %s\n", size, line);
			free(data);
		}
		fclose(in);
		if (report)
			fclose(report);
		return ok;
	}
}
