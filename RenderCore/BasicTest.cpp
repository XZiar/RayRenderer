#include "RenderCoreRely.h"
#include "resource.h"
#include "BasicTest.h"
#include "ImageUtil/ImageUtil.h"
#include "ImageUtil/DataConvertor.hpp"
#include <thread>

namespace rayr
{

struct Init
{
    Init()
    {
        basLog().verbose(u"BasicTest Static Init\n");
        oglUtil::init();
        oclu::oclUtil::init();
    }
};


void BasicTest::init2d(const u16string pname)
{
    prog2D.reset(u"Prog 2D");
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
                basLog().error(u"OpenGL compile fail:\n{}\n", gle.message);
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
            basLog().error(u"OpenGL compile fail:\n{}\n", gle.message);
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
        basLog().error(u"Fail to link Program:\n{}\n", gle.message);
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

void BasicTest::init3d(const u16string pname)
{
    prog3D.reset(u"3D Prog");
    if (pname.empty())
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
                basLog().error(u"OpenGL compile fail:\n{}\n", gle.message);
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
            basLog().error(u"OpenGL compile fail:\n{}\n", gle.message);
            COMMON_THROW(BaseException, L"OpenGL compile fail");
        }
    }
    try
    {
        prog3D->link();
        prog3D->registerLocation({ "vertPos","vertNorm","texPos","" }, { "matProj", "matView", "matModel", "matNormal", "matMVP" });
        prog3D->globalState().setSubroutine("lighter", "watcher").end();
    }
    catch (OGLException& gle)
    {
        basLog().error(u"Fail to link Program:\n{}\n", gle.message);
        COMMON_THROW(BaseException, L"link Program error");
    }
    
    cam.position = Vec3(0.0f, 0.0f, 4.0f);
    prog3D->setCamera(cam);
    {
        Wrapper<Pyramid> pyramid(1.0f);
        pyramid->name = u"Pyramid";
        pyramid->position = { 0,0,0 };
        AddObject(pyramid);
        Wrapper<Sphere> ball(0.75f);
        ball->name = u"Ball";
        ball->position = { 1,0,0 };
        AddObject(ball);
        Wrapper<Box> box(0.5f, 1.0f, 2.0f);
        box->name = u"Box";
        box->position = { 0,1,0 };
        AddObject(box);
        Wrapper<Plane> ground(500.0f, 50.0f);
        ground->name = u"Ground";
        ground->position = { 0,-2,0 };
        AddObject(ground);
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
        lightUBO.reset(lubo->size);
    else
        lightUBO.reset(0);
    lightLim = (uint8_t)lightUBO->size / sizeof(LightData);
    if (auto mubo = prog3D->getResource("materialBlock"))
        materialUBO.reset(mubo->size);
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
        memcpy_s(&data[pos], lightUBO->size - pos, &(*lgt), sizeof(LightData));
        pos += sizeof(LightData);
        if (pos >= lightUBO->size)
            break;
    }
    lightUBO->write(data, BufferWriteMode::StreamDraw);
}

void BasicTest::fontTest(const char32_t word)
{
    try
    {
        auto fonttex = fontCreator->getTexture();
        if (word == 0x0)
        {
            const auto imgShow = fontCreator->clgraysdfs(U'°¡', 16);
            oglUtil::invokeSyncGL([&imgShow, &fonttex](const common::asyexe::AsyncAgent& agent) 
            {
                fonttex->setData(TextureInnerFormat::R8, imgShow);
                agent.Await(oglu::oglUtil::SyncGL());
            })->wait();
            img::WriteImage(imgShow, basepath / (u"Show.png"));
            /*SimpleTimer timer;
            for (uint32_t cnt = 0; cnt < 65536; cnt += 4096)
            {
                const auto imgShow = fontCreator->clgraysdfs((char32_t)cnt, 4096);
                img::WriteImage(imgShow, basepath / (u"Show-" + std::to_u16string(cnt) + u".png"));
                basLog().success(u"successfully processed words begin from {}\n", cnt);
            }
            timer.Stop();
            basLog().success(u"successfully processed 65536 words, cost {}ms\n", timer.ElapseMs());*/
        }
        else
        {
            fontCreator->setChar(L'G', false);
            const auto imgG = fonttex->getImage(TextureDataFormat::R8);
            img::WriteImage(imgG, basepath / u"G.png");
            fontCreator->setChar(word, false);
            const auto imgA = fonttex->getImage(TextureDataFormat::R8);
            img::WriteImage(imgA, basepath / u"A.png");
            const auto imgShow = fontCreator->clgraysdfs(U'°¡', 16);
            fonttex->setData(TextureInnerFormat::R8, imgShow);
            img::WriteImage(imgShow, basepath / u"Show.png");
            fonttex->setProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::Clamp);
            fontViewer->bindTexture(fonttex);
            //fontViewer->prog->globalState().setSubroutine("fontRenderer", "plainFont").end();
        }
    }
    catch (BaseException& be)
    {
        basLog().error(u"Font Construct failure:\n{}\n", be.message);
        //COMMON_THROW(BaseException, L"init FontViewer failed");
    }
}

