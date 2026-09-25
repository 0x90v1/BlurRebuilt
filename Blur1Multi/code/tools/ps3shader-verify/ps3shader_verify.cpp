// [Blur1Multi] Converts every PS3 .fxb under a directory (an extracted shaders-speed
// pak) to PCFX with X360Convert::ConvertPs3Effect (RSX programs translated to HLSL,
// D3DCompile for shader model 3), and reports per effect. Standalone: no engine.
//
//   ps3shader_verify <dir> [--dump <dir>] [--write <dir>] [--hlsl <dir>] [--only <name>]
//
// --dump writes the first failing program's HLSL and compiler messages;
// --write writes the converted .fxb files; --hlsl writes every translated program
// (<effect>.<technique>.<pass>.<vs|ps>.hlsl) for reading.

#include <x360convert/X360Convert.h>

#include "RsxTranslate.h"

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
		printf("usage: ps3shader_verify <dir> [--dump <dir>] [--write <dir>] [--hlsl <dir>] [--only <name>]\n");
		return 2;
	}
	const std::string dir = argv[1];
	std::string dumpDir, writeDir, hlslDir, only;
	for (int i = 2; i + 1 < argc; ++i)
	{
		if (strcmp(argv[i], "--dump") == 0)
			dumpDir = argv[++i];
		else if (strcmp(argv[i], "--write") == 0)
			writeDir = argv[++i];
		else if (strcmp(argv[i], "--hlsl") == 0)
			hlslDir = argv[++i];
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
	unsigned effects = 0, converted = 0, withNotes = 0;
	X360Convert::EffectStats total;
	RsxTranslate::Counters counters;
	unsigned cube = 0, flat = 0, moved = 0, allocated = 0, unboundEntries = 0;
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
		RsxTranslate::EffectReport report;
		std::string error;
		const X360Convert::E_Result r = RsxTranslate::ConvertPs3Effect(in.empty() ? NULL : &in[0], in.size(), out, error, &stats, &report);
		total.programs += stats.programs;
		total.compiled += stats.compiled;
		total.failed += stats.failed;
		total.unsupported += stats.unsupported;
		counters.Add(report.counters);
		cube += report.cubeSamplers;
		flat += report.samplers2D;
		moved += report.vsBindingsMoved;
		allocated += report.psBindingsAllocated;
		unboundEntries += report.psMapEntriesUnbound;
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
		{
			++withNotes;
			printf("  %s: %s\n", name.c_str(), stats.firstUnsupported.substr(0, 300).c_str());
		}
		if (!hlslDir.empty())
			for (size_t i = 0; i < report.hlsl.size(); ++i)
			{
				std::string file = report.programNames[i];
				for (size_t k = 0; k < file.size(); ++k)
					if (file[k] == '/')
						file[k] = '.';
				WriteText(hlslDir + "\\" + name.substr(0, name.size() - 4) + "." + file + ".hlsl", report.hlsl[i]);
			}
		if (!dumpDir.empty() && !dumped && !stats.firstFailureHlsl.empty())
		{
			dumped = true;
			WriteText(dumpDir + "\\failed_program.hlsl", stats.firstFailureHlsl);
			WriteText(dumpDir + "\\failed_program.txt", name + "\n" + stats.firstFailure);
		}
	} while (FindNextFileA(h, &fd));
	FindClose(h);

	printf("effects %u, converted %u; programs %u, compiled %u, failed %u, with notes %u (effects %u)\n",
		effects, converted, total.programs, total.compiled, total.failed, total.unsupported, withNotes);
	printf("instructions: vertex %u, fragment %u; fragment literals %u, patched to constants %u\n",
		counters.vpInstructions, counters.fpInstructions, counters.fpLiterals, counters.fpPatchedLiterals);
	printf("samplers: 2D %u, cube %u; cube fetches %u; fetches from unbound units %u\n",
		flat, cube, counters.cubeFetches, counters.unboundSamplerFetches);
	printf("bindings: vertex moved below c256 %u, fragment allocated %u; fragment map entries without a binding %u\n",
		moved, allocated, unboundEntries);
	printf("reads: unbound vertex constants %u, perspective-flagged inputs %u, 2D-masked texcoord z/w %u\n",
		counters.unboundConstantReads, counters.perspectiveReads, counters.texcoord2DzwReads);
	printf("control: vertex branches %u, conditional writes %u, kills %u, render-target 1-3 outputs %u (not exported: registers ending as half temporaries %u)\n",
		counters.branches, counters.conditionalWrites, counters.kills, counters.mrtOutputs, counters.mrtSkipped);
	return converted == effects ? 0 : 1;
}
