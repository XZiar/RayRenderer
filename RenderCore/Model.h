#pragma once

#include "RenderElement.h"
#include <filesystem>

namespace rayr
{

class Model;
namespace inner
{

namespace fs = std::experimental::filesystem;

class _ModelImage
{
	friend class ::rayr::Model;
	friend class _ModelData;
private:
	using Path = fs::path;
	static map<wstring, Wrapper<_ModelImage, false>> images;
	static Wrapper<_ModelImage, false> getImage(Path picPath, const Path& curPath);
	static void shrink();
	uint16_t width = 0, height = 0;
	vector<uint32_t> image;
	_ModelImage(const wstring& pfname);
public:
};
using ModelImage = Wrapper<_ModelImage, false>;

class _ModelData
{
	friend class ::rayr::Model;
private:
	using Path = std::experimental::filesystem::path;
	static map<wstring, Wrapper<_ModelData, false>> models;
	static Wrapper<_ModelData, false> getModel(const wstring& fname);
	static void releaseModel(const wstring& fname);
	class OBJLoder
	{
		Path fpath;
		FILE *fp = nullptr;
	public:
		char curline[256];
		const char* param[5];
		struct TextLine
		{
			uint64_t type;
			uint8_t pcount;
			operator const bool() const
			{
				return type != "EOF"_hash;
			}
		};
		OBJLoder(const Path &fpath_);
		~OBJLoder();
		TextLine readLine();
		string popString();
		int8_t parseFloat(const uint8_t idx, float *output);
		int8_t parseInt(const uint8_t idx, int32_t *output);
	};
	vector<Point> pts;
	vector<uint32_t> indexs;
	vector<std::pair<string, uint32_t>> groups;
	oglu::oglBuffer vbo;
	oglu::oglEBO ebo;
	const wstring mfnane;
	bool loadMTL(const Path& mtlfname);
	bool loadOBJ(const Path& objfname);
	_ModelData(const wstring& fname);
public:
	~_ModelData();
	oglu::oglVAO getVAO() const;
};

}

using ModelData = Wrapper<inner::_ModelData, false>;

class alignas(16) Model : public Drawable
{
protected:
	ModelData data;
public:
	Model(const wstring& fname);
	~Model();
	virtual void prepareGL(const oglu::oglProgram& prog, const map<string, string>& translator = map<string, string>()) override;
};

}