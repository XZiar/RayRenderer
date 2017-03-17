#include "Model.h"
#include "../3rdParty/stblib/stblib.h"
#include <unordered_map>
#include <set>


namespace rayr
{

union alignas(uint64_t)PTstub
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

namespace inner
{

map<wstring, ModelImage> _ModelImage::images;

ModelImage _ModelImage::getImage(Path picPath, const Path& curPath)
{
	wstring fname = picPath.filename();
	auto img = getImage(fname);
	if (img)
		return img;

	if (!fs::exists(picPath))
	{
		picPath = curPath / fname;
		if (!fs::exists(picPath))
		{
		#ifdef _DEBUG
			printf("@@cannot find file, load image error.\n");
		#endif
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
	catch (const std::ios_base::failure& e)
	{
	#ifdef _DEBUG
		printf("@@invalid image file, load image error.\n");
	#endif
		return img;
	}
}

ModelImage _ModelImage::getImage(const wstring& pname)
{
	const auto it = images.find(pname);
	if (it != images.end())
		return it->second;
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
	image.swap(::stb::resizeImage(image, width, height, w, h));
	width = w, height = h;
}

oglu::oglTexture _ModelImage::genTexture()
{
	auto tex = oglu::oglTexture(oglu::TextureType::Tex2D);
	tex->setProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::Clamp);
	tex->setData(oglu::TextureInnerFormat::BC1A, oglu::TextureDataFormat::RGBA8, width, height, image.data());
	return tex;
}






map<wstring, ModelData> _ModelData::models;

ModelData _ModelData::getModel(const wstring& fname, bool asyncload)
{
	const auto it = models.find(fname);
	if (it != models.end())
	{
		return it->second;
	}
	else
	{
		auto md = new _ModelData(fname, asyncload);
		ModelData m(std::move(md));
		models.insert_or_assign(fname, m);
		return m;
	}
}

void _ModelData::releaseModel(const wstring& fname)
{
	const auto it = models.find(fname);
	if (it != models.end())
	{
		if (it->second.unique())
			models.erase(it);
	}
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

_ModelData::OBJLoder::OBJLoder(const Path &fpath_) :fpath(fpath_)
{
	FILE *fp = nullptr;
	_wfopen_s(&fp, fpath.c_str(), L"rb");
	if (fp == nullptr)
		throw std::ios_base::failure("cannot open file");
	//pre-load data, in case of Acess-Violate while reading file
	fseek(fp, 0, SEEK_END);
	flen = (uint32_t)ftell(fp);
	fdata.resize(flen);
	fseek(fp, 0, SEEK_SET);
	fread(fdata.data(), flen, 1, fp);
	fclose(fp);
	fcurpos = 0;
}

_ModelData::OBJLoder::~OBJLoder()
{
}

_ModelData::OBJLoder::TextLine _ModelData::OBJLoder::readLine()
{
	using common::hash_;
	static std::set<string> specialPrefix{ "mtllib","usemtl","newmtl","g" };
	uint32_t frompos = fcurpos, linelen = 0;
	for (bool isCounting = false; fcurpos < flen;)
	{
		const uint8_t curch = fdata[fcurpos++];
		const bool isLineEnd = (curch == '\r' || curch == '\n');
		if (isLineEnd)
		{
			if (isCounting)//finish line
				break;
			else//not even start
				++frompos;
		}
		else
		{
			++linelen;//linelen EQUALS count how many ch is not "NEWLINE"
			isCounting = true;
		}
	}
	if (linelen == 0)
	{
		if(fcurpos == flen)//EOF
			return{ "EOF"_hash, 0 };
		else
			return{ "EMPTY"_hash, 0 };
	}
	memmove(curline, &fdata[frompos], linelen);
	char *end = &curline[linelen];
	*end = '\0';

	char prefix[256] = { 0 };
	sscanf_s(curline, "%s", prefix, 240);
	const string sprefix(prefix);
	//is-note
	if (sprefix == "#")
	{
		param[0] = linelen > 1 ? &curline[2] : &curline[1];
		return{ "#"_hash, 1 };
	}
	bool isOneParam = specialPrefix.count(sprefix) != 0 || sprefix.find_first_of("map_") == 0;
	bool inParam = false;
	uint8_t pcnt = 0;
	char *cur = &curline[sprefix.length()];
	*cur++ = '\0';
	for (; cur < end; ++cur)
	{
		const uint8_t curch = *(uint8_t*)cur;
		if (curch < uint8_t(0x21) || curch == uint8_t(0x7f))//non-graph character
		{
			*cur = '\0';
			inParam = false;
		}
		else if (!inParam)
		{
			param[pcnt++] = cur;
			inParam = true;
			if (isOneParam)//need only one param
				break;
		}
	}
	return{ hash_(prefix), pcnt };
}

wstring _ModelData::OBJLoder::toWideString(const uint8_t idx)
{
	const auto len = strlen(param[idx]);
	return wstring(param[idx], param[idx] + len);
}


int8_t _ModelData::OBJLoder::parseFloat(const uint8_t idx, float *output)
{
	char *endpos = nullptr;
	int8_t cnt = 0;
	do
	{
		output[cnt++] = strtof(param[idx], &endpos);
	} while (endpos != param[idx]);
	return cnt;
	//return sscanf_s(param[idx], "%f/%f/%f/%f", &output[0], &output[1], &output[2], &output[3]);
}

int8_t _ModelData::OBJLoder::parseInt(const uint8_t idx, int32_t *output)
{
	int8_t cnt = 0;
	for (const char *obj = param[idx] - 1; obj != nullptr; obj = strchr(obj, '/'))
		output[cnt++] = atoi(++obj);
	return cnt;
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
#ifdef _DEBUG
	printf("@@build merged Diffuse texture(%d*%d)\n", maxx, maxy);
#endif
	ModelImage diffuse(maxx, maxy);
	for (const auto& op : opDiffuse)
		diffuse->placeImage(std::get<0>(op), std::get<1>(op), std::get<2>(op));
	opDiffuse.clear();
#ifdef _DEBUG
	printf("@@build merged Normal texture(%d*%d)\n", maxx, maxy);
#endif
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

map<string, inner::_ModelData::MtlStub> _ModelData::loadMTL(const Path& mtlpath) try
{
	using miniBLAS::VecI4;
	OBJLoder ldr(mtlpath);
	printf("@@opened mtl file %ls\n", mtlpath.c_str());
	map<string, MtlStub> mtlmap;
	vector<TexMergeItem> texposs;
	MtlStub *curmtl = nullptr;
	OBJLoder::TextLine line;
	while (line = ldr.readLine())
	{
		switch (line.type)
		{
		case "EMPTY"_hash:
			break;
		case "#"_hash:
		#ifdef _DEBUG
			printf("@@mtl-note\t%s\n", ldr.param[0]);
		#endif
			break;
		case "#merge"_hash:
			{
				auto img = _ModelImage::getImage(ldr.toWideString(0));
				if (!img)
					break;
				int32_t pos[2];
				ldr.parseInt(1, pos);
			#ifdef _DEBUG
				printf("@@mergeMTL\t%s\t%d,%d\n", ldr.param[0], pos[0], pos[1]);
			#endif
				texposs.push_back({ img,static_cast<uint16_t>(pos[0]),static_cast<uint16_t>(pos[1]) });
			}
			break;
		case "newmtl"_hash://vertex
			curmtl = &mtlmap.insert({ string(ldr.param[0]),MtlStub() }).first->second;
			break;
		case "Ka"_hash:
			curmtl->mtl.ambient = Vec3(atof(ldr.param[0]), atof(ldr.param[1]), atof(ldr.param[2]));
			break;
		case "Kd"_hash:
			curmtl->mtl.diffuse = Vec3(atof(ldr.param[0]), atof(ldr.param[1]), atof(ldr.param[2]));
			break;
		case "Ks"_hash:
			curmtl->mtl.specular = Vec3(atof(ldr.param[0]), atof(ldr.param[1]), atof(ldr.param[2]));
			break;
		case "Ke"_hash:
			curmtl->mtl.emission = Vec3(atof(ldr.param[0]), atof(ldr.param[1]), atof(ldr.param[2]));
			break;
		case "Ns"_hash:
			curmtl->mtl.shiness = (float)atof(ldr.param[0]);
			break;
		case "map_Ka"_hash:
			//curmtl.=loadTex(ldr.param[0], mtlpath.parent_path());
			//break;
		case "map_Kd"_hash:
			{
				auto tex = inner::_ModelImage::getImage(ldr.param[0], mtlpath.parent_path());
				curmtl->diffuse() = tex;
				if (tex)
					curmtl->width = std::max(curmtl->width, tex->width), curmtl->height = std::max(curmtl->height, tex->height);
			}break;
		case "map_bump"_hash:
			{
				auto tex = inner::_ModelImage::getImage(ldr.param[0], mtlpath.parent_path());
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
		printf("saving normal texture to %ls\n", outname.c_str());
		::stb::saveImage(outname, normal->image, normal->width, normal->height);
	}
	//::stb::saveImage(L"ONormal.png", normal->image, maxx, maxy);
#endif
	return mtlmap;
}
catch (const std::ios_base::failure& e)
{
#ifdef _DEBUG
	printf("@@cannot open mtl file:%ls\n", mtlpath.c_str());
#endif
	return map<string, MtlStub>();
}


bool _ModelData::loadOBJ(const Path& objpath) try
{
	using miniBLAS::VecI4;
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
	while (line = ldr.readLine())
	{
		switch (line.type)
		{
		case "EMPTY"_hash:
			break;
		case "#"_hash:
		#ifdef _DEBUG
			printf("@@obj-note\t%s\n", ldr.param[0]);
		#endif
			break;
		case "v"_hash://vertex
			{
				Vec3 tmp(atof(ldr.param[0]), atof(ldr.param[1]), atof(ldr.param[2]));
				maxv = miniBLAS::max(maxv, tmp);
				minv = miniBLAS::min(minv, tmp);
				points.push_back(tmp);
			}break;
		case "vn"_hash://normal
			{
				Vec3 tmp(atof(ldr.param[0]), atof(ldr.param[1]), atof(ldr.param[2]));
				normals.push_back(tmp);
			}break;
		case "vt"_hash://texcoord
			{
				Coord2D tmpc(atof(ldr.param[0]), atof(ldr.param[1]));
				tmpc.regulized_mirror();
				texcs.push_back(tmpc);
			}break;
		case "f"_hash://face
			{
				VecI4 tmpi, tmpidx;
				for (uint32_t a = 0; a < line.pcount; ++a)
				{
					ldr.parseInt(a, tmpi);//vert,texc,norm
					PTstub stub(tmpi.x, tmpi.z, tmpi.y, curmtl->posid);
					//printf("===%zd===\n", hasher(stub));
					const auto it = idxmap.find(stub);
					if (it != idxmap.end())
						tmpidx[a] = it->second;
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
				if (line.pcount == 3)
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
				string mtlname(ldr.param[0]);
				groups.push_back({ mtlname,(uint32_t)indexs.size() });
				const auto it = mtlmap.find(mtlname);
				if (it != mtlmap.end())
					curmtl = &it->second;
			}break;
		case "mtllib"_hash://import mtl file
			{
				const auto mtls = loadMTL(objpath.parent_path() / ldr.param[0]);
				for (const auto& mtlp : mtls)
					mtlmap.insert(mtlp);
			}break;
		default:
			break;
		}
	}//END of WHILE
	//printf("%zd,%zd,%f,%f\n", idxmap.bucket_count(), idxmap.max_bucket_count(), idxmap.load_factor(), idxmap.max_load_factor());
	size = maxv - minv;
	printf("@@read %zd vertex, %zd normal, %zd texcoord\n", points.size(), normals.size(), texcs.size());
	printf("@@obj: %zd points, %zd indexs, %zd triangles\n", pts.size(), indexs.size(), indexs.size() / 3);
	printf("@@obj size: %f,%f,%f\n", size.x, size.y, size.z);
	
	return true;
}
catch (const std::ios_base::failure& e)
{
#ifdef _DEBUG
	printf("@@cannot open obj file:%ls\n", objpath.c_str());
#endif
	return false;
}

void _ModelData::initData()
{
	texd = diffuse->genTexture();
	texn = normal->genTexture();
	diffuse.release();
	normal.release();
	vbo.reset(oglu::BufferType::Array);
	vbo->write(pts);
	ebo.reset(oglu::IndexSize::Int);
	ebo->write(indexs);
}

_ModelData::_ModelData(const wstring& fname, bool asyncload) :mfnane(fname)
{
	loadOBJ(mfnane);
	if (asyncload)
	{
		oglu::oglUtil::invokeGL(std::bind(&_ModelData::initData, this)).get();
	}
	else
	{
		initData();
	}
}

_ModelData::~_ModelData()
{

}

//ENDOF INNER
}


Model::Model(const wstring& fname, bool asyncload) :data(inner::_ModelData::getModel(fname, asyncload))
{
	static DrawableHelper helper(L"Model");
	helper.InitDrawable(this);
	const auto resizer = 2 / miniBLAS::max(miniBLAS::max(data->size.x, data->size.y), data->size.z);
	scale = Vec3(resizer, resizer, resizer);
}

Model::~Model()
{
	const auto mfname = data->mfnane;
	data.release();
	inner::_ModelData::releaseModel(mfname);
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