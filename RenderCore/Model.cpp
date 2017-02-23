#include "Model.h"
#include <unordered_map>

namespace rayr
{



Model::OBJLoder::OBJLoder(const wstring &fname)
{
	_wfopen_s(&fp, fname.c_str(), L"r");
	if (fp == nullptr)
		throw std::exception("cannot open file");
}

Model::OBJLoder::~OBJLoder()
{
	if (fp)
		fclose(fp);
}

Model::OBJLoder::TextLine Model::OBJLoder::readLine()
{
	using common::hash_;
	if (fgets(curline, 255, fp) == nullptr)
		return{ "EOF"_hash, 0 };
	const char *end = &curline[strlen(curline)];
	//printf("@@\t%s", curline);

	char prefix[256] = { 0 };
	if (sscanf_s(curline, "%s", prefix, 240) == 0)
		return{ "EMPTY"_hash, 0 };
	const bool isNote = prefix[0] == '#';
	bool inParam = false;
	uint8_t pcnt = 0;
	for (char *cur = &curline[strlen(prefix)]; cur < end; ++cur)
	{
		if (*cur < 0x21 || *cur == 0x7f)//non-graph character
		{
			*cur = '\0';
			inParam = false;
		}
		else if(!inParam)
		{
			param[pcnt++] = cur;
			inParam = true;
			if (isNote)//need only one param
				break;
		}
	}
	return{ hash_(prefix), pcnt };
}

int8_t Model::OBJLoder::parseFloat(const uint8_t idx, float *output)
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

int8_t Model::OBJLoder::parseInt(const uint8_t idx, int32_t *output)
{
	const char *&obj = param[idx];
	int8_t cnt = 0;
	uint8_t len = (uint8_t)strlen(obj);
	int32_t num = 0;
	bool isDummy = false;
	for (uint8_t a = 0; a <= len; ++a)
	{
		if (obj[a] >= '0' && obj[a] <= '9')
		{
			num = num * 10 + (obj[a] - '0');
			isDummy = false;
		}
		else if (!isDummy)//first non-number
		{
			output[cnt++] = num;
			num = 0;
			isDummy = true;
		}
	}
	//if (!isDummy)//first non-number
	//{
	//	output[cnt++] = num;
	//}
	return cnt;
}


struct alignas(uint64_t)PTstub
{
	union
	{
		uint64_t num;
		struct
		{
			uint16_t vid, nid, tid, empty;
		};
	};
	PTstub(int32_t vid_, int32_t nid_, int32_t tid_)
		:vid((uint16_t)vid_), nid((uint16_t)nid_), tid((uint16_t)tid_), empty(0)
	{
	}
	bool operator== (const PTstub& other) const
	{
		return num == other.num;
	}
};
struct PTstubHasher
{
	size_t operator()(const PTstub& p) const
	{
		return p.vid * 33 * 33 + p.nid * 33 + p.tid;
	}
};


void Model::loadOBJ(OBJLoder& ldr)
{
	using miniBLAS::VecI4;
	vector<Vec3> points{ Vec3(0,0,0) };
	vector<Normal> normals{ Normal(0,0,0) };
	vector<Coord2D> texcs{ Coord2D(0,0) };
	std::unordered_map<PTstub, uint32_t, PTstubHasher> idxmap;
	pts.clear();
	indexs.clear();
	Vec3 tmp;
	Coord2D tmpc;
	Vec3 maxv(-10e6, -10e6, -10e6), minv(10e6, 10e6, 10e6);
	VecI4 tmpi, tmpidx;
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
			tmp = Vec3(atof(ldr.param[0]), atof(ldr.param[1]), atof(ldr.param[2]));
			maxv = miniBLAS::max(maxv, tmp);
			minv = miniBLAS::min(minv, tmp);
			points.push_back(tmp);
			break;
		case "vn"_hash://normal
			tmp = Vec3(atof(ldr.param[0]), atof(ldr.param[1]), atof(ldr.param[2]));
			normals.push_back(tmp);
			break;
		case "vt"_hash://texcoord
			tmpc = Coord2D(atof(ldr.param[0]), atof(ldr.param[1]));
			texcs.push_back(tmpc);
			break;
		case "f"_hash://face
			for (uint32_t a = 0; a < line.pcount; ++a)
			{
				ldr.parseInt(a, tmpi);//vert,texc,norm
				PTstub stub(tmpi.x, tmpi.z, tmpi.y);
				const auto it = idxmap.find(stub);
				if (it != idxmap.end())
					tmpidx[a] = it->second;
				else
				{
					pts.push_back(Point{ points[stub.vid],normals[stub.nid], texcs[stub.tid] });
					const uint32_t idx = (uint32_t)(pts.size() - 1);
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
			break;
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
}

Model::Model(const wstring& fname)
{
	static DrawableHelper helper(L"Model");
	helper.InitDrawable(this);
	loadOBJ(OBJLoder(fname));

	vbo.reset(oglu::BufferType::Array);
	//vbo->write(pts.data(), pts.size() * sizeof(Point));
	vbo->write(pts);
	ebo.reset(oglu::BufferType::Element);
	//ebo->write(indexs.data(), indexs.size() * sizeof(uint32_t));
	ebo->write(indexs);
	vao.reset(oglu::VAODrawMode::Triangles);
	vao->setDrawSize(0, (uint32_t)indexs.size());
}

void Model::prepareGL(const GLint(&attrLoc)[3])
{
	vao->prepare().set(vbo, attrLoc, 0)
		.setIndex(ebo, oglu::IndexSize::Int)//index draw
		.end();
}

}