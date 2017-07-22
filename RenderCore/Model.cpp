#include "RenderCoreRely.h"
#include "Model.h"
#include "../3rdParty/stblib/stblib.h"
#include "../3rdParty/uchardetlib/uchardetlib.h"


namespace rayr
{

union alignas(uint64_t) PTstub
{
	uint64_t num;
	struct
	{
		uint32_t numhi, numlow;
	};
	struct
	{
		uint16_t vid, nid, tid, tposid;
	};
	PTstub(int32_t vid_, int32_t nid_, int32_t tid_, uint16_t tposid_)
		:vid((uint16_t)vid_), nid((uint16_t)nid_), tid((uint16_t)tid_), tposid(tposid_)
	{ }
	bool operator== (const PTstub& other) const
	{
		return num == other.num;
	}
	bool operator< (const PTstub& other) const
	{
		return num < other.num;
	}
};

struct PTstubHasher
{
	size_t operator()(const PTstub& p) const
	{
		return p.numhi * 33 + p.numlow;
	}
};

namespace detail
{

map<wstring, ModelImage> _ModelImage::images;

ModelImage _ModelImage::getImage(fs::path picPath, const fs::path& curPath)
{
	const wstring fname = picPath.filename();
	auto img = getImage(fname);
	if (img)
		return img;

	if (!fs::exists(picPath))
	{
		picPath = curPath / fname;
		if (!fs::exists(picPath))
		{
			basLog().error(L"Fail to open image file\t[{}]\n", fname);
			return ModelImage();
		}
	}
	try
	{
		_ModelImage *mi = new _ModelImage(picPath);
		ModelImage image(std::move(mi));
		images.insert_or_assign(fname, image);
		return image;
	}
#pragma warning(disable:4101)
	catch (FileException& fe)
	{
		if(fe.reason == FileException::Reason::ReadFail)
			basLog().error(L"Fail to decode image file\t[{}]\n", picPath.wstring());
		else
			basLog().error(L"Cannot find image file\t[{}]\n", picPath.wstring());
		return img;
	}
#pragma warning(default:4101)
}

ModelImage _ModelImage::getImage(const wstring& pname)
{
	if (auto img = findmap(images, pname))
		return **img;
	else
		return ModelImage();
}

void _ModelImage::shrink()
{
	auto it = images.cbegin();
	while (it != images.end())
	{
		if (it->second.unique())
			it = images.erase(it);
		else
			++it;
	}
}

_ModelImage::_ModelImage(const wstring& pfname)
{
	int32_t w, h;
	std::tie(w, h) = ::stb::loadImage(pfname, image);
	width = static_cast<uint16_t>(w), height = static_cast<uint16_t>(h);
}

_ModelImage::_ModelImage(const uint16_t w, const uint16_t h, const uint32_t color) :width(w), height(h)
{
	image.resize(width*height, color);
}

void _ModelImage::placeImage(const Wrapper<_ModelImage>& from, const uint16_t x, const uint16_t y)
{
	if (!from)
		return;
	const auto w = from->width;
	const size_t lim = from->image.size();
	for (size_t posFrom = 0, posTo = y*width + x; posFrom < lim; posFrom += w, posTo += width)
		memmove(&image[posTo], &from->image[posFrom], sizeof(uint32_t)*w);
}

void _ModelImage::resize(const uint16_t w, const uint16_t h)
{
	if (width == w || height == h)
		return;
	stb::resizeImage(image, width, height, w, h).swap(image);
	width = w, height = h;
}

oglu::oglTexture _ModelImage::genTexture()
{
	auto tex = oglu::oglTexture(oglu::TextureType::Tex2D);
	tex->setProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::Clamp);
	tex->setData(oglu::TextureInnerFormat::BC1A, oglu::TextureDataFormat::RGBA8, width, height, image.data());
	return tex;
}

void _ModelImage::CompressData(vector<uint8_t>& output)
{
	auto tex = genTexture();
	if (auto dat = tex->getCompressedData())
		output = std::move(*dat);
}

oglu::oglTexture _ModelImage::genTextureAsync()
{
	vector<uint8_t> texdata;
	oglu::oglUtil::invokeAsyncGL(std::bind(&_ModelImage::CompressData, this, std::ref(texdata)))->wait();
	auto tex = oglu::oglTexture(oglu::TextureType::Tex2D);
	tex->setProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::Clamp);
	tex->setCompressedData(oglu::TextureInnerFormat::BC1A, width, height, texdata);
	return tex;
}


