#include "RenderCoreRely.h"
#include "ModelMesh.h"
#include "OBJLoader.hpp"
#include "MTLLoader.hpp"
#include "OpenGLUtil/oglWorker.h"
#include "OpenGLUtil/PointEnhance.hpp"

namespace rayr::detail
{
using common::container::FindInMap;
using common::asyexe::AsyncAgent;
using b3d::Vec3;
using b3d::Vec4;
using b3d::Normal;
using b3d::PointEx;
using b3d::Coord2D;

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

struct PTstubHasher
{
    size_t operator()(const PTstub& p) const
    {
        return p.numhi * 33 + p.numlow;
    }
};

static map<u16string, ModelMesh> MODEL_CACHE;

ModelMesh _ModelMesh::GetModel(const u16string& fname, const Wrapper<oglu::oglWorker>& asyncer)
{
    if (auto md = FindInMap(MODEL_CACHE, fname))
        return *md;
    auto md = new _ModelMesh(fname, asyncer);
    ModelMesh m(std::move(md));
    MODEL_CACHE.insert_or_assign(fname, m);
    return m;
}

#pragma warning(disable:4996)
void _ModelMesh::ReleaseModel(const u16string& fname)
{
    if (auto md = FindInMap(MODEL_CACHE, fname))
        if (md->unique())
            MODEL_CACHE.erase(fname);
}
#pragma warning(default:4996)

void _ModelMesh::PrepareVAO(oglu::detail::_oglVAO::VAOPrep& vaoPrep) const
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

ejson::JObject _ModelMesh::Serialize(SerializeUtil & context) const
{
    auto jself = context.NewObject();
    jself.Add("mfname", str::to_u8string(mfname, Charset::UTF16LE));
    jself.Add("size", ToJArray(context, size));
    jself.Add("pts", context.PutResource(pts.data(), pts.size() * sizeof(b3d::PointEx)));
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
    return jself;
}


void _ModelMesh::loadOBJ(const fs::path& objpath) try
{
    using miniBLAS::VecI4;
    OBJLoder ldr(objpath);
    MTLLoader mtlLoader(oglu::TextureInnerFormat::BC7SRGB, oglu::TextureInnerFormat::BC5);
    vector<Vec3> points{ Vec3(0,0,0) };
    vector<Normal> normals{ Normal(0,0,0) };
    vector<Coord2D> texcs{ Coord2D(0,0) };
    std::unordered_map<PTstub, uint32_t, PTstubHasher> idxmap;
    idxmap.reserve(32768);
    pts.clear();
    indexs.clear();
    groups.clear();
    Vec3 maxv(-10e6, -10e6, -10e6), minv(10e6, 10e6, 10e6);
    VecI4 tmpi, tmpidx;
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
                    line.ParseInts(a + 1, tmpi.data);//vert,texc,norm
                    PTstub stub(tmpi.x, tmpi.z, tmpi.y);
                    auto[it, isAdd] = idxmap.try_emplace(stub, static_cast<uint32_t>(pts.size()));
                    if (isAdd)
                        pts.push_back(PointEx(points[stub.vid], normals[stub.nid],
                            texcs[stub.tid]));
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
    basLog().debug(u"tangent-generate cost {} us\n", tstTimer.ElapseUs());
    size = maxv - minv;
    basLog().success(u"read {} vertex, {} normal, {} texcoord\n", points.size(), normals.size(), texcs.size());
    basLog().success(u"OBJ:\t{} points, {} indexs, {} triangles\n", pts.size(), indexs.size(), indexs.size() / 3);
    basLog().info(u"OBJ size:\t [{},{},{}]\n", size.x, size.y, size.z);
    MaterialMap = mtlLoader.GetMaterialMap();
}
catch (const FileException&)
{
    basLog().error(u"Fail to open obj file\t[{}]\n", objpath.u16string());
    COMMON_THROW(BaseException, u"fail to load model data");
}

void _ModelMesh::InitDataBuffers()
{
    vbo.reset();
    vbo->Write(pts);
    ebo.reset();
    ebo->WriteCompact(indexs);
    {
        ibo.reset();
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

_ModelMesh::_ModelMesh(const u16string& fname, const Wrapper<oglu::oglWorker>& asyncer) :mfname(fname)
{
    loadOBJ(mfname);
    if (asyncer)
    {
        const auto fileName = fs::path(fname).filename().u16string();
        auto task = asyncer->InvokeShare([&](const auto& agent)
        { 
            InitDataBuffers();
            auto sync = oglu::oglUtil::SyncGL();
            agent.Await(sync);
            basLog().info(u"ModelData initialized, reported cost {}us\n", sync->ElapseNs() / 1000);
        }, fileName);
        AsyncAgent::SafeWait(task);
    }
    else
    {
        InitDataBuffers();
    }
}


}