#include "RenderCoreRely.h"
#include "resource.h"
#include "BasicTest.h"
#include "RenderCoreInternal.h"
#include "../common/ResourceHelper.h"
#include "../FontHelper/FontHelper.h"
#include <thread>

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
		basLog().verbose(L"BasicTest Static Init\n");
		oglUtil::init();
		oclu::oclUtil::init();
	}
};


void BasicTest::init2d(const wstring pname)
{
	prog2D.reset();
	if(pname.empty())
	{
		oglShader vert(ShaderType::Vertex, getShaderFromDLL(IDR_SHADER_2DVERT));
		try
		{
			vert->compile();
			prog2D->addShader(std::move(vert));
		}
		catch(BaseException& be)
		{
			basLog().error(L"Fail to compile Vertex Shader:\n{}\n", be.message);
			COMMON_THROW(BaseException, L"compile Vertex Shader error");
		}
		oglShader frag(ShaderType::Fragment, getShaderFromDLL(IDR_SHADER_2DFRAG));
		try
		{
			frag->compile();
			prog2D->addShader(std::move(frag));
		}
		catch (BaseException& be)
		{
			basLog().error(L"Fail to compile Fragment Shader:\n{}\n", be.message);
			COMMON_THROW(BaseException, L"compile Fragment Shader error");
		}
	}
	else
	{
		try
		{
			auto shaders = oglShader::loadFromFiles(pname);
			if (shaders.size() < 2)
				COMMON_THROW(BaseException, L"No enough shader loaded from file");
			for (auto shader : shaders)
			{
				shader->compile();
				prog2D->addShader(std::move(shader));
			}
		}
		catch (OGLException& gle)
		{
			basLog().error(L"OpenGL compile fail:\n{}\n", gle.message);
			COMMON_THROW(BaseException, L"OpenGL compile fail");
		}
	}
	try
	{
		prog2D->link();
		prog2D->registerLocation({ "vertPos","","","" }, { "","","","","" });
	}
	catch (OGLException& gle)
	{
		basLog().error(L"Fail to link Program:\n{}\n", gle.message);
		COMMON_THROW(BaseException, L"link Program error");
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
		try
		{
			vert->compile();
			prog3D->addShader(std::move(vert));
		}
		catch (BaseException& be)
		{
			basLog().error(L"Fail to compile Vertex Shader:\n{}\n", be.message);
			COMMON_THROW(BaseException, L"compile Vertex Shader error");
		}
		oglShader frag(ShaderType::Fragment, getShaderFromDLL(IDR_SHADER_3DFRAG));
		try
		{
			frag->compile();
			prog3D->addShader(std::move(frag));
		}
		catch (BaseException& be)
		{
			basLog().error(L"Fail to compile Fragment Shader:\n{}\n", be.message);
			COMMON_THROW(BaseException, L"compile Fragment Shader error");
		}
	}
	else
	{
		try
		{
			auto shaders = oglShader::loadFromFiles(pname);
			if (shaders.size() < 2)
				COMMON_THROW(BaseException, L"No enough shader loaded from file");
			for (auto shader : shaders)
			{
				shader->compile();
				prog3D->addShader(std::move(shader));
			}
		}
		catch (OGLException& gle)
		{
			basLog().error(L"OpenGL compile fail:\n{}\n", gle.message);
			COMMON_THROW(BaseException, L"OpenGL compile fail");
		}
	}
	try
	{
		prog3D->link();
		prog3D->registerLocation({ "vertPos","vertNorm","texPos","" }, { "matProj", "matView", "matModel", "matNormal", "matMVP" });
	}
	catch (OGLException& gle)
	{
		basLog().error(L"Fail to link Program:\n{}\n", gle.message);
		COMMON_THROW(BaseException, L"link Program error");
	}
	
	cam.position = Vec3(0.0f, 0.0f, 4.0f);
	prog3D->setCamera(cam);
	transf.push_back({ Vec4(true),TransformType::RotateXYZ });
	transf.push_back({ Vec4(true),TransformType::Translate });
	{
		Wrapper<Pyramid> pyramid(1.0f);
		pyramid->name = L"Pyramid";
		pyramid->position = { 0,0,0 };
		drawables.push_back(pyramid);
		Wrapper<Sphere> ball(0.75f);
		ball->name = L"Ball";
		ball->position = { 1,0,0 };
		drawables.push_back(ball);
		Wrapper<Box> box(0.5f, 1.0f, 2.0f);
		box->name = L"Box";
		box->position = { 0,1,0 };
		drawables.push_back(box);
		Wrapper<Plane> ground(500.0f, 50.0f);
		ground->name = L"Ground";
		ground->position = { 0,-2,0 };
		drawables.push_back(ground);
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
		picTex->setData(TextureInnerFormat::RGBA8, TextureDataFormat::RGBAf, 128, 128, empty);
		tmpTex->setData(TextureInnerFormat::RGBA8, TextureDataFormat::RGBAf, 128, 128, empty);
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
					obj = Vec4(0.05, 0.05, 0.05, 1.0);
				}
				else
				{
					obj = Vec4(0.6, 0.6, 0.6, 1.0);
				}
			}
		}
		mskTex->setData(TextureInnerFormat::RGBA8, TextureDataFormat::RGBAf, 128, 128, empty);
	}
}

