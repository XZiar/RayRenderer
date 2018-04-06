#version 430
precision mediump float;
precision lowp sampler2D;
//@@$$VERT|FRAG

const float oglu_PI = 3.1415926f;

struct LightData
{
    vec3 position, direction;
    lowp vec4 color, attenuation;
    float coang, exponent;
    int type;
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
};
layout(std140) uniform materialBlock
{
    MaterialData materials[16];
};

//@@->ProjectMat|matProj
layout(location = 0) uniform mat4 matProj;
//@@->ViewMat|matView
layout(location = 1) uniform mat4 matView;
//@@->ModelMat|matModel
layout(location = 2) uniform mat4 matModel;
//@@->MVPMat|matMVP
layout(location = 3) uniform mat4 matMVP;
//@@->CamPosVec|vecCamPos
layout(location = 4) uniform vec3 vecCamPos;
//@@##envAmbient|COLOR|environment ambient color
layout(location = 5) uniform lowp vec4 envAmbient;



GLVARY perVert
{
    vec3 pos;
    vec3 cam2pt;
    vec3 norm;
    vec4 tannorm;
    vec2 tpos;
};

////////////////
#ifdef OGLU_VERT

//@@->VertPos|vertPos
layout(location = 0) in vec3 vertPos;
//@@->VertNorm|vertNorm
layout(location = 1) in vec3 vertNorm;
//@@->VertTexc|vertTexc
layout(location = 2) in vec2 vertTexc;
//@@->VertTan|vertTan
layout(location = 3) in vec4 vertTan;

void main() 
{
    gl_Position = matMVP * vec4(vertPos, 1.0f);
    pos = (matModel * vec4(vertPos, 1.0f)).xyz;
    cam2pt = pos - vecCamPos;
    const mat3 matModel3 = mat3(matModel);
    norm = matModel3 * vertNorm;
    tannorm = vec4(matModel3 * vertTan.xyz, vertTan.w);
    tpos = vertTexc;
}

#endif


////////////////
#ifdef OGLU_FRAG

uniform sampler2D tex[16];
uniform bool useDiffuseMap = false;
uniform bool useNormalMap = false;

out vec4 FragColor;

subroutine vec4 LightModel();
subroutine uniform LightModel lighter;

subroutine vec3 NormalCalc();
subroutine uniform NormalCalc getNorm;

subroutine vec3 AlbedoCalc(const vec4);
subroutine uniform AlbedoCalc getAlbedo;

subroutine(NormalCalc)
vec3 vertedNormal()
{
    return normalize(norm);
}
subroutine(NormalCalc)
vec3 mappedNormal()
{
    const vec3 ptNorm = normalize(norm);
    const vec3 ptTan = normalize(tannorm.xyz);
    vec3 bitanNorm = cross(ptNorm, ptTan);
    if(tannorm.w < 0.0f) bitanNorm *= -1.0f;
    const mat3 TBN = mat3(ptTan, bitanNorm, ptNorm);
    const vec3 ptNormTex = texture(tex[1], tpos).rgb * 2.0f - 1.0f;
    const vec3 ptNorm2 = TBN * ptNormTex;
    return ptNorm2;
}
subroutine(NormalCalc)
vec3 bothNormal()
{
    return useNormalMap ? mappedNormal() : vertedNormal();
}

subroutine(AlbedoCalc)
vec3 materialAlbedo(const vec4 albedo)
{
    return albedo.xyz;
}
subroutine(AlbedoCalc)
vec3 mappedAlbedo(const vec4 albedo)
{
    const vec3 ptNorm = normalize(norm);
    const vec3 ptTan = normalize(tannorm.xyz);
    vec3 bitanNorm = cross(ptNorm, ptTan);
    if(tannorm.w < 0.0f) bitanNorm *= -1.0f;
    const mat3 TBN = mat3(ptTan, bitanNorm, ptNorm);
    const vec3 ptNormTex = texture(tex[1], tpos).rgb * 2.0f - 1.0f;
    const vec3 ptNorm2 = TBN * ptNormTex;
    return ptNorm2;
}
subroutine(AlbedoCalc)
vec3 bothAlbedo(const vec4 albedo)
{
    return useDiffuseMap ? mappedAlbedo(albedo) : materialAlbedo(albedo);
}


subroutine(LightModel)
vec4 onlytex()
{
    vec4 texColor = texture(tex[0], tpos);
    return texColor;
}
subroutine(LightModel)
vec4 tanvec()
{
    const vec3 ptNorm = normalize(tannorm.xyz);
    return vec4((ptNorm + 1.0f) * 0.5f, 1.0f);
}
subroutine(LightModel)
vec4 norm()
{
    const vec3 ptNorm = getNorm();
    return vec4((ptNorm + 1.0f) * 0.5f, 1.0f);
}
subroutine(LightModel)
vec4 normdiff()
{
    if(!useNormalMap)
        return vec4(0.5f, 0.5f, 0.5f, 1.0f);
    const vec3 ptNorm = vertedNormal();
    const vec3 ptNorm2 = mappedNormal();
    const vec3 diff = ptNorm2 - ptNorm;
    return vec4((diff + 1.0f) * 0.5f, 1.0f);
}
subroutine(LightModel)
vec4 dist()
{
    float depth = pow(gl_FragCoord.z, 5);
    return vec4(vec3(depth), 1.0f);
}
subroutine(LightModel)
vec4 eye()
{
    const vec3 eyeRay = normalize(cam2pt);
    return vec4((eyeRay + 1.0f) * 0.5f, 1.0f);
}
subroutine(LightModel)
vec4 lgt0()
{
    const vec3 p2l = lights[0].direction;
    return vec4((p2l + 1.0f) * 0.5f, 1.0f);
}

