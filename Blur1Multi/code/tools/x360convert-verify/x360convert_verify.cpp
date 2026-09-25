// [Blur1Multi] x360convert_verify: runs the Xbox 360 -> PC converters
// (code/mercury/library/util/x360convert) over an extracted retail 360 data set
// and compares every result with the retail PC file of the same asset.
//
//   x360convert_verify <x360 extract root> <pc extract root> [--ext .texture] [--write <dir>] [--ps3]
//
// --ps3: the extract root holds PS3 data (X360Convert::SetSource(E_Source_PS3)).
//
// Extract roots hold <pak>/<pak path> trees (xmem_verify --extract,
// paklist_pc.py --extract-all); a 360 file's PC counterpart is the same pak path
// under any PC pak. Masked before comparing: TextureDesc.Usage in .texture
// (uninitialised bytes in the retail PC files; the PC loader ignores it), and the same
// field of a .font's first texture page.
//
// Build (x86 developer prompt), from Blur1Multi/:
//   cl /nologo /O2 /EHsc /MD /Icode/mercury/library/util/x360convert/include code/tools/x360convert-verify/x360convert_verify.cpp
//      code/mercury/library/util/x360convert/source/*.cpp /Fo_work/tools/ /Fe_work/tools/x360convert_verify.exe

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <string>
#include <vector>

#include <x360convert/X360Convert.h>

namespace
{
	bool ReadFile(const std::string& path, std::vector<unsigned char>& data)
	{
		FILE* f = NULL;
		if (fopen_s(&f, path.c_str(), "rb") != 0 || !f)
			return false;
		_fseeki64(f, 0, SEEK_END);
		const long long n = _ftelli64(f);
		_fseeki64(f, 0, SEEK_SET);
		data.resize((size_t)n);
		const bool ok = n == 0 || fread(&data[0], 1, (size_t)n, f) == (size_t)n;
		fclose(f);
		return ok;
	}

	void Walk(const std::string& dir, const std::string& rel, std::vector<std::string>& out)
	{
		WIN32_FIND_DATAA fd;
		HANDLE h = FindFirstFileA((dir + "\\*").c_str(), &fd);
		if (h == INVALID_HANDLE_VALUE)
			return;
		do
		{
			const std::string name = fd.cFileName;
			if (name == "." || name == "..")
				continue;
			const std::string r = rel.empty() ? name : rel + "\\" + name;
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				Walk(dir + "\\" + name, r, out);
			else
				out.push_back(r);
		} while (FindNextFileA(h, &fd));
		FindClose(h);
	}

	std::string Ext(const std::string& p)
	{
		const size_t dot = p.find_last_of('.');
		std::string e = dot == std::string::npos ? "" : p.substr(dot);
		for (size_t i = 0; i < e.size(); ++i)
			e[i] = (char)tolower((unsigned char)e[i]);
		return e;
	}

	// Offset of TextureDesc.Usage in a little-endian .texture, or 0 when not found.
	size_t TextureUsageOffset(const std::vector<unsigned char>& d)
	{
		if (d.size() < 32)
			return 0;
		size_t off = 16;
		const size_t end = d[8] | (d[9] << 8) | (d[10] << 16) | ((size_t)d[11] << 24);
		while (off + 16 <= end && off + 16 <= d.size())
		{
			const size_t size = d[off + 8] | (d[off + 9] << 8) | (d[off + 10] << 16) | ((size_t)d[off + 11] << 24);
			if (memcmp(&d[off], "Data\0\0\0\0", 8) == 0)
				return off + 16 + 20;
			if (size < 16)
				return 0;
			off += size;
		}
		return 0;
	}