void BasicTest::initUBO()
{
	materialUBO.reset();
	lightUBO.reset();
}

void BasicTest::prepareUBO()
{
	vector<uint8_t> data(lights.size() * 96);
	size_t pos = 0;
	for (const auto& lgt : lights)
	{
		memmove(&data[pos], &(*lgt), 96);
		pos += 96;
	}
	lightUBO->write(data);
}

Wrapper<Model> BasicTest::_addModel(const wstring& fname)
{
	Wrapper<Model> mod(fname);
	mod->name = L"model";
	return mod;
}

BasicTest::BasicTest(const wstring sname2d, const wstring sname3d)
{
	static Init _init;
	{
		const auto pltfs = oclu::oclUtil::getPlatforms();
		for (const auto plt : pltfs)
		{
			auto txt = fmt::format(L"\nPlatform {} --- {} -- {}\n", plt->name, plt->ver, plt->isCurrentGL ? 'Y' : 'N');
			for (const auto dev : plt->getDevices())
				txt += fmt::format(L"--Device {}: {} -- {} -- {}\n", dev->type == oclu::DeviceType::CPU ? "CPU" : dev->type == oclu::DeviceType::GPU ? "GPU" : "OTHER",
					dev->name, dev->vendor, dev->version);
			basLog().verbose(txt);
			if (plt->isCurrentGL)
			{
				clContext = plt->createContext();
				basLog().success(L"Created Context Here!\n");
			}
		}
	}
	initTex();
	initUBO();
	init2d(sname2d);
	init3d(sname3d);
	prog2D->globalState().setTexture(picTex, "tex").end();
	prog3D->globalState().setTexture(mskTex, "tex").end();
	prog3D->globalState().setUBO(lightUBO, "lightBlock").setUBO(materialUBO, "mat").end();
}

void BasicTest::draw()
{
	if (mode)
	{
		prog3D->setCamera(cam);
		prepareUBO();
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

bool BasicTest::addModel(const wstring& fname)
{
	Wrapper<Model> mod(fname);
	mod->name = L"model";
	mod->prepareGL(prog3D);
	drawables.push_back(mod);
	return true;
}

void BasicTest::addModelAsync(const wstring& fname, std::function<void(std::function<bool(void)>)> onFinish)
{
	std::thread([this, onFinish](const wstring name)
	{
		Wrapper<Model> mod(name, true);
		mod->name = L"model";
		onFinish([&, mod]()mutable
		{
			mod->prepareGL(prog3D);
			drawables.push_back(mod);
			return true;
		});
	}, fname).detach();
}

void BasicTest::addLight(const b3d::LightType type)
{
	switch (type)
	{
	case LightType::Parallel:
		lights.push_back(Wrapper<Light>((Light*)new ParallelLight()));
		break;
	case LightType::Point:
		lights.push_back(Wrapper<Light>((Light*)new PointLight()));
		break;
	case LightType::Spot:
		lights.push_back(Wrapper<Light>((Light*)new SpotLight()));
		break;
	}
	basLog().info(L"add Light {} type {}\n", lights.size(), (int32_t)lights.back()->type);
}

void BasicTest::moveobj(const uint16_t id, const float x, const float y, const float z)
{
	drawables[id]->position += Vec3(x, y, z);
}

void BasicTest::rotateobj(const uint16_t id, const float x, const float y, const float z)
{
	drawables[id]->rotation += Vec3(x, y, z);
}

void BasicTest::movelgt(const uint16_t id, const float x, const float y, const float z)
{
	lights[id]->position += Vec3(x, y, z);
}

void BasicTest::rotatelgt(const uint16_t id, const float x, const float y, const float z)
{
	lights[id]->direction += Vec3(x, y, z);
}

uint16_t BasicTest::objectCount() const
{
	return (uint16_t)drawables.size();
}

uint16_t BasicTest::lightCount() const
{
	return (uint16_t)lights.size();
}

void BasicTest::showObject(uint16_t objIdx) const
{
	const auto& d = drawables[objIdx];
	basLog().info(L"Drawable {}:\t {}  [{}]\n", objIdx, d->name, d->getType());
}

}