float ClampDot(const vec3 v1, const vec3 v2)
{
    return max(dot(v1, v2), 0.0f);
}

float D_GGXTR(const vec3 ptNorm, const vec3 halfVec, const float roughness4)
{
    const float nh = ClampDot(ptNorm, halfVec);
    const float r2 = roughness4;
    const float nh2 = nh * nh;
    const float tmp1 = nh2 * (r2 - 1.0f) + 1.0f;
    return r2 / (oglu_PI * tmp1 * tmp1);
}

float G_SchlickGGX(const float nv, const float k, const float one_k)
{
    return nv / (nv * one_k + k);
}

float G_GGX(const float n_eye, const float n_lgt, const float k, const float one_k)
{
    const float geoObs = G_SchlickGGX(n_eye, k, one_k);
    const float geoShd = G_SchlickGGX(n_lgt, k, one_k);
    return geoObs * geoShd;
}

vec3 F_Schlick(const vec3 halfVec, const vec3 viewRay, const vec3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - ClampDot(halfVec, viewRay), 5.0f);
}

void PBR(out lowp vec3 diffuseColor, out lowp vec3 specularColor)
{
    const vec3 viewRay = -normalize(cam2pt);
    const vec3 ptNorm = getNorm();
    const vec3 albedo = getAlbedo(materials[0].basic);
    const float metallic = materials[0].basic.w;
    const float one_metallicPI = (1.0f - metallic) * oglu_PI;
    const vec3 F0 = mix(vec3(0.04f), albedo, metallic);
    const float roughness = materials[0].other.x;
    const float roughness4 = roughness * roughness * roughness * roughness;
    const float r_1 = roughness + 1.0f;
    const float k = (r_1 * r_1) / 8.0f;
    const float one_k = 1.0f - k;
    for (int id = 0; id < lightCount; id++)
    {
        vec3 p2l;
        float atten = 1.0f;
        if (lights[id].type == 0) // parallel light
        {
            p2l = -lights[id].direction;
        }
        else if (lights[id].type == 1) // point light
        {
            p2l = lights[id].position - pos;
            const float inv_dist = inversesqrt(dot(p2l, p2l));
            p2l *= inv_dist; // normalize
            atten = inv_dist * inv_dist;
        }
        else
            continue;
        const vec3 halfVec = normalize(p2l + viewRay);
        const float NDF = D_GGXTR(ptNorm, halfVec, roughness4);
        const float n_eye = ClampDot(ptNorm, viewRay), n_lgt = ClampDot(ptNorm, p2l);
        const float G = G_GGX(n_eye, n_lgt, k, one_k);
        const vec3 F = F_Schlick(halfVec, viewRay, F0);
       
        const lowp vec3 lgtColor = lights[id].color.rgb * (atten * n_lgt);
        const vec3 diffuse = one_metallicPI - one_metallicPI * F;
        diffuseColor += diffuse * lgtColor;
        const vec3 specular = F * (NDF * G / (4.0f * n_eye * n_lgt + 0.001f));
        specularColor += specular * lgtColor;
    }
}

subroutine(LightModel)
vec4 basic()
{
    const lowp vec3 ambientColor = envAmbient.rgb;
    lowp vec3 diffuseColor = vec3(0.0f);
    lowp vec3 specularColor = vec3(0.0f);
    PBR(diffuseColor, specularColor);
    lowp vec4 finalColor = texture(tex[0], tpos);
    finalColor.rgb *= ambientColor + diffuseColor + specularColor;
    return finalColor;
}
subroutine(LightModel)
vec4 diffuse()
{
    const lowp vec3 ambientColor = envAmbient.rgb;
    lowp vec3 diffuseColor = vec3(0.0f);
    lowp vec3 specularColor = vec3(0.0f);
    PBR(diffuseColor, specularColor);
    lowp vec4 finalColor = texture(tex[0], tpos);
    finalColor.rgb *= ambientColor + diffuseColor;
    return finalColor;
}

subroutine(LightModel)
vec4 specular()
{
    const lowp vec3 ambientColor = envAmbient.rgb;
    lowp vec3 diffuseColor = vec3(0.0f);
    lowp vec3 specularColor = vec3(0.0f);
    PBR(diffuseColor, specularColor);
    lowp vec4 finalColor = texture(tex[0], tpos);
    finalColor.rgb *= ambientColor + specularColor;
    return finalColor;
}

void main() 
{
    FragColor = lighter();
}

#endif