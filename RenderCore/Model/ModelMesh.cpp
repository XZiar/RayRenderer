#include "RenderCorePch.h"
#include "ModelMesh.h"
#include "OBJLoader.hpp"
#include "MTLLoader.hpp"
#include "OBJSaver.hpp"
#include "OpenGLUtil/oglWorker.h"
#include "OpenGLUtil/PointEnhance.hpp"

namespace dizz::detail
{
using std::vector;
using common::str::Encoding;
using common::container::FindInMap;
using common::asyexe::AsyncAgent;
using oglu::PointEx;
using xziar::respak::SerializeUtil;
using xziar::respak::DeserializeUtil;


#if COMMON_COMPILER_GCC
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wpedantic"
#elif COMMON_COMPILER_CLANG
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wnested-anon-types"
#   pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#elif COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4201)
#endif
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
    PTstub(int32_t vid_, int32_t nid_, int32_t tid_)
        :vid((uint16_t)vid_), nid((uint16_t)nid_), tid((uint16_t)tid_), tposid(0)
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
#if COMMON_COMPILER_GCC
#   pragma GCC diagnostic pop
#elif COMMON_COMPILER_CLANG
#   pragma clang diagnostic pop
#elif COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

struct PTstubHasher
{
    size_t operator()(const PTstub& p) const
    {
        return p.numhi * 33 + p.numlow;
    }
};

static std::map<u16string, ModelMesh> MODEL_CACHE;

ModelMesh _ModelMesh::GetModel(DeserializeUtil& context, const string& id)
{
    ModelMesh m = context.DeserializeShare<_ModelMesh>(id);
    MODEL_CACHE.insert_or_assign(m->mfname, m);
    return m;
}

ModelMesh _ModelMesh::GetModel(const u16string& fname, const std::shared_ptr<TextureLoader>& texLoader, const std::shared_ptr<oglu::oglWorker>& asyncer)
{
    if (auto md = FindInMap(MODEL_CACHE, fname))
        return *md;
    ModelMesh m(new _ModelMesh(fname, texLoader, asyncer));
    MODEL_CACHE.insert_or_assign(fname, m);
    return m;
}

#pragma warning(disable:4996)
void _ModelMesh::ReleaseModel(const u16string& fname)
{
    if (auto md = FindInMap(MODEL_CACHE, fname))
        if (md->use_count() == 1)
            MODEL_CACHE.erase(fname);
}
#pragma warning(default:4996)

void _ModelMesh::PrepareVAO(oglu::oglVAO_::VAOPrep& vaoPrep) const
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
        vaoPrep.SetDrawSizeFrom(ibo);
    }
}

