#include "RenderCoreRely.h"
#include "Model.h"
#include "Model/OBJLoader.hpp"
#include "OpenGLUtil/PointEnhance.hpp"

namespace rayr
{
using common::container::FindInMap;
using common::asyexe::AsyncAgent;

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



map<u16string, ModelData> _ModelData::models;

ModelData _ModelData::getModel(const u16string& fname, bool asyncload)
{
    if (auto md = FindInMap(models, fname))
        return *md;
    auto md = new _ModelData(fname, asyncload);
    ModelData m(std::move(md));
    models.insert_or_assign(fname, m);
    return m;
}

#pragma warning(disable:4996)
void _ModelData::releaseModel(const u16string& fname)
{
    if (auto md = FindInMap(models, fname))
        if (md->unique())
            models.erase(fname);
}
#pragma warning(default:4996)

void _ModelData::PrepareVAO(oglu::detail::_oglVAO::VAOPrep& vaoPrep) const
{
    if (groups.empty())
        vaoPrep.SetDrawSize(0, (uint32_t)indexs.size());
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
        //vaoPrep.SetDrawSize(offs, sizs);
        vaoPrep.SetDrawSize(ibo);
    }
}


std::tuple<ModelImage, ModelImage> _ModelData::mergeTex(map<string, MtlStub>& mtlmap, vector<TexMergeItem>& texposs)
{
    //Resize Texture
    for (auto& mp : mtlmap)
    {
        auto& mtl = mp.second;
        for (auto& tex : mtl.texs)
        {
            if (tex && (tex->Width < mtl.width || tex->Height < mtl.height))
                tex->Resize(mtl.width, mtl.height);
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
        maxx = std::max(maxx, static_cast<uint16_t>(img->Width + x));
        maxy = std::max(maxy, static_cast<uint16_t>(img->Height + y));
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
    basLog().verbose(u"Build merged Diffuse texture({}*{})\n", maxx, maxy);
    ModelImage diffuse(maxx, maxy);
    for (const auto& op : opDiffuse)
    {
        auto[objimg, x, y] = op;
        diffuse->PlaceImage(*objimg, 0, 0, x, y);
    }
    opDiffuse.clear();
    basLog().verbose(u"Build merged Normal texture({}*{})\n", maxx, maxy);
    ModelImage normal(maxx, maxy);
    for (const auto& op : opNormal)
    {
        auto[objimg, x, y] = op;
        normal->PlaceImage(*objimg, 0, 0, x, y);
    }
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
    basLog().verbose(u"Parsing mtl file [{}]\n", mtlpath.u16string());
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
            basLog().verbose(u"--mtl-note [{}]\n", line.ToUString());
            break;
        case "#merge"_hash:
            {
                auto img = _ModelImage::getImage(line.GetUString(1));
                if (!img)
                    break;
                int32_t pos[2];
                line.ParseInts(2, pos);
                basLog().verbose(u"--mergeMTL [{}]--[{},{}]\n", str::to_u16string(line.Params[1]), pos[0], pos[1]);
                texposs.push_back({ img,static_cast<uint16_t>(pos[0]),static_cast<uint16_t>(pos[1]) });
            }
            break;
        case "newmtl"_hash://vertex
            curmtl = &mtlmap.insert({ string(line.Params[1]),MtlStub() }).first->second;
            break;
        case "Ka"_hash:
            curmtl->mtl.ambient = Vec4(atof(line.Params[1].data()), atof(line.Params[2].data()), atof(line.Params[3].data()), 1.0);
            break;
        case "Kd"_hash:
            curmtl->mtl.diffuse = Vec4(atof(line.Params[1].data()), atof(line.Params[2].data()), atof(line.Params[3].data()), 1.0);
            break;
        case "Ks"_hash:
            curmtl->mtl.specular = Vec4(atof(line.Params[1].data()), atof(line.Params[2].data()), atof(line.Params[3].data()), 1.0);
            break;
        case "Ke"_hash:
            curmtl->mtl.emission = Vec4(atof(line.Params[1].data()), atof(line.Params[2].data()), atof(line.Params[3].data()), 1.0);
            break;
        case "Ns"_hash:
            curmtl->mtl.shiness = (float)atof(line.Params[1].data());
            break;
        case "map_Ka"_hash:
            //curmtl.=loadTex(ldr.param[0], mtlpath.parent_path());
            //break;
        case "map_Kd"_hash:
            {
                auto tex = detail::_ModelImage::getImage(str::to_wstring(line.Rest(1), ldr.chset), mtlpath.parent_path());
                curmtl->diffuse() = tex;
                if (tex)
                    curmtl->width = std::max(curmtl->width, static_cast<uint16_t>(tex->Width)), curmtl->height = std::max(curmtl->height, static_cast<uint16_t>(tex->Height));
            }break;
        case "map_bump"_hash:
            {
                auto tex = detail::_ModelImage::getImage(str::to_wstring(line.Rest(1), ldr.chset), mtlpath.parent_path());
                curmtl->normal() = tex;
                if (tex)
                    curmtl->width = std::max(curmtl->width, static_cast<uint16_t>(tex->Width)), curmtl->height = std::max(curmtl->height, static_cast<uint16_t>(tex->Height));
            }break;
        }
    }

    std::tie(diffuse, normal) = mergeTex(mtlmap, texposs);
    diffuse->Name = mfname + u"-diffuse";
    normal->Name = mfname + u"-normal";
#if !defined(_DEBUG) && 0
    {
        auto outname = mtlpath.parent_path() / (mtlpath.stem().wstring() + L"_Normal.jpg");
        basLog().info(u"Saving Normal texture to [{}]...\n", outname.wstring());
        SimpleTimer timer;
        timer.Start();
        img::WriteImage(normal->image, outname);
        timer.Stop();
        basLog().info(u"Saving texture cost {} ms\n", timer.ElapseMs());
    }
#endif
    return mtlmap;
}
#pragma warning(disable:4101)
catch (FileException& fe)
{
    basLog().error(u"Fail to open mtl file\t[{}]\n", mtlpath.wstring());
    return map<string, MtlStub>();
}
#pragma warning(default:4101)


