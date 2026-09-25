// [Blur1Multi] xmem_verify: checks the PC XMem decoder (code/vendor/xmem-pc)
// against a reference extraction of an Xbox 360 pak.
//
//   xmem_verify <file.pak> [<reference dir>] [--stream-chunk N]
//   xmem_verify <file.pak> --list      one line per file: logical size, first 16 bytes, path
//   xmem_verify <file.pak> --extract <dir>   writes every file under <dir>\<pak path>
//
// Reads a big-endian pak v1 (retail Xbox 360 Blur, the Aug 31 prototype):
// pre-header, C_PakHeader, string table, FAT (docs/analysis.md "File system
// and paks"). Every file entry's payload is decoded with the codec the header
// names (1 raw, 3 XMem stream, 5 XMem packet), checked against the FAT's
// logical length, and, when a reference directory is given, looked up by
// SHA-1 among the reference files (names are not needed: the reference tree
// is Blur2Test's _work/disc_cache/<disc>/<archive>). XMem stream payloads are
// decoded twice, one-shot and through XMemDecompressStream fed in small
// pieces, and the two results must agree.
//
// Build (x86 developer prompt), from Blur1Multi/:
//   cl /nologo /O2 /EHsc /MD /DBLUR_XMEM_LZX /Icode/vendor/xmem-pc /Icode/vendor/libmspack code/tools/xmem-verify/xmem_verify.cpp
//      code/vendor/xmem-pc/XMemPc.cpp code/vendor/libmspack/lzxd.c /Fo_work/tools/ /Fe_work/tools/xmem_verify.exe bcrypt.lib

#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <map>

#include "xcompress.h"

static unsigned BE32(const unsigned char* p) { return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }
static unsigned long long BE64(const unsigned char* p) { return ((unsigned long long)BE32(p) << 32) | BE32(p + 4); }

static std::string Sha1(const unsigned char* data, size_t size)
{
	BCRYPT_ALG_HANDLE alg = NULL;
	BCRYPT_HASH_HANDLE h = NULL;
	unsigned char digest[20];
	BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA1_ALGORITHM, NULL, 0);
	BCryptCreateHash(alg, &h, NULL, 0, NULL, 0, 0);
	BCryptHashData(h, (PUCHAR)data, (ULONG)size, 0);
	BCryptFinishHash(h, digest, sizeof digest, 0);
	BCryptDestroyHash(h);
	BCryptCloseAlgorithmProvider(alg, 0);
	char hex[41];
	for (int i = 0; i < 20; ++i)
		sprintf_s(hex + i * 2, 3, "%02x", digest[i]);
	return hex;
}

static bool ReadAt(FILE* f, unsigned long long pos, void* buf, size_t n)
{
	if (_fseeki64(f, (long long)pos, SEEK_SET) != 0)
		return false;
	return fread(buf, 1, n, f) == n;
}

static void CollectReference(const std::string& dir, std::map<std::string, std::string>& bySha, size_t& count)
{
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA((dir + "\\*").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE)
		return;
	do
	{
		std::string name = fd.cFileName;
		if (name == "." || name == "..")
			continue;
		std::string path = dir + "\\" + name;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			CollectReference(path, bySha, count);
			continue;
		}
		FILE* f = NULL;
		if (fopen_s(&f, path.c_str(), "rb") != 0 || !f)
			continue;
		_fseeki64(f, 0, SEEK_END);
		long long size = _ftelli64(f);
		_fseeki64(f, 0, SEEK_SET);
		std::vector<unsigned char> data((size_t)size);
		if (size)
			fread(&data[0], 1, (size_t)size, f);
		fclose(f);
		bySha[Sha1(data.empty() ? NULL : &data[0], data.size())] = path;
		++count;
	} while (FindNextFileA(h, &fd));
	FindClose(h);
}

