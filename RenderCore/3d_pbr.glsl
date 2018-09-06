#version 430 core
precision mediump float;
precision lowp sampler2D;
//@OGLU@Stage("VERT", "FRAG")

const float oglu_PI = 3.1415926f;

struct LightData
{
    lowp vec4 color;
    vec4 position; // w = cutoffOuter
    vec4 direction; // w = cutoffDiff
    lowp vec4 attenuation;
};
layout(std140) uniform lightBlock
{
    LightData lights[16];
};
uniform uint lightCount = 0;

struct MaterialData
{
    vec4 basic;
    vec4 other;
    uvec4 mappos;
};
layout(std140) uniform materialBlock
{
    MaterialData materials[32];
};

//@OGLU@Mapping(ProjectMat, "matProj")
uniform mat4 matProj;
//@OGLU@Mapping(ViewMat, "matView")
uniform mat4 matView;
//@OGLU@Mapping(ModelMat, "matModel")
uniform mat4 matModel;
//@OGLU@Mapping(MVPMat, "matMVP")
uniform mat4 matMVP;
//@OGLU@Mapping(CamPosVec, "vecCamPos")
uniform vec3 vecCamPos;



GLVARY perVert
{
    vec3 pos;
    vec3 pt2cam;
    vec3 norm;
    vec4 tannorm;
    vec2 tpos;
    flat uint drawId;
};

////////////////
#ifdef OGLU_VERT

//@OGLU@Mapping(VertPos, "vertPos")
in vec3 vertPos;
//@OGLU@Mapping(VertNorm, "vertNorm")
in vec3 vertNorm;
//@OGLU@Mapping(VertTexc, "vertTexc")
in vec2 vertTexc;
//@OGLU@Mapping(VertTan, "vertTan")
in vec4 vertTan;
//@OGLU@Mapping(DrawID, "ogluDrawId")
in uint ogluDrawId;

void main() 
{
    gl_Position = matMVP * vec4(vertPos, 1.0f);
    pos = (matModel * vec4(vertPos, 1.0f)).xyz;
    pt2cam = vecCamPos - pos;
    const mat3 matModel3 = mat3(matModel);
    norm = matModel3 * vertNorm;
    tannorm = vec4(matModel3 * vertTan.xyz, vertTan.w);
    tpos = vertTexc;
    drawId = ogluDrawId;
}

#endif


////////////////
#ifdef OGLU_FRAG

uniform sampler2DArray texs[16];

//@OGLU@Property("srgbTexture", BOOL, "whether input texture is srgb space")
//uniform bool srgbTexture = true;
//@OGLU@Property("envAmbient", COLOR, "environment ambient color")
uniform lowp vec4 envAmbient;
//@OGLU@Property("objidx", FLOAT, "highlighted drawIdx", 0.0, 30.0)
uniform float objidx = 0.0f;
//@OGLU@Property("idxscale", FLOAT, "scale(e^x) of drawIdx", 1.0, 24.0)
uniform float idxscale = 1.0f;

out vec4 FragColor;

OGLU_ROUTINE(LightModel, lighter, vec3)

OGLU_ROUTINE(NormalCalc, getNorm, vec3, const uint id)

OGLU_ROUTINE(AlbedoCalc, getAlbedo, vec3, const uint id)

OGLU_SUBROUTINE(NormalCalc, vertedNormal)
{
    return normalize(norm);
}
OGLU_SUBROUTINE(NormalCalc, mappedNormal)
{
    const vec3 ptNorm = normalize(norm);
    const vec3 ptTan = normalize(tannorm.xyz);
    vec3 bitanNorm = cross(ptNorm, ptTan);
    if(tannorm.w < 0.0f) bitanNorm *= -1.0f;
    const mat3 TBN = mat3(ptTan, bitanNorm, ptNorm);

    const uint pos = materials[id].mappos.y;
    const uint bank = pos >> 16;
    const float layer = float(pos & 0xffff);//let it be wrong
    const vec3 newtpos = vec3(tpos, layer);
    const vec2 ptNormRG = texture(texs[bank], newtpos).rg * 2.0f - 1.0f;
    const vec3 ptNormTex = vec3(ptNormRG, sqrt(1.0f - dot(ptNormRG, ptNormRG)));
    const vec3 ptNorm2 = TBN * ptNormTex;
    return ptNorm2;
}
OGLU_SUBROUTINE(NormalCalc, bothNormal)
{
    return (materials[id].mappos.y == 0xffff) ? vertedNormal(id) : mappedNormal(id);
}