void _ModelMesh::loadOBJ(const fs::path& objpath, const std::shared_ptr<TextureLoader>& texLoader) try
{
    using mbase::IVec4;
    OBJLoder ldr(objpath);
    MTLLoader mtlLoader(texLoader);
    vector<mbase::Vec3> points{ mbase::Vec3(0.f,0.f,0.f) };
    vector<mbase::Normal> normals{ mbase::Normal(0.f,0.f,0.f) };
    vector<mbase::Vec2> texcs{ mbase::Vec2(0.f,0.f) };
    std::unordered_map<PTstub, uint32_t, PTstubHasher> idxmap;
    idxmap.reserve(32768);
    pts.clear();
    indexs.clear();
    groups.clear();
    mbase::Vec3 maxv(-10e6f, -10e6f, -10e6f), minv(10e6f, 10e6f, 10e6f);
    OBJLoder::TextLine line;
    common::SimpleTimer tstTimer;
    while ((line = ldr.ReadLine()))
    {
        switch (line.Type)
        {
        case "EMPTY"_hash:
            break;
        case "#"_hash:
            dizzLog().Verbose(u"--obj-note [{}]\n", line.ToUString());
            break;
        case "v"_hash://vertex
            {
                auto tmp = line.ParamsToFloat3(1);
                //Vec3 tmp(atof(line.Params[1].data()), atof(line.Params[2].data()), atof(line.Params[3].data()));
                maxv = Max(maxv, tmp);
                minv = Min(minv, tmp);
                points.push_back(tmp);
            }break;
        case "vn"_hash://normal
            {
                mbase::Vec3 tmp = line.ParamsToFloat3(1);
                //Vec3 tmp(atof(line.Params[1].data()), atof(line.Params[2].data()), atof(line.Params[3].data()));
                normals.push_back(tmp);
            }break;
        case "vt"_hash://texcoord
            {
                auto tmpc = line.ParamsToFloat2(1);
                //Coord2D tmpc(atof(line.Params[1].data()), atof(line.Params[2].data()));
                float intpart;
                tmpc.X = std::abs(std::modf(tmpc.X, &intpart));
                tmpc.Y = std::abs(std::modf(tmpc.Y, &intpart));
                // tmpc.regulized_mirror();
                texcs.push_back(tmpc);
            }break;
        case "f"_hash://face
            {
                mbase::IVec4 tmpidx;
                const auto lim = common::min((size_t)4, line.Params.size() - 1);
                if (lim < 3)
                {
                    dizzLog().Warning(u"too few params for face, ignored : {}\n", line.Line);
                    break;
                }
                for (uint32_t a = 0; a < lim; ++a)
                {
                    int32_t tmpi[3] = { 0 };
                    line.ParseInts(static_cast<uint8_t>(a + 1), tmpi);//vert,texc,norm
                    PTstub stub(tmpi[0], tmpi[2], tmpi[1]);
                    auto[it, isAdd] = idxmap.try_emplace(stub, static_cast<uint32_t>(pts.size()));
                    if (isAdd)
                        pts.push_back(PointEx(points[stub.vid], normals[stub.nid],
                            texcs[stub.tid]));
                    tmpidx[a] = it->second;
                }
                if (lim == 3)
                {
                    if (tmpidx.X == tmpidx.Y || tmpidx.Y == tmpidx.Z || tmpidx.X == tmpidx.Z)
                    {
                        dizzLog().Warning(u"repeat index for face, ignored : {}\n", line.Line);
                        break;
                    }
                    indexs.push_back(tmpidx.X);
                    indexs.push_back(tmpidx.Y);
                    indexs.push_back(tmpidx.Z);
                }
                else//4 vertex-> 3 vertex
                {
                    indexs.push_back(tmpidx.X);
                    indexs.push_back(tmpidx.Y);
                    indexs.push_back(tmpidx.Z);
                    indexs.push_back(tmpidx.X);
                    indexs.push_back(tmpidx.Z);
                    indexs.push_back(tmpidx.W);
                }
            }break;
        case "usemtl"_hash://each mtl is a group
            {
                string mtlName(line.Rest(1));
                groups.push_back({ mtlName,(uint32_t)indexs.size() });
            }break;
        case "mtllib"_hash://import mtl file
            mtlLoader.LoadMTL(objpath.parent_path() / string(line.Rest(1)));
            break;
        default:
            break;
        }
    }//END of WHILE
    tstTimer.Start();
    for (uint32_t i = 0, total = (uint32_t)indexs.size(); i < total; i += 3)
    {
        oglu::FixInvertNormal(pts[indexs[i]], pts[indexs[i + 1]], pts[indexs[i + 2]]);
        oglu::GenerateTanPoint(pts[indexs[i]], pts[indexs[i + 1]], pts[indexs[i + 2]]);
    }
    tstTimer.Stop();
    dizzLog().Debug(u"tangent-generate cost {} us\n", tstTimer.ElapseUs());
    size = maxv - minv;
    dizzLog().Success(u"read {} vertex, {} normal, {} texcoord\n", points.size(), normals.size(), texcs.size());
    dizzLog().Success(u"OBJ:\t{} points, {} indexs, {} triangles\n", pts.size(), indexs.size(), indexs.size() / 3);
    dizzLog().Info(u"OBJ size:\t [{:.5},{:.5},{:.5}]\n", size.X, size.Y, size.Z);
    MaterialMap = mtlLoader.GetMaterialMap();
}
catch (const common::file::FileException&)
{
    dizzLog().Error(u"Fail to open obj file\t[{}]\n", objpath.u16string());
    COMMON_THROWEX(BaseException, u"fail to load model data");
}

