#include "Model.h"
#include "../3rdParty/stblib/stblib.h"
#include <unordered_map>


namespace rayr
{


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
		if (it->second.refCount() == 1)
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

void _ModelImage::placeImage(const Wrapper<_ModelImage, false>& from, const uint16_t x, const uint16_t y)
{
	if (!from)
		return;
	const auto w = from->width;
	const size_t lim = from->image.size();
	for (size_t posf = 0, post = y*width + x; posf < lim; posf += w, post += width)
		memmove(&image[post], &from->image[posf], sizeof(uint32_t)*w);
}

oglu::oglTexture _ModelImage::genTexture()
{
	auto tex = oglu::oglTexture(oglu::TextureType::Tex2D);
	tex->setProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::Clamp);
	tex->setData(oglu::TextureFormat::RGBA, width, height, image.data());
	return tex;
}






map<wstring, ModelData> _ModelData::models;

ModelData _ModelData::getModel(const wstring& fname)
{
	const auto it = models.find(fname);
	if (it != models.end())
	{
		return it->second;
	}
	else
	{
		auto md = new _ModelData(fname);
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
		if (it->second.refCount() == 1)
			models.erase(it);
	}
}

_ModelData::_ModelData(const wstring& fname) :mfnane(fname)
{
	loadOBJ(fname);

	vbo.reset(oglu::BufferType::Array);
	vbo->write(pts);
	ebo.reset(oglu::IndexSize::Int);
	ebo->write(indexs);
}

_ModelData::~_ModelData()
{

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
	_wfopen_s(&fp, fpath.c_str(), L"r");
	if (fp == nullptr)
		throw std::ios_base::failure("cannot open file");
}

_ModelData::OBJLoder::~OBJLoder()
{
	if (fp)
		fclose(fp);
}

_ModelData::OBJLoder::TextLine _ModelData::OBJLoder::readLine()
{
	using common::hash_;
	if (fgets(curline, 255, fp) == nullptr)
		return{ "EOF"_hash, 0 };
	const char *end = &curline[strlen(curline)];

	char prefix[256] = { 0 };
	if (sscanf_s(curline, "%s", prefix, 240) == 0)
		return{ "EMPTY"_hash, 0 };
	const bool isNote = prefix[0] == '#';
	bool inParam = false;
	uint8_t pcnt = 0;
	char *cur = &curline[strlen(prefix)];
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
			if (isNote)//need only one param
				break;
		}
	}
	return{ isNote ? "#"_hash : hash_(prefix), pcnt };
}

string _ModelData::OBJLoder::popString()
{
	const auto bakptr = param[0];
	const size_t len = strlen(param[0]);
	size_t objlen = 0;
	bool inParam = true;
	for (size_t a=0; a < len; ++a)
	{
		const uint8_t curch = *(uint8_t*)&param[0][a];
		if (curch < uint8_t(0x21) || curch == uint8_t(0x7f))//non-graph character
		{
			objlen = a;
			inParam = false;
		}
		else if (!inParam)
		{
			param[0] = &param[0][a];
			break;
		}
	}
	return string(bakptr, objlen);
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



map<string, inner::_ModelData::MtlStub> _ModelData::loadMTL(const Path& mtlpath) try
{
	using miniBLAS::VecI4;
	OBJLoder ldr(mtlpath);
	printf("@@opened mtl file %ls\n", mtlpath.c_str());
	map<string, MtlStub> mtlmap;
	vector<std::tuple<ModelImage, uint16_t, uint16_t>> texposs;
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
			printf("@@mtl-note\t%s", ldr.param[0]);
		#endif
			if (strlen(ldr.curline) > 1)//has extra information
			{
				switch (hash_(&ldr.curline[1]))
				{
				case "merge"_hash:
					{
						string mname = ldr.popString();
						auto img = _ModelImage::getImage(wstring(mname.begin(), mname.end()));
						if (!img)
							break;
						int32_t pos[2];
						ldr.parseInt(0, pos);
						texposs.push_back({ img,static_cast<uint16_t>(pos[0]),static_cast<uint16_t>(pos[1]) });
					}
					break;
				}
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
			curmtl->diffuse = inner::_ModelImage::getImage(ldr.param[0], mtlpath.parent_path());
			break;
		case "bump"_hash:
		case "map_bump"_hash:
			curmtl->normal = inner::_ModelImage::getImage(ldr.param[0], mtlpath.parent_path());
			break;
		}
	}
	//Merge Textures
	ModelImage img;
	uint16_t x, y, maxx = 0, maxy = 0;
	for (uint16_t idx = 0; idx < texposs.size(); ++idx)
	{
		std::tie(img, x, y) = texposs[idx];
		maxx = std::max(maxx, static_cast<uint16_t>(img->width + x));
		maxy = std::max(maxy, static_cast<uint16_t>(img->height + y));
		std::for_each(mtlmap.begin(), mtlmap.end(), [&](auto& mp)
		{
			if (mp.second.hasImage(img))
				mp.second.sx = x, mp.second.sy = y, mp.second.posid = idx;
		});
	}
	texposs.clear();
#ifdef _DEBUG
	printf("@@build merged texture(%d*%d)\n", maxx, maxy);
#endif
	ModelImage diffuse(maxx, maxy), normal(maxx,maxy);
	for (auto& mp : mtlmap)
	{
		auto& mtl = mp.second;
		diffuse->placeImage(mtl.diffuse, mtl.sx, mtl.sy);
		normal->placeImage(mtl.normal, mtl.sx, mtl.sy);
		//texture's uv coordinate system has reversed y-axis
		//hence y'=(1-y)*scale+offset = -scale*y + (scale+offset)
		mtl.scalex = mtl.diffuse->width*1.0f / maxx, mtl.scaley = mtl.diffuse->height*-1.0f / maxy;
		mtl.offsetx = mtl.sx*1.0f / maxx, mtl.offsety = mtl.sy*1.0f / maxy - mtl.scaley;
		mtl.diffuse = mtl.normal = ModelImage();
	}
	_ModelImage::shrink();
	//::stb::saveImage(L"ODiffuse.png", diffuse->image, maxx, maxy);
	//::stb::saveImage(L"ONormal.png", normal->image, maxx, maxy);
	texd = diffuse->genTexture();
	texn = normal->genTexture();
	return mtlmap;
}
catch (const std::ios_base::failure& e)
{
#ifdef _DEBUG
	printf("@@cannot open mtl file:%ls\n", mtlpath.c_str());
#endif
	return map<string, MtlStub>();
}