void _ModelData::loadOBJ(const fs::path& objpath) try
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
    SimpleTimer tstTimer;
    while (line = ldr.ReadLine())
    {
        switch (line.Type)
        {
        case "EMPTY"_hash:
            break;
        case "#"_hash:
            basLog().verbose(u"--obj-note [{}]\n", line.ToUString());
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
                {
                    basLog().warning(u"too few params for face, ignored : {}\n", line.Line);
                    break;
                }
                for (uint32_t a = 0; a < lim; ++a)
                {
                    line.ParseInts(a + 1, tmpi.raw());//vert,texc,norm
                    PTstub stub(tmpi.x, tmpi.z, tmpi.y, curmtl->posid);
                    auto [it, isAdd] = idxmap.try_emplace(stub, static_cast<uint32_t>(pts.size()));
                    if (isAdd)
                        pts.push_back(PointEx(points[stub.vid], normals[stub.nid],
                            //texcs[stub.tid]));
                            texcs[stub.tid].repos(curmtl->scalex, curmtl->scaley, curmtl->offsetx, curmtl->offsety)));
                    tmpidx[a] = it->second;
                }
                if (lim == 3)
                {
                    if (tmpidx.x == tmpidx.y || tmpidx.y == tmpidx.z || tmpidx.x == tmpidx.z)
                    {
                        basLog().warning(u"repeat index for face, ignored : {}\n", line.Line);
                        break;
                    }
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
                if (auto omtl = FindInMap(mtlmap, mtlName))
                    curmtl = omtl;
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
    basLog().debug(u"index-resize cost {} us\n", tstTimer.ElapseUs());
    tstTimer.Start();
    for (uint32_t i = 0, total = (uint32_t)indexs.size(); i < total; i += 3)
    {
        oglu::FixInvertNormal(pts[indexs[i]], pts[indexs[i + 1]], pts[indexs[i + 2]]);
        oglu::GenerateTanPoint(pts[indexs[i]], pts[indexs[i + 1]], pts[indexs[i + 2]]);
    }
    tstTimer.Stop();
    basLog().debug(u"tangent-generate cost {} us\n", tstTimer.ElapseUs());
    size = maxv - minv;
    basLog().success(u"read {} vertex, {} normal, {} texcoord\n", points.size(), normals.size(), texcs.size());
    basLog().success(u"OBJ:\t{} points, {} indexs, {} triangles\n", pts.size(), indexs.size(), indexs.size() / 3);
    basLog().info(u"OBJ size:\t [{},{},{}]\n", size.x, size.y, size.z);
}
catch (const FileException&)
{
    basLog().error(u"Fail to open obj file\t[{}]\n", objpath.u16string());
    COMMON_THROW(BaseException, L"fail to load model data");
}

void _ModelData::InitDataBuffers()
{
    diffuse.release();
    normal.release();
    vbo.reset();
    vbo->Write(pts);
    ebo.reset();
    ebo->WriteCompact(indexs);
    {
        ibo.reset();
        //ibo->WriteCommands(0, indexs.size(), true);
        vector<uint32_t> offs, sizes;
        uint32_t last = 0;
        for (const auto& g : groups)
        {
            if (!offs.empty())
                sizes.push_back(g.second - last);
            offs.push_back(last = g.second);
        }
        sizes.push_back(static_cast<uint32_t>(indexs.size() - last));
        ibo->WriteCommands(offs, sizes, true);
    }
}

void _ModelData::initData(const bool asyncload)
{
    if (asyncload)
    {
        texd = diffuse->genTextureAsync();
        texn = normal->genTextureAsync();
    }
    else
    {
        texd = diffuse->genTexture();
        texn = normal->genTexture();
    }
    InitDataBuffers();
    if (asyncload)
    {
        auto sync = oglu::oglUtil::SyncGL();
        const auto& agent = *AsyncAgent::GetAsyncAgent();
        agent.Await(sync);
        basLog().info(u"ModelData initialized, reported cost {}us\n", sync->ElapseNs() / 1000);
    }
}

_ModelData::_ModelData(const u16string& fname, bool asyncload) :mfname(fname)
{
    loadOBJ(mfname);
    if (asyncload)
    {
        const auto fileName = fs::path(fname).filename().u16string();
        auto task = oglu::oglUtil::invokeSyncGL([&](const auto&) { initData(true); }, fileName);
        AsyncAgent::SafeWait(task);
    }
    else
    {
        initData(false);
    }
}

_ModelData::~_ModelData()
{

}

//ENDOF detail
}


void Model::InitMaterial()
{
    PrepareMaterial(false);
    MaterialHolder[0].Roughness = 0.9f;
    MaterialHolder[0].UseDiffuseMap = true;
    //MaterialHolder[0].DiffuseMap = data->texd;
    MaterialHolder[0].DiffuseMap = MultiMaterialHolder::GetCheckTex();
    MaterialHolder[0].UseNormalMap = true;
    //MaterialHolder[0].NormalMap = data->texn;
    MaterialHolder[0].NormalMap = MultiMaterialHolder::GetCheckTex();
    AssignMaterial();
}

Model::Model(const u16string& fname, bool asyncload) : Drawable(this, TYPENAME), data(detail::_ModelData::getModel(fname, asyncload))
{
    const auto resizer = 2 / max(max(data->size.x, data->size.y), data->size.z);
    scale = Vec3(resizer, resizer, resizer);
    if (asyncload)
    {
        auto task = oglu::oglUtil::invokeSyncGL([&](const common::asyexe::AsyncAgent&) { InitMaterial(); });
    }
    else
    {
        InitMaterial();
    }
}

Model::~Model()
{
    const auto mfname = data->mfname;
    data.release();
    detail::_ModelData::releaseModel(mfname);
}

void Model::PrepareGL(const oglu::oglProgram& prog, const map<string, string>& translator)
{
    oglu::oglVAO vao(oglu::VAODrawMode::Triangles);
    const GLint attrs[4] = { prog->Attr_Vert_Pos, prog->Attr_Vert_Norm, prog->Attr_Vert_Texc, prog->Attr_Vert_Tan };
    {
        auto vaoprep = std::move(vao->Prepare()
            .Set(data->vbo, attrs, 0)
            .Set<uint32_t>(Drawable::GetDrawIdVBO(), prog->Attr_Draw_ID, sizeof(uint32_t), 1, 0, 1)
            .SetIndex(data->ebo));
        data->PrepareVAO(vaoprep);
    }
    SetVAO(prog, vao);
    vao->Test();
}

void Model::Draw(Drawcall& drawcall) const
{
    MaterialHolder.BindTexture(drawcall);
    DrawPosition(drawcall)
        .SetUBO(MaterialUBO, "materialBlock")
        .SetTexture(data->texd, "tex")
        .SetTexture(data->texn, "tex", 1)
        .Draw(GetVAO(drawcall.GetProg()));
}

}