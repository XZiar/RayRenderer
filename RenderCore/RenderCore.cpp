#include "resource.h"
#include "RenderCore.h"
#include "ResourceHelper.h"

namespace rayr
{

static string getShaderFromDLL(int32_t id)
{
	std::vector<uint8_t> data;
	if (ResourceHelper::getData(data, L"SHADER", id) != ResourceHelper::Result::Success)
		return "";
	data.push_back('\0');
	return std::string((const char*)data.data());
}


struct Init
{
	Init()
	{
		oglUtil::init();
	}
};


void BasicTest::init2d(const string pname)
{
	prog2D.reset();
	if(pname == "")
	{
		oglShader vert(ShaderType::Vertex, getShaderFromDLL(IDR_SHADER_2DVERT));
		auto ret = vert->compile();
		if (ret)
			prog2D->addShader(std::move(vert));
		else
		{
			printf("ERROR on Vertex Shader Compiler:\n%s\n", ret.msg.c_str());
			getchar();
		}
		oglShader frag(ShaderType::Fragment, getShaderFromDLL(IDR_SHADER_2DFRAG));
		ret = frag->compile();
		if (ret)
			prog2D->addShader(std::move(frag));
		else
		{
			printf("ERROR on Fragment Shader Compiler:\n%s\n", ret.msg.c_str());
			getchar();
		}
	}
	else
	{
		auto ret = oglUtil::loadShader(prog2D, pname);
		if (!ret)
		{
			printf("%s\n%s\n", ret.msg.c_str(), ret.data.c_str());
			getchar();
		}
	}
	{
		auto ret = prog2D->link();
		if (!ret)
		{
			printf("ERROR on Program Linker:\n%s\n", ret.msg.c_str());
			getchar();
		}
	}
	picVAO.reset(VAODrawMode::Triangles);
	screenBox.reset(BufferType::Array);
	{
		Vec3 DatVert[] = { { -1.0f, -1.0f, 0.0f },{ 1.0f, -1.0f, 0.0f },{ -1.0f, 1.0f, 0.0f },
		{ 1.0f, 1.0f, 0.0f },{ -1.0f, 1.0f, 0.0f },{ 1.0f, -1.0f, 0.0f } };
		screenBox->write(DatVert, sizeof(DatVert));
		picVAO->prepare().set(screenBox, prog2D->Attr_Vert_Pos, sizeof(Vec3), 3, 0).end();
	}
}

void BasicTest::init3d(const string pname)
{
	prog3D.reset();
	if (pname == "")
	{
		oglShader vert(ShaderType::Vertex, getShaderFromDLL(IDR_SHADER_3DVERT));
		auto ret = vert->compile();
		if (ret)
			prog3D->addShader(std::move(vert));
		else
		{
			printf("ERROR on Vertex Shader Compiler:\n%s\n", ret.msg.c_str());
			getchar();
		}
		oglShader frag(ShaderType::Fragment, getShaderFromDLL(IDR_SHADER_3DFRAG));
		ret = frag->compile();
		if (ret)
			prog3D->addShader(std::move(frag));
		else
		{
			printf("ERROR on Fragment Shader Compiler:\n%s\n", ret.msg.c_str());
			getchar();
		}
	}
	else
	{
		auto ret = oglUtil::loadShader(prog3D, pname);
		if (!ret)
		{
			printf("%s\n%s\n", ret.msg.c_str(), ret.data.c_str());
			getchar();
		}
	}
	{
		auto ret = prog3D->link({ "matProj","matView","matModel","matMVP" }, { "tex","","" }, { "vertPos","vertNorm","texPos","" });
		if (!ret)
		{
			printf("ERROR on Program Linker:\n%s\n", ret.msg.c_str());
			getchar();
		}
	}
	testVAO.reset(VAODrawMode::Triangles);
	testTri.reset(BufferType::Array);
	{
		const Point pa({ 0.0f,2.0f,0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }),
			pb({ 1.155f,0.0f,-1.0f }, { 1.0f, 0.0f, 0.0f }, { 4.0f, 0.0f }),
			pc({ -1.155f,0.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 4.0f }),
			pd({ 0.0f,0.0f,1.0f }, { 0.0f, 0.0f, 0.0f }, { 4.0f, 4.0f });
		Point DatVert[] = { pa,pb,pc, pa,pb,pd, pa,pc,pd, pb,pc,pd };
		testTri->write(DatVert, sizeof(DatVert));
		testVAO->prepare().set(testTri, prog3D->Attr_Vert_Pos, sizeof(Point), 3, 0)
			.set(testTri, prog3D->Attr_Vert_Texc, sizeof(Point), 2, 32).end();
	}
	cam.position = Vec3(0.0f, 0.0f, 4.0f);
	prog3D->setCamera(cam);
	transf.push_back({ Vec4(true),TransformType::RotateXYZ });
	transf.push_back({ Vec4(true),TransformType::Translate });
}

void BasicTest::initTex()
{
	picTex.reset(TextureType::Tex2D);
	picBuf.reset(BufferType::Pixel);
	{
		picTex->setProperty(TextureFilterVal::Nearest, TextureWrapVal::Repeat);
		Vec4 empty[128][128];
		for (int a = 0; a < 128; ++a)
		{
			for (int b = 0; b < 128; ++b)
			{
				auto& obj = empty[a][b];
				obj.x = a / 128.0f;
				obj.y = b / 128.0f;
				obj.z = (a + b) / 256.0f;
				obj.w = 1.0f;
			}
		}
		empty[0][0] = empty[0][127] = empty[127][0] = empty[127][127] = Vec4(0, 0, 1, 1);
		picTex->setData(TextureFormat::RGBAf, 128, 128, empty);
		picBuf->write(nullptr, 128 * 128 * 4, DrawMode::DynamicDraw);
	}

	mskTex.reset(TextureType::Tex2D);
	{
		mskTex->setProperty(TextureFilterVal::Nearest, TextureWrapVal::Repeat);
		Vec4 empty[128][128];
		for (int a = 0; a < 128; ++a)
		{
			for (int b = 0; b < 128; ++b)
			{
				auto& obj = empty[a][b];
				if ((a < 64 && b < 64) || (a >= 64 && b >= 64))
				{
					obj = Vec4(1, 1, 1, 1);
				}
				else
				{
					obj = Vec4(0, 0, 0, 1);
				}
			}
		}
		mskTex->setData(TextureFormat::RGBAf, 128, 128, empty);
	}
}

BasicTest::BasicTest(const string sname2d, const string sname3d)
{
	static Init _init;
	init2d(sname2d);
	init3d(sname3d);
	initTex();
	prog2D->setTexture(picTex, 0);
	prog3D->setTexture(mskTex, 0);
	
}

void BasicTest::draw()
{
	if (mode)
	{
		prog3D->draw(transf.begin(), transf.end()).draw(testVAO, 12);
	}
	else
	{
		prog2D->draw().draw(picVAO, 6);
	}
}

void BasicTest::resize(const int w, const int h)
{
	cam.resize(w, h);
	prog2D->setProject(cam, w, h);
	prog3D->setProject(cam, w, h);
}

void BasicTest::moveobj(const float x, const float y, const float z)
{
	tvec += Vec4(x, y, z, 0.0f);
	rfsData();
}

void BasicTest::rotateobj(const float x, const float y, const float z)
{
	rvec += Vec4(x, y, z, 0.0f);
	rfsData();
}

void BasicTest::rfsData()
{
	prog3D->setCamera(cam);
	transf[0].vec = rvec;
	transf[1].vec = tvec;
}

}