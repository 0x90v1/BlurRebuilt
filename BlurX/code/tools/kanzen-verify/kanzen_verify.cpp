// [BlurX] kanzen_verify: run the Kanzen -> Blur 1 normalisers (assetcatalog/KanzenNormalise)
// over every file under a folder and report what normalised, what failed and why.
//   kanzen_verify <folder> [--write <out folder>]
// Build: BlurX/_work/tools/build_kanzen_verify.bat
#include <assetcatalog/KanzenNormalise.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::cerr << "usage: kanzen_verify <folder> [--write <out folder>]\n";
		return 2;
	}
	std::filesystem::path writeTo;
	if (argc >= 4 && std::string(argv[2]) == "--write")
		writeTo = argv[3];
	std::map<std::string, int> counts, errors;
	for (const auto& e : std::filesystem::recursive_directory_iterator(argv[1]))
	{
		if (!e.is_regular_file())
			continue;
		const std::string path = e.path().string();
		if (!KanzenNormalise::Handles(path.c_str()))
			continue;
		std::ifstream f(e.path(), std::ios::binary);
		std::vector<unsigned char> in((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
		std::vector<unsigned char> out;
		std::string error;
		const auto r = KanzenNormalise::Normalise(path.c_str(), in.data(), in.size(), out, error);
		const std::string ext = e.path().extension().string();
		if (r == KanzenNormalise::E_Result_Normalised)
		{
			++counts[ext + " normalised"];
			if (!writeTo.empty())
			{
				const auto dst = writeTo / std::filesystem::relative(e.path(), argv[1]);
				std::filesystem::create_directories(dst.parent_path());
				std::ofstream(dst, std::ios::binary).write((const char*)out.data(), out.size());
			}
		}
		else if (r == KanzenNormalise::E_Result_NotHandled)
			++counts[ext + " not Kanzen-only (served as is)"];
		else
		{
			++counts[ext + " FAILED"];
			if (errors[error]++ < 3)
				std::cout << "FAIL " << path << ": " << error << "\n";
		}
	}
	for (const auto& c : counts)
		std::cout << c.second << "  " << c.first << "\n";
	for (const auto& c : errors)
		std::cout << c.second << " x " << c.first << "\n";
	return errors.empty() ? 0 : 1;
}