class OBJLoder
{
private:
	static std::set<string> specialPrefix;
	fs::path FilePath;
	Charset chset;
	vector<uint8_t> Content;
	size_t CurPos, Length;
public:
	struct TextLine
	{
		uint64_t Type;
		std::string_view Line;
		vector<std::string_view> Params;
		Charset charset;

		TextLine() {}

		TextLine(const Charset chset, const string& prefix) : charset(chset), Type(hash_(prefix)) {}

		template<size_t N>
		TextLine(const Charset chset, const char(&prefix)[N] = "EMPTY") : charset(chset), Type(hash_(prefix)) {}

		TextLine(const Charset chset, const std::string_view& line) : charset(chset), Line(line) { Params.reserve(8); }

		TextLine(const TextLine& other) = default;
		TextLine(TextLine&& other) = default;
		TextLine& operator =(const TextLine& other) = default;
		TextLine& operator =(TextLine&& other) = default;

		template<typename T>
		void SetType(const T& prefix) { Type = hash_(prefix); }

		std::string_view Rest(const size_t fromIndex = 1) 
		{
			if (Params.size() <= fromIndex)
				return {};
			const auto lenTotal = Line.length();
			const auto beginLine = Line.data(), beginRest = Params[fromIndex].data();
			return std::string_view(beginRest, lenTotal - (beginRest - beginLine));
		}

		wstring GetWString(const size_t index)
		{
			if (Params.size() <= index)
				return L"";
			return to_wstring(Params[index], charset);
		}

		wstring ToWString() { return to_wstring(Line, charset); }

		template<size_t N>
		int8_t ParseInts(const uint8_t idx, int32_t(&output)[N])
		{
			int8_t cnt = 0;
			/*
			const auto chBegin = Params[idx].cbegin(), chEnd = Params[idx].cend();
			bool isInNumber = false;
			int32_t num = 0;
			while (chBegin != chEnd && cnt < N)
			{
				auto ch = *chBegin++;
				if (ch < '0' || ch > '9')
				{
					if (isInNumber)
					{
						output[cnt++] = num;
						num = 0;
						isInNumber = false;
					}
				}
				else
				{
					isInNumber = true;
					num = num * 10 + (ch - '0');
				}
			}
			return cnt;
			*/
			str::SplitAndDo(Params[idx], '/', [&cnt, &output](const char *substr, const size_t len)
			{
				if (cnt < N)
				{
					if (len == 0)
						output[cnt++] = 0;
					else
						output[cnt++] = atoi(substr);
				}
			});
			return cnt;
		}

		template<size_t N>
		int8_t ParseFloats(const uint8_t idx, float(&output)[N])
		{
			str::SplitAndDo(Params[idx], '/', [&cnt, &output](const char *substr, const size_t len)
			{
				if (cnt < N)
				{
					if (len == 0)
						output[cnt++] = 0;
					else
						output[cnt++] = atof(Params[idx]);
				}
			});
		}

		operator const bool() const
		{
			return Type != "EOF"_hash;
		}
	};

	OBJLoder(const fs::path& fpath_) : FilePath(fpath_)
	{
		//pre-load data, in case of Acess-Violate while reading file
		Content = file::readAll(FilePath);
		Length = Content.size() - 1;
		CurPos = 0;
		chset = uchdet::detectEncoding(Content);
		basLog().debug(L"obj file[{}]--encoding[{}]\n", FilePath.wstring(), getCharsetWName(chset));
	}