void _ModelMesh::InitDataBuffers(const std::shared_ptr<oglu::oglWorker>& asyncer)
{
    if (asyncer)
    {
        const auto fileName = fs::path(mfname).filename().u16string();
        auto task = asyncer->InvokeShare([&](const auto& agent)
        {
            InitDataBuffers();
            auto sync = oglu::oglUtil::SyncGL();
            agent.Await(sync);
            dizzLog().Info(u"ModelData initialized, reported cost {}us\n", sync->ElapseNs() / 1000);
        }, fileName);
        AsyncAgent::SafeWait(task);
        return;
    }
    vbo = oglu::oglArrayBuffer_::Create();
    vbo->WriteSpan(pts);
    ebo = oglu::oglElementBuffer_::Create();
    ebo->WriteCompact(indexs);
    {
        ibo = oglu::oglIndirectBuffer_::Create();
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
_ModelMesh::_ModelMesh(const u16string& fname) : mfname(fname) {}
_ModelMesh::_ModelMesh(const u16string& fname, const std::shared_ptr<TextureLoader>& texLoader, const std::shared_ptr<oglu::oglWorker>& asyncer) 
    : mfname(fname)
{
    loadOBJ(mfname, texLoader);
    InitDataBuffers(asyncer);
}

RESPAK_IMPL_COMP_DESERIALIZE(_ModelMesh, u16string)
{
    u16string name = common::str::to_u16string(object.Get<string>("mfname"), Encoding::UTF8);
    return std::any(std::make_tuple(name));
}
void _ModelMesh::Serialize(SerializeUtil & context, xziar::ejson::JObject& jself) const
{
    jself.Add("mfname", common::str::to_u8string(mfname));
    jself.Add("size", ToJArray(context, size));
    jself.Add("pts", context.PutResource(pts.data(), pts.size() * sizeof(oglu::PointEx)));
    jself.Add("indexs", context.PutResource(indexs.data(), indexs.size() * sizeof(uint32_t)));
    auto groupArray = context.NewArray();
    for (const auto&[name, count] : groups)
    {
        auto grp = context.NewObject();
        grp.Add("Name", name);
        grp.Add("Offset", count);
        groupArray.Push(grp);
    }
    jself.Add("groups", groupArray);
    auto jmaterials = context.NewObject();
    for (const auto&[name, mat] : MaterialMap)
        context.AddObject(jmaterials, name, mat);
    jself.Add("materials", jmaterials);
}
void _ModelMesh::Deserialize(DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object)
{
    FromJArray(object.GetArray("size"), size);
    {
        const auto ptsData = context.GetResource(object.Get<string>("pts"));
        pts.resize(ptsData.GetSize() / sizeof(oglu::PointEx));
        memcpy_s(pts.data(), pts.size() * sizeof(oglu::PointEx), ptsData.GetRawPtr(), ptsData.GetSize());
    }
    {
        const auto idxData = context.GetResource(object.Get<string>("indexs"));
        indexs.resize(idxData.GetSize() / sizeof(uint32_t));
        memcpy_s(indexs.data(), indexs.size() * sizeof(uint32_t), idxData.GetRawPtr(), idxData.GetSize());
    }
    groups = common::linq::FromContainer(object.GetArray("groups"))
        .Cast<xziar::ejson::JObjectRef<true>>()
        .Select([](const xziar::ejson::JObjectRef<true>& obj) { return std::pair{ obj.Get<string>("Name"), obj.Get<uint32_t>("Offset") }; })
        .ToVector();
    MaterialMap.clear();
    common::linq::FromContainer(object.GetObject("materials"))
        .IntoMap(MaterialMap, 
            [](const auto& kvpair) { return (string)kvpair.first; },
            [&](const auto& kvpair) { return PBRMaterial(*context.Deserialize<PBRMaterial>(xziar::ejson::JObjectRef<true>(kvpair.second)).release()); });

    const auto asyncer = context.GetCookie<std::shared_ptr<oglu::oglWorker>>("oglWorker");
    InitDataBuffers(asyncer ? *asyncer : std::shared_ptr<oglu::oglWorker>());
}


}