struct alignas(uint64_t) PTstub
{
	uint16_t vid, nid, tid, tposid;
	PTstub(int32_t vid_, int32_t nid_, int32_t tid_, uint16_t tposid_)
		:vid((uint16_t)vid_), nid((uint16_t)nid_), tid((uint16_t)tid_), tposid(tposid_)
	{
	}
	bool operator== (const PTstub& other) const
	{
		return *(uint64_t*)this == *(uint64_t*)&other;
	}
	bool operator< (const PTstub& other) const
	{
		return *(uint64_t*)this < *(uint64_t*)&other;
	}
};
struct PTstubHasher
{
	size_t operator()(const PTstub& p) const
	{
		return p.vid * 33 * 33 + p.nid * 33 + p.tid;
	}
};


bool _ModelData::loadOBJ(const Path& objpath) try
{
	using miniBLAS::VecI4;
	OBJLoder ldr(objpath);
	vector<Vec3> points{ Vec3(0,0,0) };
	vector<Normal> normals{ Normal(0,0,0) };
	vector<Coord2D> texcs{ Coord2D(0,0) };
	map<string, MtlStub> mtlmap;
	//std::unordered_map<PTstub, uint32_t, PTstubHasher> idxmap;
	std::map<PTstub, uint32_t> idxmap;
	pts.clear();
	indexs.clear();
	groups.clear();
	Vec3 maxv(-10e6, -10e6, -10e6), minv(10e6, 10e6, 10e6);
	VecI4 tmpi, tmpidx;
	MtlStub curmtl;
	OBJLoder::TextLine line;
	while (line = ldr.readLine())
	{
		switch (line.type)
		{
		case "EMPTY"_hash:
			break;
		case "#"_hash:
		#ifdef _DEBUG
			printf("@@obj-note\t%s", ldr.param[0]);
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
				texcs.push_back(tmpc);
			}break;
		case "f"_hash://face
			{
				VecI4 tmpi, tmpidx;
				for (uint32_t a = 0; a < line.pcount; ++a)
				{
					ldr.parseInt(a, tmpi);//vert,texc,norm
					PTstub stub(tmpi.x, tmpi.z, tmpi.y, curmtl.posid);
					const auto it = idxmap.find(stub);
					if (it != idxmap.end())
						tmpidx[a] = it->second;
					else
					{
						const uint32_t idx = static_cast<uint32_t>(pts.size());
						pts.push_back(Point(points[stub.vid], normals[stub.nid],
							//texcs[stub.tid]));
							texcs[stub.tid].repos(curmtl.scalex, curmtl.scaley, curmtl.offsetx, curmtl.offsety)));
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
					curmtl = it->second;
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
	const auto sizev = maxv - minv;
	printf("@@read %zd vertex, %zd normal, %zd texcoord\n", points.size(), normals.size(), texcs.size());
	printf("@@obj: %zd points, %zd indexs, %zd triangles\n", pts.size(), indexs.size(), indexs.size() / 3);
	printf("@@obj size: %f,%f,%f\n", sizev.x, sizev.y, sizev.z);
	const auto resizer = 2 / miniBLAS::max(miniBLAS::max(sizev.x, sizev.y), sizev.z);
	for (auto& p : pts)
		p.pos *= resizer;
	firstcnt = groups[1].second;
	return true;
}
catch (const std::ios_base::failure& e)
{
#ifdef _DEBUG
	printf("@@cannot open obj file:%ls\n", objpath.c_str());
#endif
	return false;
}

//ENDOF INNER
}


Model::Model(const wstring& fname) :data(inner::_ModelData::getModel(fname))
{
	static DrawableHelper helper(L"Model");
	helper.InitDrawable(this);
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
	prog->draw(getModelMat()).setTexture(data->texd, "tex").draw(getVAO(prog));
}

}