	TextLine ReadLine()
	{
		using common::hash_;
		using std::string_view;
		size_t fromPos = CurPos, lineLength = 0;
		for (bool isInLine = false; CurPos < Length;)
		{
			const uint8_t curChar = Content[CurPos++];
			const bool isLineEnd = (curChar == '\r' || curChar == '\n');
			if (isLineEnd)
			{
				if (isInLine)//finish this line
					break;
				else
					++fromPos;
			}
			else
			{
				++lineLength;//linelen EQUALS count how many ch is not "NEWLINE"
				isInLine = true;
			}
		}
		if (lineLength == 0)
		{
			if (CurPos == Length)//EOF
				return { chset, "EOF" };
			else
				return { chset, "EMPTY" };
		}
		TextLine textLine(chset, string_view((const char*)&Content[fromPos], lineLength));

		str::split(textLine.Line, [](const char ch) 
		{ 
			return (uint8_t)(ch) < uint8_t(0x21) || (uint8_t)(ch) == uint8_t(0x7f);//non-graph character
		}, textLine.Params, false);
		if (textLine.Params.size() == 0)
			return { chset, "EMPTY" };

		textLine.SetType(textLine.Params[0]);
		return textLine;
	}
};
std::set<string> OBJLoder::specialPrefix = { "mtllib","usemtl","newmtl","g" };

map<wstring, ModelData> _ModelData::models;

ModelData _ModelData::getModel(const wstring& fname, bool asyncload)
{
	if (auto md = findmap(models, fname))
		return **md;
	auto md = new _ModelData(fname, asyncload);
	ModelData m(std::move(md));
	models.insert_or_assign(fname, m);
	return m;
}

void _ModelData::releaseModel(const wstring& fname)
{
	if (auto md = findmap(models, fname))
		if ((**md).unique())
			models.erase(fname);
}

oglu::oglVAO _ModelData::getVAO() const
{
	oglu::oglVAO vao(oglu::VAODrawMode::Triangles);
	if (groups.empty())
		vao->setDrawSize(0, (uint32_t)indexs.size());
	else
	{
		vector<uint32_t> offs, sizs;
		uint32_t last = 0;
		for (const auto& g : groups)
		{
			if (!offs.empty())
				sizs.push_back(g.second - last);
			offs.push_back(last = g.second);
		}
		sizs.push_back(static_cast<uint32_t>(indexs.size() - last));
		vao->setDrawSize(offs, sizs);
	}
	return vao;
}


std::tuple<ModelImage, ModelImage> _ModelData::mergeTex(map<string, MtlStub>& mtlmap, vector<TexMergeItem>& texposs)
{
	//Resize Texture
	for (auto& mp : mtlmap)
	{
		auto& mtl = mp.second;
		for (auto& tex : mtl.texs)
		{
			if (tex && (tex->width < mtl.width || tex->height < mtl.height))
				tex->resize(mtl.width, mtl.height);
		}
	}
	//Merge Textures
	vector<std::tuple<ModelImage, uint16_t, uint16_t>> opDiffuse, opNormal;
	uint16_t maxx = 0, maxy = 0;
	for (uint16_t idx = 0; idx < texposs.size(); ++idx)
	{
		ModelImage img;
		uint16_t x, y;
		std::tie(img, x, y) = texposs[idx];
		maxx = std::max(maxx, static_cast<uint16_t>(img->width + x));
		maxy = std::max(maxy, static_cast<uint16_t>(img->height + y));
		bool addedDiffuse = false, addedNormal = false;
		for (auto& mp : mtlmap)
		{
			auto& mtl = mp.second;
			if (mtl.hasImage(img))
			{
				mtl.sx = x, mtl.sy = y, mtl.posid = idx;
				if (!addedDiffuse && mtl.diffuse())
				{
					opDiffuse.push_back({ mtl.diffuse(),x,y });
					addedDiffuse = true;
				}
				mtl.diffuse().release();
				if (!addedNormal && mtl.normal())
				{
					opNormal.push_back({ mtl.normal(),x,y });
					addedNormal = true;
				}
				mtl.normal().release();
			}
		}
		//ENDOF setting mtl-position AND preparing opSequence
	}
	texposs.clear();
	basLog().verbose(L"Build merged Diffuse texture({}*{})\n", maxx, maxy);
	ModelImage diffuse(maxx, maxy);
	for (const auto& op : opDiffuse)
		diffuse->placeImage(std::get<0>(op), std::get<1>(op), std::get<2>(op));
	opDiffuse.clear();
	basLog().verbose(L"Build merged Normal texture({}*{})\n", maxx, maxy);
	ModelImage normal(maxx, maxy);
	for (const auto& op : opNormal)
		normal->placeImage(std::get<0>(op), std::get<1>(op), std::get<2>(op));
	opNormal.clear();

	for (auto& mp : mtlmap)
	{
		auto& mtl = mp.second;
		//texture's uv coordinate system has reversed y-axis
		//hence y'=(1-y)*scale+offset = -scale*y + (scale+offset)
		mtl.scalex = mtl.width*1.0f / maxx, mtl.scaley = mtl.height*-1.0f / maxy;
		mtl.offsetx = mtl.sx*1.0f / maxx, mtl.offsety = mtl.sy*1.0f / maxy - mtl.scaley;
	}
	_ModelImage::shrink();
	return { diffuse,normal };
}

