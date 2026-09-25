// [Blur1Multi] Converts every Xbox 360 .fxb under a directory (an extracted
// shaders-speed pak) to PCFX with X360Convert::ConvertEffect (XenosRecomp to
// shader model 3, D3DCompile), and reports per effect. Standalone: no engine.
//
//   x360shader_verify <dir> [--dump <dir>] [--write <dir>] [--only <name>]
//
// --dump writes the first failing program's HLSL and compiler messages;
// --write writes the converted .fxb files.

#include <x360convert/X360Convert.h>

#include <windows.h>

#include <stdio.h>
#include <string>
#include <vector>

namespace
{
	bool ReadFile(const std::string& path, std::vector<unsigned char>& out)
	{
		FILE* f = NULL;
		if (fopen_s(&f, path.c_str(), "rb") != 0 || !f)
			return false;
		fseek(f, 0, SEEK_END);
		out.resize((size_t)ftell(f));
		fseek(f, 0, SEEK_SET);
		const bool ok = out.empty() || fread(&out[0], 1, out.size(), f) == out.size();
		fclose(f);
		return ok;
	}

	void WriteText(const std::string& path, const std::string& text)
	{
		FILE* f = NULL;
		if (fopen_s(&f, path.c_str(), "wb") == 0 && f)
		{
			fwrite(text.data(), 1, text.size(), f);
			fclose(f);
		}
	}
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("usage: x360shader_verify <dir> [--dump <dir>] [--write <dir>] [--only <name>]\n");
		return 2;
	}
	const std::string dir = argv[1];
	std::string dumpDir, writeDir, only;
	for (int i = 2; i + 1 < argc; ++i)
	{
		if (strcmp(argv[i], "--dump") == 0)
			dumpDir = argv[++i];
		else if (strcmp(argv[i], "--write") == 0)
			writeDir = argv[++i];
		else if (strcmp(argv[i], "--only") == 0)
			only = argv[++i];
	}

	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA((dir + "\\*.fxb").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE)
	{
		printf("no .fxb under %s\n", dir.c_str());
		return 1;
	}
	unsigned effects = 0, converted = 0;
	X360Convert::EffectStats total;
	bool dumped = false;
	do
	{
		const std::string name = fd.cFileName;
		if (!only.empty() && _stricmp(name.c_str(), only.c_str()) != 0)
			continue;
		std::vector<unsigned char> in, out;
		if (!ReadFile(dir + "\\" + name, in))
			continue;
		++effects;
		X360Convert::EffectStats stats;
		std::string error;
		const X360Convert::E_Result r = X360Convert::ConvertEffect(in.empty() ? NULL : &in[0], in.size(), out, error, &stats);
		total.programs += stats.programs;
		total.compiled += stats.compiled;
		total.failed += stats.failed;
		total.unsupported += stats.unsupported;
		if (r == X360Convert::E_Result_Converted)
		{
			++converted;
			if (!writeDir.empty())
			{
				FILE* f = NULL;
				if (fopen_s(&f, (writeDir + "\\" + name).c_str(), "wb") == 0 && f)
				{
					fwrite(&out[0], 1, out.size(), f);
					fclose(f);
				}
			}
		}
		else
			printf("FAILED %s: %s\n", name.c_str(), error.substr(0, 400).c_str());
		if (!stats.firstUnsupported.empty())
			printf("  %s: unsupported: %s\n", name.c_str(), stats.firstUnsupported.c_str());
		if (!dumpDir.empty() && !dumped && !stats.firstFailureHlsl.empty())
		{
			dumped = true;
			WriteText(dumpDir + "\\failed_program.hlsl", stats.firstFailureHlsl);
			WriteText(dumpDir + "\\failed_program.txt", name + "\n" + stats.firstFailure);
		}
	} while (FindNextFileA(h, &fd));
	FindClose(h);

	printf("effects %u, converted %u; programs %u, compiled %u, failed %u, with unsupported features %u\n",
		effects, converted, total.programs, total.compiled, total.failed, total.unsupported);
	return converted == effects ? 0 : 1;
}