OGLU_SUBROUTINE(AlbedoCalc, materialAlbedo)
{
    return materials[id].basic.xyz;
}
OGLU_SUBROUTINE(AlbedoCalc, mappedAlbedo)
{
    const uint pos = materials[id].mappos.x;
    const uint bank = pos >> 16;
    const float layer = float(pos & 0xffff);
    const vec3 newtpos = vec3(tpos, layer);
    return texture(texs[bank], newtpos).rgb;
}
OGLU_SUBROUTINE(AlbedoCalc, bothAlbedo)
{
    return (materials[id].mappos.x == 0xffff) ? materialAlbedo(id) : mappedAlbedo(id);
}


bool parseLight(const uint id, out vec3 p2l, out vec3 color)
{
    float atten = lights[id].attenuation.w;
    int lightType = floatBitsToInt(lights[id].color.w);
    if (lightType == 0) // parallel light
    {
        p2l = -lights[id].direction.xyz;
    }
    else if (lightType == 1) // point light
    {
        p2l = lights[id].position.xyz - pos;
        const float inv_dist = inversesqrt(dot(p2l, p2l));
        p2l *= inv_dist; // normalize
        atten *= inv_dist * inv_dist;
    }
    else if (lightType == 2) // spot light
    {
        p2l = lights[id].position.xyz - pos;
        const float inv_dist = inversesqrt(dot(p2l, p2l));
        p2l *= inv_dist; // normalize
        atten *= inv_dist * inv_dist;
        const float theta = dot(p2l, -lights[id].direction.xyz);
        const float coOuter = lights[id].position.w;
        const float coDiff_1 = lights[id].direction.w;
        atten *= clamp((theta - coOuter) * coDiff_1, 0.0f, 1.0f);
    }
    color = lights[id].color.rgb * atten;
    return atten > 0.0001f;
}

float ClampDot(const vec3 v1, const vec3 v2)
{
    return max(dot(v1, v2), 0.0f);
}


OGLU_SUBROUTINE(LightModel, tex0)
{
    const vec4 texColor = texture(texs[0], vec3(tpos, 0.0f));
    return texColor.rgb;
}
OGLU_SUBROUTINE(LightModel, tanvec)
{
    const vec3 ptNorm = normalize(tannorm.xyz);
    return (ptNorm + 1.0f) * 0.5f;
}
OGLU_SUBROUTINE(LightModel, normal)
{
    const vec3 ptNorm = getNorm(drawId);
    return (ptNorm + 1.0f) * 0.5f;
}
OGLU_SUBROUTINE(LightModel, normdiff)
{
    if(materials[drawId].mappos.y == 0xffff)
        return vec3(0.5f, 0.5f, 0.5f);
    const vec3 ptNorm = vertedNormal(drawId);
    const vec3 ptNorm2 = mappedNormal(drawId);
    const vec3 diff = ptNorm2 - ptNorm;
    return (diff + 1.0f) * 0.5f;
}
OGLU_SUBROUTINE(LightModel, dist)
{
    float depth = pow(gl_FragCoord.z, 5);
    return vec3(depth);
}
OGLU_SUBROUTINE(LightModel, view)
{
    const vec3 viewRay = normalize(pt2cam);
    return (viewRay + 1.0f) * 0.5f;
}
OGLU_SUBROUTINE(LightModel, lgt0p2l)
{
    vec3 p2l, color;
    parseLight(0, p2l, color);
    return (p2l + 1.0f) * 0.5f;
}
OGLU_SUBROUTINE(LightModel, lgt0color)
{
    vec3 p2l, color;
    parseLight(0, p2l, color);
    return color;
}
OGLU_SUBROUTINE(LightModel, drawidx)
{
    //const uint didX = drawId % 3 + 1, didY = (drawId / 3) % 3 + 1, didZ = drawId / 9 + 1;
    //const float stride = 0.25f;
    //return vec3(didX * stride, didY * stride, didZ * stride);
    if(objidx <= float(drawId))
        return vec3(1.0f, 0.0f, 0.0f);
    else
        return vec3(0.0f, 1.0f, 0.0f);
}
OGLU_SUBROUTINE(LightModel, mat0)
{
    const uint pos = materials[0].mappos.x;
    if(pos == 0xffff)
        return vec3(1.0f, 0.0f, 0.0f);
    const uint bank = pos >> 16;
    const float layer = float(pos & 0xffff);
    return vec3(0.0f, bank / 16.0f, layer / 16.0f);
}

//NDF(n,h,r) = a^2 / PI((n.h)^2 * (a^2-1) + 1)^2
//roughness4 = a^2 = roughness^4
//distribution of normal of the microfacet
float D_GGXTR(const vec3 ptNorm, const vec3 halfVec, const float roughness4)
{
    const float nh = ClampDot(ptNorm, halfVec);
    const float r2 = roughness4;
    const float tmp1 = (nh * r2 - nh) * nh + 1.0f;
    return r2 / (oglu_PI * tmp1 * tmp1);
}