BasicTest::BasicTest(const u16string sname2d, const u16string sname3d)
{
    static Init _init;
    fontViewer.reset();
    fontCreator.reset(oclu::Vendor::Intel);
    basepath = u"D:\\Programs Temps\\RayRenderer";
    if (!fs::exists(basepath))
        basepath = u"C:\\Programs Temps\\RayRenderer";
    fontCreator->reloadFont(basepath / u"test.ttf");

    fontTest(/*L'‡å'*/);
    initTex();
    init2d(sname2d);
    init3d(sname3d);
    prog2D->globalState().setTexture(fontCreator->getTexture(), "tex").end();
    prog3D->globalState().setTexture(mskTex, "tex").end();
    initUBO();
    glProgs.push_back(prog2D);
    glProgs.push_back(prog3D);
    glProgs.push_back(fontViewer->prog);
}

void BasicTest::Draw()
{
    {
        const auto changed = (ChangableUBO)IsUBOChanged.exchange(0, std::memory_order::memory_order_relaxed);
        if (HAS_FIELD(changed, ChangableUBO::Light))
            prepareLight();
    }
    if (mode)
    {
        prog3D->setCamera(cam);
        auto drawcall = prog3D->draw();
        for (const auto& d : drawables)
        {
            d->draw(drawcall);
        }
    }
    else
    {
        fontViewer->draw();
        //prog2D->draw().draw(picVAO);
    }
}

void BasicTest::Resize(const int w, const int h)
{
    cam.resize(w, h);
    prog2D->setProject(cam, w, h);
    prog3D->setProject(cam, w, h);
}

void BasicTest::ReloadFontLoader(const u16string& fname)
{
    auto clsrc = file::ReadAllText(fs::path(fname));
    fontCreator->reload(clsrc);
    fontTest(0);
}

void BasicTest::ReloadFontLoaderAsync(const u16string& fname, CallbackInvoke<bool> onFinish, std::function<void(BaseException&)> onError)
{
    std::thread([this, onFinish, onError](const u16string name)
    {
        common::SetThreadName(u"AsyncLoader for FontTest");
        try
        {
            auto clsrc = file::ReadAllText(fs::path(name));
            fontCreator->reload(clsrc);
            fontTest(0);
            onFinish([]() { return true; });
        }
        catch (BaseException& be)
        {
            basLog().error(u"failed to reload font test\n");
            if (onError)
                onError(be);
            else
                onFinish([]() { return false; });
        }
    }, fname).detach();
}


void BasicTest::LoadModelAsync(const u16string& fname, std::function<void(Wrapper<Model>)> onFinish, std::function<void(BaseException&)> onError)
{
    std::thread([this, onFinish, onError](const u16string name)
    {
        common::SetThreadName(u"AsyncLoader for Model");
        try
        {
            Wrapper<Model> mod(name, true);
            mod->name = u"model";
            onFinish(mod);
        }
        catch (BaseException& be)
        {
            basLog().error(u"failed to load model by file {}\n", name);
            if (onError)
                onError(be);
            else
                onFinish(Wrapper<Model>());
        }
    }, fname).detach();
}

bool BasicTest::AddObject(const Wrapper<Drawable>& drawable)
{
    drawable->prepareGL(prog3D);
    drawables.push_back(drawable);
    basLog().success(u"Add an Drawable [{}][{}]:  {}\n", drawables.size() - 1, drawable->getType(), drawable->name);
    return true;
}

bool BasicTest::AddLight(const Wrapper<Light>& light)
{
    lights.push_back(light);
    prepareLight();
    basLog().success(u"Add a Light [{}][{}]:  {}\n", lights.size() - 1, (int32_t)light->type, light->name);
    return true;
}

void BasicTest::DelAllLight()
{
    lights.clear();
    prepareLight();
}

void BasicTest::ReportChanged(const ChangableUBO target)
{
    IsUBOChanged |= (uint32_t)target;
}

static uint32_t getTID()
{
    auto tid = std::this_thread::get_id();
    return *(uint32_t*)&tid;
}

void BasicTest::tryAsync(CallbackInvoke<bool> onFinish, std::function<void(BaseException&)> onError) const
{
    basLog().debug(u"begin async in pid {}\n", getTID());
    std::thread([onFinish, onError] ()
    {
        common::SetThreadName(u"AsyncThread for TryAsync");
        basLog().debug(u"async thread in pid {}\n", getTID());
        std::this_thread::sleep_for(std::chrono::seconds(10));
        basLog().debug(u"sleep finish. async thread in pid {}\n", getTID());
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
            basLog().debug(u"async callback in pid {}\n", getTID());
            return true;
        });
    }).detach();
}

}