map<string, detail::_ModelData::MtlStub> _ModelData::loadMTL(const fs::path& mtlpath) try
{
	using miniBLAS::VecI4;
	OBJLoder ldr(mtlpath);
	basLog().verbose(L"Parsing mtl file [{}]\n", mtlpath.wstring());
	map<string, MtlStub> mtlmap;
	vector<TexMergeItem> texposs;
	MtlStub *curmtl = nullptr;
	OBJLoder::TextLine line;
	while (line = ldr.ReadLine())
	{
		switch (line.Type)
		{
		case "EMPTY"_hash:
			break;
		case "#"_hash:
			basLog().verbose(L"--mtl-note [{}]\n", line.ToWString());
			break;
		case "#merge"_hash:
			{
				auto img = _ModelImage::getImage(line.GetWString(1));
				if (!img)
					break;
				int32_t pos[2];
				line.ParseInts(2, pos);
				basLog().verbose(L"--mergeMTL [{}]--[{},{}]\n", to_wstring(line.Params[1]), pos[0], pos[1]);
				texposs.push_back({ img,static_cast<uint16_t>(pos[0]),static_cast<uint16_t>(pos[1]) });
			}
			break;
		case "newmtl"_hash://vertex
			curmtl = &mtlmap.insert({ string(line.Params[1]),MtlStub() }).first->second;
			break;
		case "Ka"_hash:
			curmtl->mtl.ambient = Vec3(atof(line.Params[1].data()), atof(line.Params[2].data()), atof(line.Params[3].data()));
			break;
		case "Kd"_hash:
			curmtl->mtl.diffuse = Vec3(atof(line.Params[1].data()), atof(line.Params[2].data()), atof(line.Params[3].data()));
			break;
		case "Ks"_hash:
			curmtl->mtl.specular = Vec3(atof(line.Params[1].data()), atof(line.Params[2].data()), atof(line.Params[3].data()));
			break;
		case "Ke"_hash:
			curmtl->mtl.emission = Vec3(atof(line.Params[1].data()), atof(line.Params[2].data()), atof(line.Params[3].data()));
			break;
		case "Ns"_hash:
			curmtl->mtl.shiness = (float)atof(line.Params[1].data());
			break;
		case "map_Ka"_hash:
			//curmtl.=loadTex(ldr.param[0], mtlpath.parent_path());
			//break;
		case "map_Kd"_hash:
			{
				auto tex = detail::_ModelImage::getImage(to_wstring(line.Rest(1)), mtlpath.parent_path());
				curmtl->diffuse() = tex;
				if (tex)
					curmtl->width = std::max(curmtl->width, tex->width), curmtl->height = std::max(curmtl->height, tex->height);
			}break;
		case "map_bump"_hash:
			{
				auto tex = detail::_ModelImage::getImage(to_wstring(line.Rest(1)), mtlpath.parent_path());
				curmtl->normal() = tex;
				if (tex)
					curmtl->width = std::max(curmtl->width, tex->width), curmtl->height = std::max(curmtl->height, tex->height);
			}break;
		}
	}

	std::tie(diffuse, normal) = mergeTex(mtlmap, texposs);
#if !defined(_DEBUG) && 0
	{
		auto outname = mtlpath.parent_path() / (mtlpath.stem().wstring() + L"_Normal.png");
		basLog().info(L"Saving Normal texture to [{}]...\n", outname.wstring());
		::stb::saveImage(outname, normal->image, normal->width, normal->height);
	}
	//::stb::saveImage(L"ONormal.png", normal->image, maxx, maxy);
#endif
	return mtlmap;
}
#pragma warning(disable:4101)
catch (FileException& fe)
{
	basLog().error(L"Fail to open mtl file\t[{}]\n", mtlpath.wstring());
	return map<string, MtlStub>();
}
#pragma warning(default:4101)


