// [Blur1Multi] Xbox 360 and PS3 -> PC conversion dispatch. See X360Convert.h.

#include <x360convert/X360Convert.h>

#include <string.h>
#include <ctype.h>

namespace X360Convert
{
	namespace
	{
		E_Source g_source = E_Source_Xbox360;

		bool EndsWith(const char* path, const char* ext)
		{
			const size_t n = strlen(path), m = strlen(ext);
			if (n < m)
				return false;
			for (size_t i = 0; i < m; ++i)
				if (tolower((unsigned char)path[n - m + i]) != ext[i])
					return false;
			return true;
		}
	}

	void SetSource(E_Source source)
	{
		g_source = source;
	}

	E_Source GetSource()
	{
		return g_source;
	}

	const char* SourceFourCC()
	{
		return g_source == E_Source_PS3 ? "PS3 " : "360 ";
	}

	bool HandlesPath(const char* path)
	{
		if (path && g_source == E_Source_PS3 && EndsWith(path, ".baf"))
			return true;
		return path && (EndsWith(path, ".texture") || EndsWith(path, ".font") || EndsWith(path, ".particlemesh") || EndsWith(path, ".bpr") || EndsWith(path, ".loc") ||
			EndsWith(path, ".bin") || EndsWith(path, ".xt") || EndsWith(path, ".dat"));
	}

	E_Result ConvertFile(const char* path, const unsigned char* in, size_t inSize,
		std::vector<unsigned char>& out, std::string& error)
	{
		if (!path)
			return E_Result_NotHandled;
		if (EndsWith(path, ".texture"))
			return ConvertTexture(in, inSize, out, error);
		if (EndsWith(path, ".font"))
			return ConvertFont(in, inSize, out, error);
		if (EndsWith(path, ".bin") || EndsWith(path, ".xt"))
		{
			// XT flasks; other .bin formats pass through
			if (inSize < 4 || memcmp(in, "FLSK", 4) != 0)
				return E_Result_NotHandled;
			return ConvertFlask(in, inSize, out, error);
		}
		if (EndsWith(path, ".loc"))
			return ConvertLoc(in, inSize, out, error);
		if (EndsWith(path, ".bpr"))
			return ConvertBundle(in, inSize, out, error);
		if (EndsWith(path, ".particlemesh"))
			return ConvertParticleMesh(in, inSize, out, error);
		if (g_source == E_Source_PS3 && EndsWith(path, ".baf"))
			return ConvertBank(in, inSize, out, error);
		if (EndsWith(path, ".dat"))
		{
			// audio object definitions; other .dat formats pass through
			if (inSize < 4 || memcmp(in, "AOBJ", 4) != 0)
				return E_Result_NotHandled;
			return ConvertAudioObject(in, inSize, out, error);
		}
		return E_Result_NotHandled;
	}
}