//G = n.v / (n.v) * (1-k) + k 
float G_SchlickGGX(const float nv, const float k, const float one_k)
{
    return nv / (nv * one_k + k);
}

//G = G(n,v,k) * G(n,l,k)
//percentage of acceptable specular(not in shadow or mask)
float G_GGX(const float n_eye, const float n_lgt, const float k, const float one_k)
{
    //Geometry Obstruction
    const float geoObs = G_SchlickGGX(n_eye, k, one_k);
    //Geometry Shadowing
    const float geoShd = G_SchlickGGX(n_lgt, k, one_k);
    return geoObs * geoShd;
}

//fresnel reflection caused by the microfacet
vec3 F_Schlick(const vec3 halfVec, const vec3 viewRay, const vec3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - ClampDot(halfVec, viewRay), 5.0f);
}

void PBR(const lowp vec3 albedo, inout lowp vec3 diffuseColor, inout lowp vec3 specularColor)
{
    const vec3 viewRay = normalize(pt2cam);
    const vec3 ptNorm = getNorm(drawId);
    const float n_eye = ClampDot(ptNorm, viewRay);
    const float metallic = materials[drawId].basic.w;
    const vec3 diffuse_PI = (1.0f - metallic) * albedo / oglu_PI;
    const vec3 F0 = mix(vec3(0.04f), albedo, metallic);
    const float roughness = materials[drawId].other.x;
    const float roughness4 = max(roughness * roughness * roughness * roughness, 0.002f);
    const float r_1 = roughness + 1.0f;
    const float k = (r_1 * r_1) / 8.0f;
    const float one_k = 1.0f - k;
    //Geometry Obstruction
    const float geoObs = G_SchlickGGX(n_eye, k, one_k);

    for (int id = 0; id < lightCount; id++)
    {
        vec3 p2l, lgtColor;
        if(!parseLight(id, p2l, lgtColor))
            continue;

        const vec3 halfVec = normalize(p2l + viewRay);
        const float NDF = D_GGXTR(ptNorm, halfVec, roughness4);
        const float n_lgt = ClampDot(ptNorm, p2l);
        //Geometry Shadowing
        const float geoShd = G_SchlickGGX(n_lgt, k, one_k);
        //percentage of acceptable specular(not in shadow or mask)
        const float G = geoObs * geoShd;
        const vec3 F = F_Schlick(halfVec, viewRay, F0);
       
        lgtColor *= n_lgt;
        //diffuse = (1-F)(1-metal)albedo/pi
        const vec3 diffuse = diffuse_PI - diffuse_PI * F;
        diffuseColor += diffuse * lgtColor;
        const vec3 specular = F * (NDF * G / (4.0f * n_eye * n_lgt + 0.001f));
        specularColor += specular * lgtColor;
    }
}

OGLU_SUBROUTINE(LightModel, albedoOnly)
{
    const vec3 albedo = getAlbedo(drawId) / oglu_PI;
    return albedo;
}
OGLU_SUBROUTINE(LightModel, metalOnly)
{
    const float metallic = materials[drawId].basic.w;
    return vec3(metallic);
}
OGLU_SUBROUTINE(LightModel, basic)
{
    const float AO = materials[drawId].other.z;
    const lowp vec3 ambientColor = envAmbient.rgb * AO;
    lowp vec3 diffuseColor = vec3(0.0f);
    lowp vec3 specularColor = vec3(0.0f);
    const lowp vec3 albedo = getAlbedo(drawId);
    PBR(albedo, diffuseColor, specularColor);
    lowp vec3 finalColor = ambientColor + diffuseColor + specularColor;
    return finalColor;
}
OGLU_SUBROUTINE(LightModel, diffuse)
{
    const float AO = materials[drawId].other.z;
    const lowp vec3 ambientColor = envAmbient.rgb * AO;
    lowp vec3 diffuseColor = vec3(0.0f);
    lowp vec3 specularColor = vec3(0.0f);
    const lowp vec3 albedo = getAlbedo(drawId);
    PBR(albedo, diffuseColor, specularColor);
    lowp vec3 finalColor = ambientColor + diffuseColor;
    return finalColor;
}

OGLU_SUBROUTINE(LightModel, specular)
{
    const float AO = materials[drawId].other.z;
    const lowp vec3 ambientColor = envAmbient.rgb * AO;
    lowp vec3 diffuseColor = vec3(0.0f);
    lowp vec3 specularColor = vec3(0.0f);
    const lowp vec3 albedo = getAlbedo(drawId);
    PBR(albedo, diffuseColor, specularColor);
    lowp vec3 finalColor = ambientColor + specularColor;
    return finalColor;
}

void main() 
{
    const lowp vec3 color = lighter();
    FragColor = vec4(color, 1.0f);
}

#endif