void _ModelData::loadOBJ(const fs::path& objpath) try
{
	using miniBLAS::VecI4;
	{
		OBJLoder ldrEx(objpath);
		auto tmpLine = ldrEx.ReadLine();
		tmpLine = ldrEx.ReadLine();
		string rest = string(tmpLine.Rest());
	}
	OBJLoder ldr(objpath);
	vector<Vec3> points{ Vec3(0,0,0) };
	vector<Normal> normals{ Normal(0,0,0) };
	vector<Coord2D> texcs{ Coord2D(0,0) };
	map<string, MtlStub> mtlmap;
	std::unordered_map<PTstub, uint32_t, PTstubHasher> idxmap;
	idxmap.reserve(32768);
	pts.clear();
	indexs.clear();
	groups.clear();
	Vec3 maxv(-10e6, -10e6, -10e6), minv(10e6, 10e6, 10e6);
	VecI4 tmpi, tmpidx;
	MtlStub tmpmtl;
	MtlStub *curmtl = &tmpmtl;
	OBJLoder::TextLine line;
	SimpleTimer tstTimer;
	while (line = ldr.ReadLine())
	{
		switch (line.Type)
		{
		case "EMPTY"_hash:
			break;
		case "#"_hash:
			basLog().verbose(L"--obj-note [{}]\n", line.ToWString());
			break;
		case "v"_hash://vertex
			{
				Vec3 tmp(atof(line.Params[1].data()), atof(line.Params[2].data()), atof(line.Params[3].data()));
				maxv = miniBLAS::max(maxv, tmp);
				minv = miniBLAS::min(minv, tmp);
				points.push_back(tmp);
			}break;
		case "vn"_hash://normal
			{
				Vec3 tmp(atof(line.Params[1].data()), atof(line.Params[2].data()), atof(line.Params[3].data()));
				normals.push_back(tmp);
			}break;
		case "vt"_hash://texcoord
			{
				Coord2D tmpc(atof(line.Params[1].data()), atof(line.Params[2].data()));
				tmpc.regulized_mirror();
				texcs.push_back(tmpc);
			}break;
		case "f"_hash://face
			{
				VecI4 tmpi, tmpidx;
				const auto lim = min((size_t)4, line.Params.size() - 1);
				if (lim < 3)
					basLog().warning(L"too few params for face : {}", to_wstring(line.Line));
				for (uint32_t a = 0; a < lim; ++a)
				{
					line.ParseInts(a + 1, tmpi.raw());//vert,texc,norm
					PTstub stub(tmpi.x, tmpi.z, tmpi.y, curmtl->posid);
					if (auto oidx = findmap(idxmap, stub))
						tmpidx[a] = **oidx;
					else
					{
						const uint32_t idx = static_cast<uint32_t>(pts.size());
						pts.push_back(Point(points[stub.vid], normals[stub.nid],
							//texcs[stub.tid]));
							texcs[stub.tid].repos(curmtl->scalex, curmtl->scaley, curmtl->offsetx, curmtl->offsety)));
						idxmap.insert_or_assign(stub, idx);
						tmpidx[a] = idx;
					}
				}
				if (lim == 3)
				{
					indexs.push_back(tmpidx.x);
					indexs.push_back(tmpidx.y);
					indexs.push_back(tmpidx.z);
				}
				else//4 vertex-> 3 vertex
				{
					indexs.push_back(tmpidx.x);
					indexs.push_back(tmpidx.y);
					indexs.push_back(tmpidx.z);
					indexs.push_back(tmpidx.x);
					indexs.push_back(tmpidx.z);
					indexs.push_back(tmpidx.w);
				}
			}break;
		case "usemtl"_hash://each mtl is a group
			{
				string mtlName(line.Rest(1));
				groups.push_back({ mtlName,(uint32_t)indexs.size() });
				if (groups.size() == 1)
					tstTimer.Start();
				if (auto omtl = findmap(mtlmap, mtlName))
					curmtl = &**omtl;
			}break;
		case "mtllib"_hash://import mtl file
			{
				const auto mtls = loadMTL(objpath.parent_path() / string(line.Rest(1)));
				for (const auto& mtlp : mtls)
					mtlmap.insert(mtlp);
			}break;
		default:
			break;
		}
	}//END of WHILE
	tstTimer.Stop();
	size = maxv - minv;
	basLog().success(L"read {} vertex, {} normal, {} texcoord\n", points.size(), normals.size(), texcs.size());
	basLog().success(L"OBJ:\t{} points, {} indexs, {} triangles\n", pts.size(), indexs.size(), indexs.size() / 3);
	basLog().info(L"OBJ size:\t [{},{},{}]\n", size.x, size.y, size.z);
	basLog().debug(L"index-resize cost {} us\n", tstTimer.ElapseUs());
}
#pragma warning(disable:4101)
catch (const FileException& fe)
{
	basLog().error(L"Fail to open obj file\t[{}]\n", objpath.wstring());
	COMMON_THROW(BaseException, L"fail to load model data");
}
#pragma warning(default:4101)