	struct Stats
	{
		unsigned converted, identical, differing, failed, noPc;
		Stats() : converted(0), identical(0), differing(0), failed(0), noPc(0) {}
	};
}

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		printf("usage: x360convert_verify <x360 extract root> <pc extract root> [--ext .texture] [--write <dir>] [--ps3]\n");
		return 2;
	}
	const std::string xroot = argv[1], proot = argv[2];
	std::string onlyExt, writeDir;
	for (int i = 3; i < argc; ++i)
	{
		if (strcmp(argv[i], "--ext") == 0 && i + 1 < argc)
			onlyExt = argv[++i];
		else if (strcmp(argv[i], "--write") == 0 && i + 1 < argc)
			writeDir = argv[++i];
		else if (strcmp(argv[i], "--ps3") == 0)
			X360Convert::SetSource(X360Convert::E_Source_PS3);
	}

	std::vector<std::string> pcPaks;
	{
		WIN32_FIND_DATAA fd;
		HANDLE h = FindFirstFileA((proot + "\\*").c_str(), &fd);
		if (h != INVALID_HANDLE_VALUE)
		{
			do
				if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && fd.cFileName[0] != '.')
					pcPaks.push_back(fd.cFileName);
			while (FindNextFileA(h, &fd));
			FindClose(h);
		}
	}

	std::vector<std::string> files;
	Walk(xroot, "", files);
	std::map<std::string, Stats> stats;
	std::map<std::string, unsigned> reasons;
	unsigned shown = 0;
	for (size_t i = 0; i < files.size(); ++i)
	{
		const std::string& rel = files[i];
		if (!X360Convert::HandlesPath(rel.c_str()))
			continue;
		const std::string ext = Ext(rel);
		if (!onlyExt.empty() && ext != onlyExt)
			continue;
		Stats& st = stats[ext];
		std::vector<unsigned char> in, out;
		ReadFile(xroot + "\\" + rel, in);
		std::string error;
		const X360Convert::E_Result r = X360Convert::ConvertFile(rel.c_str(), in.empty() ? NULL : &in[0], in.size(), out, error);
		if (r != X360Convert::E_Result_Converted)
		{
			++st.failed;
			++reasons[ext + ": " + error];
			if (shown++ < 10)
				printf("FAILED %s: %s\n", rel.c_str(), error.c_str());
			continue;
		}
		++st.converted;
		if (!writeDir.empty())
		{
			std::string outPath = writeDir + "\\" + rel;
			for (size_t c = 3; c < outPath.size(); ++c)
				if (outPath[c] == '\\')
				{
					outPath[c] = 0;
					CreateDirectoryA(outPath.c_str(), NULL);
					outPath[c] = '\\';
				}
			FILE* o = NULL;
			if (fopen_s(&o, outPath.c_str(), "wb") == 0 && o)
			{
				fwrite(&out[0], 1, out.size(), o);
				fclose(o);
			}
		}
		const std::string inPak = rel.substr(rel.find('\\') + 1);
		std::vector<unsigned char> pc;
		bool havePc = false;
		for (size_t p = 0; p < pcPaks.size() && !havePc; ++p)
			havePc = ReadFile(proot + "\\" + pcPaks[p] + "\\" + inPak, pc);
		// PS3 root_streaming.pak keeps its banks under audio\ (mounted at /); PC's has them
		// at its root (mounted at /audio/)
		if (!havePc && _strnicmp(inPak.c_str(), "audio\\", 6) == 0)
			for (size_t p = 0; p < pcPaks.size() && !havePc; ++p)
				havePc = ReadFile(proot + "\\" + pcPaks[p] + "\\" + inPak.substr(6), pc);
		if (!havePc)
		{
			++st.noPc;
			continue;
		}
		if (ext == ".texture")
		{
			const size_t a = TextureUsageOffset(out), b = TextureUsageOffset(pc);
			if (a && b && a == b && a + 4 <= out.size() && b + 4 <= pc.size())
				memcpy(&out[a], &pc[b], 4);
		}
		if (ext == ".font")
		{
			// each page's TextureDesc.Usage: find the "Texture" block, skip the U16 page count
			for (size_t o = 16; o + 16 <= out.size() && o + 16 <= pc.size(); )
			{
				const size_t sz = out[o + 8] | (out[o + 9] << 8) | (out[o + 10] << 16) | ((size_t)out[o + 11] << 24);
				if (memcmp(&out[o], "Texture\0", 8) == 0 && o + 18 + 24 <= out.size() && o + 18 + 24 <= pc.size())
					memcpy(&out[o + 18 + 20], &pc[o + 18 + 20], 4);	// first page; fonts here have one
				if (sz < 16)
					break;
				o += sz;
			}
		}
		if (ext == ".bpr" && out.size() >= 20 && pc.size() >= 20)
		{
			// each resource header's flags word (+10): unused by the PC loader, differs per platform
			const size_t info = out[6] | (out[7] << 8);
			size_t o = 0;
			while (o + 28 <= info && 20 + o + 12 <= pc.size())
			{
				out[20 + o + 10] = pc[20 + o + 10];
				out[20 + o + 11] = pc[20 + o + 11];
				const size_t next = out[20 + o] | (out[20 + o + 1] << 8);
				if (!next)
					break;
				o += next;
			}
		}
		if (out == pc)
		{
			++st.identical;
			continue;
		}
		++st.differing;
		size_t d = 0;
		while (d < out.size() && d < pc.size() && out[d] == pc[d])
			++d;
		if (shown++ < 20)
			printf("DIFFERS %s: converted %u bytes, PC %u bytes, first difference at 0x%x (%02x vs %02x)\n", rel.c_str(),
				(unsigned)out.size(), (unsigned)pc.size(), (unsigned)d,
				d < out.size() ? out[d] : 0, d < pc.size() ? pc[d] : 0);
	}
	int rc = 0;
	for (std::map<std::string, Stats>::iterator it = stats.begin(); it != stats.end(); ++it)
	{
		const Stats& s = it->second;
		printf("%-10s converted %u: identical to PC %u, differing %u, no PC file %u; failed %u\n",
			it->first.c_str(), s.converted, s.identical, s.differing, s.noPc, s.failed);
		if (s.differing || s.failed)
			rc = 1;
	}
	for (std::map<std::string, unsigned>::iterator it = reasons.begin(); it != reasons.end(); ++it)
		printf("  %4u x %s\n", it->second, it->first.c_str());
	return rc;
}
