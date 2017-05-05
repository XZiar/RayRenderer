#include "RenderCoreRely.h"
#include "resource.h"
#include "BasicTest.h"
#include "RenderCoreInternal.h"
#include "../3rdParty/stblib/stblib.h"
#include "../common/ResourceHelper.h"
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
		auto shaders = oglShader::loadFromExSrc(getShaderFromDLL(IDR_SHADER_2D));
		for (auto shader : shaders)
		{
			try
			{
				shader->compile();
				prog2D->addShader(std::move(shader));
			}
			catch (OGLException& gle)
			{
				basLog().error(L"OpenGL compile fail:\n{}\n", gle.message);
				COMMON_THROW(BaseException, L"OpenGL compile fail", std::any(shader));
			}
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
		auto shaders = oglShader::loadFromExSrc(getShaderFromDLL(IDR_SHADER_3D));
		for (auto shader : shaders)
		{
			try
			{
				shader->compile();
				prog3D->addShader(std::move(shader));
			}
			catch (OGLException& gle)
			{
				basLog().error(L"OpenGL compile fail:\n{}\n", gle.message);
				COMMON_THROW(BaseException, L"OpenGL compile fail", std::any(shader));
			}
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
		tmpTex->setData(TextureInnerFormat::RGBA8, TextureDataFormat::RGBAf, 128, 128, empty);
		picTex->setData(TextureInnerFormat::RGBA8, TextureDataFormat::RGBAf, 128, 128, empty);
		picBuf->write(nullptr, 128 * 128 * 4, BufferWriteMode::DynamicDraw);
	}
	{
		uint32_t empty[4][4] = { 0 };
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
	if (auto lubo = prog3D->getResource("lightBlock"))
		lightUBO.reset((*lubo)->size);
	else
		lightUBO.reset(0);
	lightLim = (uint8_t)lightUBO->size / sizeof(LightData);
	if (auto mubo = prog3D->getResource("materialBlock"))
		materialUBO.reset((*mubo)->size);
	else
		materialUBO.reset(0);
	materialLim = (uint8_t)materialUBO->size / sizeof(Material);
	prog3D->globalState().setUBO(lightUBO, "lightBlock").setUBO(materialUBO, "mat").end();
}

void BasicTest::prepareLight()
{
	vector<uint8_t> data(lightUBO->size);
	size_t pos = 0;
	for (const auto& lgt : lights)
	{
		memmove(&data[pos], &(*lgt), sizeof(LightData));
		pos += sizeof(LightData);
		if (pos >= lightUBO->size)
			break;
	}
	lightUBO->write(data, BufferWriteMode::StreamDraw);
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
	if (false)
	{
		try
		{
			oclPlatform clPlat;
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
					clPlat = plt;
					clContext = plt->createContext();
					basLog().success(L"Created Context Here!\n");
				}
			}
			oclCmdQue clQue(clContext, clPlat->getDefaultDevice());
			oclProgram clProg(clContext, getShaderFromDLL(IDR_SHADER_CL));
			clProg->build();
		}
		catch (BaseException& be)
		{
			basLog().error(L"init OpenCL context failed:\n{}\n", be.message);
			COMMON_THROW(BaseException, L"init OpenCL context failed");
		}
	}
	try
	{
		fontViewer.reset();
		fontCreator.reset(L"F:\\Software\\Font\\test.ttf");
		auto fonttex = fontCreator->getTexture();
		fontCreator->setChar(L'G', false);
		fontViewer->bindTexture(fonttex);
		vector<uint32_t> outer;
		auto tmper = fonttex->getData(TextureDataFormat::R8);
		outer.reserve(tmper.size());
		for (auto c : tmper)
			outer.push_back((c * 0x00010101) | 0xff000000);
		auto ftexsize = fonttex->getSize();
		::stb::saveImage(L"F:\\Software\\Font\\G.png", outer, ftexsize.first, ftexsize.second);
		fontCreator->setChar(0x554A, false);
		tmper = fonttex->getData(TextureDataFormat::R8);
		outer.clear();
		for (auto c : tmper)
			outer.push_back((c * 0x00010101) | 0xff000000);
		ftexsize = fonttex->getSize();
		::stb::saveImage(L"F:\\Software\\Font\\A.png", outer, ftexsize.first, ftexsize.second);
		//fontCreator->bmpsdf(0x554A);
		fontCreator->clbmpsdfs(/*0x9f8d*/0x554A, 4);
		fontCreator->clbmpsdfgrey(0x554C);
		tmper = fonttex->getData(TextureDataFormat::R8);
		outer.clear();
		outer.reserve(tmper.size());
		for (auto c : tmper)
			outer.push_back((c * 0x00010101) | 0xff000000);
		ftexsize = fonttex->getSize();
		::stb::saveImage(L"F:\\Software\\Font\\16.png", outer, ftexsize.first, ftexsize.second);
		//fontCreator->setChar(0x9f8d, false);
		//fontCreator->stroke();
		//fonttex->setProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::Repeat);
	}
	catch (BaseException& be)
	{
		basLog().error(L"Font Construct failure:\n{}\n", be.message);
		COMMON_THROW(BaseException, L"init FontViewer failed");
	}
	initTex();
	init2d(sname2d);
	init3d(sname3d);
	prog2D->globalState().setTexture(fontCreator->getTexture(), "tex").end();
	prog3D->globalState().setTexture(mskTex, "tex").end();
	initUBO();
}

void BasicTest::draw()
{
	if (mode)
	{
		prog3D->setCamera(cam);
		for (const auto& d : drawables)
		{
			d->draw(prog3D);
		}
	}
	else
	{
		fontViewer->draw();
		//prog2D->draw().draw(picVAO);
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
	Wrapper<Light> lgt;
	switch (type)
	{
	case LightType::Parallel:
		lgt = Wrapper<ParallelLight>(NoArg());
		lgt->color = Vec4(2.0, 0.5, 0.5, 10.0);
		break;
	case LightType::Point:
		lgt = Wrapper<PointLight>(NoArg());
		lgt->color = Vec4(0.5, 2.0, 0.5, 10.0);
		break;
	case LightType::Spot:
		lgt = Wrapper<SpotLight>(NoArg());
		lgt->color = Vec4(0.5, 0.5, 2.0, 10.0);
		break;
	default:
		return;
	}
	lights.push_back(lgt);
	prepareLight();
	basLog().info(L"add Light {} type {}\n", lights.size(), (int32_t)lgt->type);
}

void BasicTest::delAllLight()
{
	lights.clear();
	prepareLight();
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