void _ModelData::initData()
{
	texd = diffuse->genTextureAsync();
	texn = normal->genTextureAsync();
	diffuse.release();
	normal.release();
	vbo.reset(oglu::BufferType::Array);
	vbo->write(pts);
	ebo.reset();
	ebo->writeCompat(indexs);
}

_ModelData::_ModelData(const wstring& fname, bool asyncload) :mfnane(fname)
{
	loadOBJ(mfnane);
	if (asyncload)
	{
		auto task = oglu::oglUtil::invokeSyncGL(std::bind(&_ModelData::initData, this));
		task->wait();
	}
	else
	{
		initData();
	}
}

_ModelData::~_ModelData()
{

}

//ENDOF detail
}


Model::Model(const wstring& fname, bool asyncload) : Drawable(TYPENAME), data(detail::_ModelData::getModel(fname, asyncload))
{
	const auto resizer = 2 / max(max(data->size.x, data->size.y), data->size.z);
	scale = Vec3(resizer, resizer, resizer);
}

Model::~Model()
{
	const auto mfname = data->mfnane;
	data.release();
	detail::_ModelData::releaseModel(mfname);
}

void Model::prepareGL(const oglu::oglProgram& prog, const map<string, string>& translator)
{
	auto vao = data->getVAO();
	defaultBind(prog, vao, data->vbo)
		.setIndex(data->ebo)//index draw
		.end();
	setVAO(prog, vao);
}

void Model::draw(oglu::oglProgram & prog) const
{
	drawPosition(prog).setTexture(data->texd, "tex").draw(getVAO(prog)).end();
}

}