// XMem stream (codec 3): decode through XMemDecompressStream with chunked input and output.
static bool StreamDecode(const unsigned char* src, size_t srcSize, std::vector<unsigned char>& out, size_t logical, size_t chunk)
{
	XMEMDECOMPRESSION_CONTEXT ctx = NULL;
	if (FAILED(XMemCreateDecompressionContext(XMEMCODEC_LZX, NULL, XMEMCOMPRESS_STREAM, &ctx)))
		return false;
	out.assign(logical, 0);
	size_t in = 0, produced = 0;
	int stalls = 0;
	while (produced < logical)
	{
		SIZE_T inSize = srcSize - in;
		if (inSize > chunk)
			inSize = chunk;
		SIZE_T outSize = logical - produced;
		if (outSize > chunk * 3)
			outSize = chunk * 3;
		HRESULT hr = XMemDecompressStream(ctx, &out[produced], &outSize, src + in, &inSize);
		if (FAILED(hr))
		{
			XMemDestroyDecompressionContext(ctx);
			return false;
		}
		in += inSize;
		produced += outSize;
		if (inSize == 0 && outSize == 0 && ++stalls > 2)
			break;
	}
	XMemDestroyDecompressionContext(ctx);
	return produced == logical;
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("usage: xmem_verify <file.pak> [<reference dir>] [--stream-chunk N]\n");
		return 2;
	}
	const char* pakPath = argv[1];
	std::string refDir;
	size_t streamChunk = 777;
	bool list = false;
	std::string extractDir;
	for (int i = 2; i < argc; ++i)
	{
		if (strcmp(argv[i], "--list") == 0)
			list = true;
		else if (strcmp(argv[i], "--extract") == 0 && i + 1 < argc)
			extractDir = argv[++i];
		else if (strcmp(argv[i], "--stream-chunk") == 0 && i + 1 < argc)
			streamChunk = (size_t)atoi(argv[++i]);
		else
			refDir = argv[i];
	}

	FILE* f = NULL;
	if (fopen_s(&f, pakPath, "rb") != 0 || !f)
	{
		printf("cannot open %s\n", pakPath);
		return 2;
	}
	unsigned char pre[16];
	ReadAt(f, 0, pre, 16);
	if (memcmp(pre, "PAK2", 4) != 0)
	{
		printf("not a big-endian pak\n");
		return 2;
	}
	unsigned version = BE32(pre + 4), sector = BE32(pre + 8), headerSize = BE32(pre + 12);
	if (version != 1 || headerSize != 40)
	{
		printf("pak version %u header size %u: only v1 is handled here\n", version, headerSize);
		return 2;
	}
	unsigned char hdr[40];
	ReadAt(f, 16, hdr, 40);
	unsigned codec = hdr[20], fatType = hdr[21];
	unsigned long long pos = 56;
	unsigned char u4[4];
	ReadAt(f, pos, u4, 4);
	unsigned strSize = BE32(u4);
	std::vector<unsigned char> strTable(strSize + 1, 0);
	ReadAt(f, pos + 4, &strTable[0], strSize);
	{
		// PakCryptDecrypt (PakUtil.cpp): XOR with the cipher, index advanced before use
		static const char cipher[] = "xVXo40j3@$%\\%`";
		unsigned idx = 0;
		for (unsigned i = 0; i < strSize; ++i)
		{
			idx = (idx + 1) % 14;
			strTable[i] ^= (unsigned char)cipher[idx];
		}
	}
	pos += 4 + strSize;
	pos += sector - (pos % sector);			// sector padding (always at least one byte short of a full sector here)
	unsigned char fatHead[8];
	ReadAt(f, pos, fatHead, 8);
	unsigned nEntries = BE32(fatHead), fatSize = BE32(fatHead + 4);
	unsigned entrySize = fatType == 1 ? 56 : 32;
	if (fatSize != nEntries * entrySize)
	{
		printf("FAT size %u != %u entries x %u (string table size %u, FAT at 0x%llx)\n", fatSize, nEntries, entrySize, strSize, pos);
		return 2;
	}
	std::vector<unsigned char> fat(fatSize);
	ReadAt(f, pos + 8, &fat[0], fatSize);
	if (!list)
		printf("%s: v%u sector 0x%x codec %u fatType %u entries %u\n", pakPath, version, sector, codec, fatType, nEntries);

	std::map<std::string, std::string> refBySha;
	size_t refCount = 0;
	if (!refDir.empty())
	{
		CollectReference(refDir, refBySha, refCount);
		printf("reference: %zu files (%zu distinct contents) under %s\n", refCount, refBySha.size(), refDir.c_str());
	}

	unsigned files = 0, decoded = 0, matched = 0, unmatched = 0, failed = 0, streamChecked = 0, streamFailed = 0;
	unsigned long long logicalTotal = 0;
	std::map<std::string, int> seen;
	for (unsigned i = 0; i < nEntries; ++i)
	{
		const unsigned char* e = &fat[i * entrySize];
		unsigned logical = BE32(e + 12);
		unsigned long long startSector = BE64(e + 24);
		if (startSector == 0xFFFFFFFFFFFFFFFFull)
			continue;							// directory
		++files;
		unsigned long long at = startSector * sector;
		ReadAt(f, at, u4, 4);
		unsigned stored = BE32(u4);
		std::vector<unsigned char> payload(stored);
		if (stored && !ReadAt(f, at + 4, &payload[0], stored))
		{
			printf("entry %u: short read\n", i);
			++failed;
			continue;
		}
		std::vector<unsigned char> out(logical ? logical : 1);
		bool ok = false;
		if (codec == 1)
		{
			ok = stored == logical;
			if (ok && logical)
				memcpy(&out[0], &payload[0], logical);
		}
		else if (codec == 3)
		{
			SIZE_T outSize = logical;
			HRESULT hr = logical ? XMemDecompress(NULL, &out[0], &outSize, &payload[0], stored) : S_OK;
			ok = SUCCEEDED(hr) && outSize == logical;
			if (ok && logical)
			{
				std::vector<unsigned char> s;
				++streamChecked;
				if (!StreamDecode(&payload[0], stored, s, logical, streamChunk) || memcmp(&s[0], &out[0], logical) != 0)
				{
					++streamFailed;
					if (streamFailed <= 5)
						printf("entry %u: stream decode differs from one-shot\n", i);
				}
			}
			else if (!ok)
				printf("entry %u: XMemDecompress hr 0x%08lx, %zu of %u bytes\n", i, (unsigned long)hr, (size_t)outSize, logical);
		}
		else if (codec == 5)
		{
			// [u32 packet size][chunks]; each chunk [u32 compressed][data], 0xFFFFFFFF = stored
			unsigned packet = stored >= 4 ? BE32(&payload[0]) : 0;
			size_t in = 4, produced = 0;
			ok = packet != 0;
			XMEMDECOMPRESSION_CONTEXT ctx = NULL;
			XMemCreateDecompressionContext(XMEMCODEC_DEFAULT, NULL, 0, &ctx);
			while (ok && produced < logical)
			{
				if (in + 4 > stored) { ok = false; break; }
				unsigned c = BE32(&payload[in]);
				in += 4;
				size_t want = logical - produced < packet ? logical - produced : packet;
				if (c == 0xFFFFFFFFu)
				{
					if (in + want > stored) { ok = false; break; }
					memcpy(&out[produced], &payload[in], want);
					in += want;
					produced += want;
					continue;
				}
				if (in + c > stored) { ok = false; break; }
				SIZE_T outSize = want;
				HRESULT hr = XMemDecompress(ctx, &out[produced], &outSize, &payload[in], c);
				if (FAILED(hr) || outSize != want)
				{
					printf("entry %u: packet at +0x%zx hr 0x%08lx, %zu of %zu bytes\n", i, in, (unsigned long)hr, (size_t)outSize, want);
					ok = false;
					break;
				}
				in += c;
				produced += want;
			}
			XMemDestroyDecompressionContext(ctx);
			// a logical size that is an exact multiple of the packet size is followed by an
			// empty chunk (size 0) that the writer flushed
			if (ok && in + 4 == stored && BE32(&payload[in]) == 0)
				in += 4;
			if (ok && in != stored)
			{
				printf("entry %u: packet walk ended at %zu of %u stored bytes\n", i, in, stored);
				ok = false;
			}
		}
		if (!ok)
		{
			++failed;
			continue;
		}
		++decoded;
		logicalTotal += logical;
		if (!extractDir.empty())
		{
			unsigned nameOff = BE32(e + 4);
			std::string outPath = extractDir + "\\" + (nameOff < strSize ? (const char*)&strTable[nameOff] : "unnamed");
			for (size_t c = 0; c < outPath.size(); ++c)
			{
				if (outPath[c] == '/')
					outPath[c] = '\\';
				if (outPath[c] == '\\' && c > 2)
				{
					outPath[c] = 0;
					CreateDirectoryA(outPath.c_str(), NULL);
					outPath[c] = '\\';
				}
			}
			FILE* o = NULL;
			if (fopen_s(&o, outPath.c_str(), "wb") == 0 && o)
			{
				if (logical)
					fwrite(&out[0], 1, logical, o);
				fclose(o);
			}
			else
				printf("cannot write %s\n", outPath.c_str());
		}
		if (list)
		{
			unsigned nameOff = BE32(e + 4);
			printf("%u ", logical);
			for (unsigned b = 0; b < 16; ++b)
				printf(b < logical ? "%02x" : "--", b < logical ? out[b] : 0);
			printf(" %s\n", nameOff < strSize ? (const char*)&strTable[nameOff] : "?");
		}
		if (!refDir.empty())
		{
			std::string sha = Sha1(logical ? &out[0] : NULL, logical);
			if (refBySha.count(sha))
			{
				++matched;
				seen[sha]++;
			}
			else
			{
				++unmatched;
				if (unmatched <= 5)
					printf("entry %u: %u bytes, sha1 %s not in the reference\n", i, logical, sha.c_str());
			}
		}
	}
	fclose(f);
	if (list || !extractDir.empty())
		return failed ? 1 : 0;
	printf("files %u: decoded %u (%llu bytes), failed %u", files, decoded, logicalTotal, failed);
	if (codec == 3)
		printf("; stream path checked %u, differing %u", streamChecked, streamFailed);
	if (!refDir.empty())
		printf("; matched reference %u, not in reference %u; reference contents covered %zu of %zu", matched, unmatched, seen.size(), refBySha.size());
	printf("\n");
	return (failed || unmatched || streamFailed) ? 1 : 0;
}
