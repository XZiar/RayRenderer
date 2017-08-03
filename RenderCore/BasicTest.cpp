#include "RenderCoreRely.h"
#include "resource.h"
#include "BasicTest.h"
#include "stblib/stblib.h"
#include "ImageUtil/ImageUtil.h"
#include "ImageUtil/DataConvertor.hpp"
#include <thread>

namespace rayr
{

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

static void imguTest()
{
	common::SimpleTimer timer;
	if(false)
	{
		const fs::path srcPath = L"D:\\Programs Temps\\RayRenderer\\qw11.png";
		const auto pftch = file::FileObject::OpenThrow(srcPath, file::OpenFlag::READ | file::OpenFlag::BINARY).ReadAll();
		timer.Start();
		auto img = xziar::img::ReadImage(srcPath);
		timer.Stop();
		basLog().debug(L"libpng read cost {} ms\n", timer.ElapseMs());
		std::vector<uint32_t> data;
		timer.Start();
		auto img2 = ::stb::loadImage(srcPath, data);
		timer.Stop();
		basLog().debug(L"stbpng read cost {} ms\n", timer.ElapseMs());
		auto size = img.Width * img.Height + data.size();
		timer.Start();
		::stb::saveImage(L"D:\\Programs Temps\\RayRenderer\\ReadFrom.png", img.GetRawPtr(), img.Width, img.Height, img.ElementSize);
		timer.Stop();
		basLog().debug(L"stbpng write cost {} ms\n", timer.ElapseMs());
		timer.Start();
		xziar::img::WriteImage(img, L"D:\\Programs Temps\\RayRenderer\\ReadFrom2.png");
		timer.Stop();
		basLog().debug(L"libpng write cost {} ms\n", timer.ElapseMs());
	}
	{
		const fs::path srcPath = L"D:\\Programs Temps\\RayRenderer\\4096.tga";
		const auto pftch = file::FileObject::OpenThrow(srcPath, file::OpenFlag::READ | file::OpenFlag::BINARY).ReadAll();
		timer.Start();
		auto img = xziar::img::ReadImage(srcPath);
		timer.Stop();
		basLog().debug(L"zextga read cost {} ms\n", timer.ElapseMs()); 
		//xziar::img::WriteImage(img, L"D:\\Programs Temps\\RayRenderer\\tga.png");

		std::vector<uint32_t> data;
		timer.Start();
		auto size = stb::loadImage(srcPath, data);
		timer.Stop();
		basLog().debug(L"stbtga read cost {} ms\n", timer.ElapseMs()); 
		xziar::img::Image img2(xziar::img::ImageDataType::RGBA);
		img2.SetSize(std::get<0>(size), std::get<1>(size));
		memcpy(img2.GetRawPtr(), data.data(), img2.Size());
		//xziar::img::WriteImage(img2, L"D:\\Programs Temps\\RayRenderer\\tga2.png");
	}
	{
		const fs::path srcPath = L"D:\\Programs Temps\\RayRenderer\\head.tga";
		const auto pftch = file::FileObject::OpenThrow(srcPath, file::OpenFlag::READ | file::OpenFlag::BINARY).ReadAll();
		timer.Start();
		auto img = xziar::img::ReadImage(srcPath);
		timer.Stop();
		basLog().debug(L"zextga read cost {} ms\n", timer.ElapseMs());

		std::vector<uint32_t> data;
		timer.Start();
		auto size = stb::loadImage(srcPath, data);
		timer.Stop();
		basLog().debug(L"stbtga read cost {} ms\n", timer.ElapseMs());
		xziar::img::Image img2(xziar::img::ImageDataType::RGBA);
		img2.SetSize(std::get<0>(size), std::get<1>(size));
		memcpy(img2.GetRawPtr(), data.data(), img2.Size());
	}
}

void BasicTest::fontTest(const wchar_t word)
{
	try
	{
		imguTest();
	}
	catch (BaseException& be)
	{
		basLog().error(L"ImageUtil ERROR {}\n", be.message);
	}
	try
	{
		fontViewer.reset();
		fontCreator.reset(L"D:\\Programs Temps\\RayRenderer\\test.ttf");
		auto fonttex = fontCreator->getTexture();
		fontCreator->setChar(L'G', false);
		fontViewer->bindTexture(fonttex);
		vector<uint32_t> outer;
		auto tmper = fonttex->getData(TextureDataFormat::R8);
		outer.reserve(tmper.size());
		for (auto c : tmper)
			outer.push_back((c * 0x00010101) | 0xff000000);
		auto ftexsize = fonttex->getSize();
		::stb::saveImage(L"D:\\Programs Temps\\RayRenderer\\G.png", outer, ftexsize.first, ftexsize.second);
		fontCreator->setChar(word, false);
		tmper = fonttex->getData(TextureDataFormat::R8);
		outer.clear();
		for (auto c : tmper)
			outer.push_back((c * 0x00010101) | 0xff000000);
		ftexsize = fonttex->getSize();
		::stb::saveImage(L"D:\\Programs Temps\\RayRenderer\\A.png", outer, ftexsize.first, ftexsize.second);
		//fontCreator->bmpsdf(0x554A);
		//fontCreator->clbmpsdfgrey(0x554A);
		//fontCreator->clbmpsdfs(/*0x9f8d*/0x554A, 4096);
		tmper = fonttex->getData(TextureDataFormat::R8);
		outer.clear();
		outer.reserve(tmper.size());
		//for (auto c : tmper)
		//outer.push_back((c * 0x00010101) | 0xff000000);
		//ftexsize = fonttex->getSize();
		//::stb::saveImage(L"D:\\Programs Temps\\RayRenderer\\4096-2.png", outer, ftexsize.first, ftexsize.second);

		//fontCreator->setChar(0x9f8d, false);
		//fontCreator->stroke();
		fonttex->setProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::Repeat);
	}
	catch (BaseException& be)
	{
		basLog().error(L"Font Construct failure:\n{}\n", be.message);
		COMMON_THROW(BaseException, L"init FontViewer failed");
	}
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
	fontTest();
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

void BasicTest::reloadFontLoader(const wstring& fname)
{
	auto clsrc = file::ReadAllText(fname);
	fontCreator->reload(clsrc);
	fontTest(L'Œ“');
}

void BasicTest::reloadFontLoaderAsync(const wstring& fname, CallbackInvoke<bool> onFinish, std::function<void(BaseException&)> onError)
{
	auto clsrc = file::ReadAllText(fname);
	fontCreator->reload(clsrc);
	fontTest(L'Œ“');
	onFinish([]() { return true; });
}

bool BasicTest::addModel(const wstring& fname)
{
	Wrapper<Model> mod(fname);
	mod->name = L"model";
	mod->prepareGL(prog3D);
	drawables.push_back(mod);
	return true;
}

void BasicTest::addModelAsync(const wstring& fname, CallbackInvoke<bool> onFinish, std::function<void(BaseException&)> onError)
{
	std::thread([this, onFinish, onError](const wstring name)
	{
		try
		{
			Wrapper<Model> mod(name, true);
			mod->name = L"model";
			onFinish([&, mod]()mutable
			{
				mod->prepareGL(prog3D);
				drawables.push_back(mod);
				return true;
			});
		}
		catch (BaseException& be)
		{
			basLog().error(L"failed to load model by file {}", name);
			if (onError)
				onError(be);
			else
				onFinish([]() { return false; });
		}
	}, fname).detach();
}

void BasicTest::addLight(const b3d::LightType type)
{
	Wrapper<Light> lgt;
	switch (type)
	{
	case LightType::Parallel:
		lgt = Wrapper<ParallelLight>(std::in_place);
		lgt->color = Vec4(2.0, 0.5, 0.5, 10.0);
		break;
	case LightType::Point:
		lgt = Wrapper<PointLight>(std::in_place);
		lgt->color = Vec4(0.5, 2.0, 0.5, 10.0);
		break;
	case LightType::Spot:
		lgt = Wrapper<SpotLight>(std::in_place);
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

static uint32_t getTID()
{
	auto tid = std::this_thread::get_id();
	return *(uint32_t*)&tid;
}

void BasicTest::tryAsync(CallbackInvoke<bool> onFinish, std::function<void(BaseException&)> onError) const
{
	basLog().debug(L"begin async in pid {}\n", getTID());
	std::thread([onFinish, onError] ()
	{
		basLog().debug(L"async thread in pid {}\n", getTID());
		std::this_thread::sleep_for(std::chrono::seconds(10));
		basLog().debug(L"sleep finish. async thread in pid {}\n", getTID());
		try
		{
			if (false)
				COMMON_THROW(BaseException, L"ERROR in async call");
		}
		catch (BaseException& be)
		{
			onError(be);
			return;
		}
		onFinish([]() 
		{
			basLog().debug(L"async callback in pid {}\n", getTID());
			return true;
		});
	}).detach();
}

}