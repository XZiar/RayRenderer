#include "resource.h"
#include "BasicTest.h"
#include "../common/ResourceHelper.h"

namespace rayr
{

static string getShaderFromDLL(int32_t id)
{
	std::vector<uint8_t> data;
	if (ResourceHelper::getData(data, L"SHADER", id) != ResourceHelper::Result::Success)
		return "";
	data.push_back('\0');
	return string((const char*)data.data());
}


struct Init
{
	Init()
	{
		oglUtil::init();
	}
};


void BasicTest::init2d(const wstring pname)
{
	prog2D.reset();
	if(pname == L"")
	{
		oglShader vert(ShaderType::Vertex, getShaderFromDLL(IDR_SHADER_2DVERT));
		auto ret = vert->compile();
		if (ret)
			prog2D->addShader(std::move(vert));
		else
		{
			printf("ERROR on Vertex Shader Compiler:\n%ls\n", ret.msg.c_str());
			getchar();
		}
		oglShader frag(ShaderType::Fragment, getShaderFromDLL(IDR_SHADER_2DFRAG));
		ret = frag->compile();
		if (ret)
			prog2D->addShader(std::move(frag));
		else
		{
			printf("ERROR on Fragment Shader Compiler:\n%ls\n", ret.msg.c_str());
			getchar();
		}
	}
	else
	{
		auto ret = oglUtil::loadShader(prog2D, pname);
		if (!ret)
		{
			printf("%ls\n%ls\n", ret.msg.c_str(), ret.data.c_str());
			getchar();
		}
	}
	{
		auto ret = prog2D->link();
		if (!ret)
		{
			printf("ERROR on Program Linker:\n%ls\n", ret.msg.c_str());
			getchar();
		}
	}
	picVAO.reset(VAODrawMode::Triangles);
	screenBox.reset(BufferType::Array);
	{
		Vec3 DatVert[] = { { -1.0f, -1.0f, 0.0f },{ 1.0f, -1.0f, 0.0f },{ -1.0f, 1.0f, 0.0f },
		{ 1.0f, 1.0f, 0.0f },{ -1.0f, 1.0f, 0.0f },{ 1.0f, -1.0f, 0.0f } };
		screenBox->write(DatVert, sizeof(DatVert));
		picVAO->setDrawSize(0, 6);
		picVAO->prepare().set(screenBox, prog2D->Attr_Vert_Pos, sizeof(Vec3), 3, 0).end();
	}
}

void BasicTest::init3d(const wstring pname)
{
	prog3D.reset();
	if (pname == L"")
	{
		oglShader vert(ShaderType::Vertex, getShaderFromDLL(IDR_SHADER_3DVERT));
		auto ret = vert->compile();
		if (ret)
			prog3D->addShader(std::move(vert));
		else
		{
			printf("ERROR on Vertex Shader Compiler:\n%ls\n", ret.msg.c_str());
			getchar();
		}
		oglShader frag(ShaderType::Fragment, getShaderFromDLL(IDR_SHADER_3DFRAG));
		ret = frag->compile();
		if (ret)
			prog3D->addShader(std::move(frag));
		else
		{
			printf("ERROR on Fragment Shader Compiler:\n%ls\n", ret.msg.c_str());
			getchar();
		}
	}
	else
	{
		auto ret = oglUtil::loadShader(prog3D, pname);
		if (!ret)
		{
			printf("%ls\n%ls\n", ret.msg.c_str(), ret.data.c_str());
			getchar();
		}
	}
	{
		auto ret = prog3D->link({ "matProj","matView","matModel","matMVP" }, { "vertPos","vertNorm","texPos","" });
		if (!ret)
		{
			printf("ERROR on Program Linker:\n%ls\n", ret.msg.c_str());
			getchar();
		}
	}
	testVAO.reset(VAODrawMode::Triangles);
	testTri.reset(BufferType::Array);
	triIdx.reset(IndexSize::Byte);
	if(true)
	{
		const Point pa({ 0.0f,2.0f,0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }),
			pb({ 1.155f,0.0f,-1.0f }, { 1.0f, 0.0f, 0.0f }, { 4.0f, 0.0f }),
			pc({ -1.155f,0.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 4.0f }),
			pd({ 0.0f,0.0f,1.0f }, { 0.0f, 0.0f, 0.0f }, { 4.0f, 4.0f });
		//Point DatVert[] = { pa,pb,pc, pa,pb,pd, pa,pc,pd, pb,pc,pd };
		Point DatVert[] = { pa,pb,pc,pd };
		testTri->write(DatVert, sizeof(DatVert));
		uint8_t idxmap[] = { 0,1,2, 0,1,3, 0,2,3, 1,2,3 };
		triIdx->write(idxmap, 12);
		const GLint attrs[3] = { prog3D->Attr_Vert_Pos,prog3D->Attr_Vert_Norm,prog3D->Attr_Vert_Texc };
		testVAO->prepare().set(testTri, attrs, 0).setIndex(triIdx).end();
		testVAO->setDrawSize(0, 12);
	}
	else
	{
		const Point pa({ -0.5f,0.5f,0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }),
			pb({ -0.5f,-0.5f,0.0f }, { 1.0f, 0.0f, 0.0f }, { 4.0f, 0.0f }),
			pc({ 0.5f,-0.5f,0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 4.0f });
		Point DatVert[] = { pa,pb,pc };
		testTri->write(DatVert, sizeof(DatVert));
		testVAO->setDrawSize(0, 3);
		const GLint attrs[3] = { prog3D->Attr_Vert_Pos,prog3D->Attr_Vert_Norm,prog3D->Attr_Vert_Texc };
		testVAO->prepare().set(testTri, attrs, 0).end();
	}
	cam.position = Vec3(0.0f, 0.0f, 4.0f);
	prog3D->setCamera(cam);
	transf.push_back({ Vec4(true),TransformType::RotateXYZ });
	transf.push_back({ Vec4(true),TransformType::Translate });
	{
		Wrapper<Sphere, false> ball(0.75f);
		ball->name = L"Ball";
		ball->position = { 1,0,0,0 };
		drawables.push_back(ball);
		Wrapper<Box, false> box(0.5f, 1.0f, 2.0f);
		box->name = L"Box";
		box->position = { 0,1,0,0 };
		drawables.push_back(box);
		Wrapper<Plane, false> ground(500.0f, 50.0f);
		ground->name = L"Ground";
		ground->position = { 0,-2,0,0 };
		drawables.push_back(ground);
		Wrapper<Model, false> mod1(L"F:\\Project\\RayTrace\\DOD3model\\0\\0.obj");
		mod1->name = L"DOD3-0";
		mod1->position = { -1,3,0,0 };
		drawables.push_back(mod1);
		Wrapper<Model, false> mod2(L"F:\\Project\\RayTrace\\DOD3model\\0\\0.obj");
		mod2->name = L"DOD3-0";
		mod2->position = { 2,3,0,0 };
		drawables.push_back(mod2);
		for (auto& d : drawables)
		{
			d->prepareGL(prog3D);
		}
	}
}

void BasicTest::initTex()
{
	picTex.reset(TextureType::Tex2D);
	picBuf.reset(BufferType::Pixel);
	tmpTex.reset(TextureType::Tex2D);
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
		tmpTex->setData(TextureFormat::RGBAf, 128, 128, empty);
		picBuf->write(nullptr, 128 * 128 * 4, BufferWriteMode::DynamicDraw);
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

BasicTest::BasicTest(const wstring sname2d, const wstring sname3d)
{
	static Init _init;
	init2d(sname2d);
	init3d(sname3d);
	initTex();
	prog2D->globalState().setTexture(picTex, "tex").end();
	prog3D->globalState().setTexture(mskTex, "tex").end();
}

void BasicTest::draw()
{
	if (mode)
	{
		transf[0].vec = rvec; transf[1].vec = tvec;
		prog3D->draw(transf.begin(), transf.end()).setTexture(mskTex, "tex").draw(testVAO);
		for (const auto& d : drawables)
		{
			d->draw(prog3D);
		}
	}
	else
	{
		prog2D->draw().draw(picVAO);
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

uint16_t BasicTest::objectCount() const
{
	return (uint16_t)drawables.size();
}

void BasicTest::showObject(uint16_t objIdx) const
{
	const auto& d = drawables[objIdx];
	printf("@@Drawable %d:\t %ls  [%ls]\n", objIdx, d->name.c_str(), DrawableHelper::getType(*d).c_str());
}

}