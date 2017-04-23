#pragma once
#include <cstdint>
#include <filesystem>

struct FT_LibraryRec_;
namespace ft
{
namespace fs = std::experimental::filesystem;
class FreeTyper
{
private:
	static void* createLibrary();
	static FT_LibraryRec_& getLibrary();
public:
	FreeTyper(const fs::path& fontpath);
};

}