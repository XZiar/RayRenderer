#version 430 core
precision mediump float;
precision lowp sampler2D;
//@OGLU@Stage("VERT", "FRAG")

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
    vec4 intensity;
    vec4 others;
};
layout(std140) uniform materialBlock
{
    MaterialData materials[16];
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
    vec3 cam2pt;
    vec3 norm;
    vec4 tannorm;
    vec2 tpos;
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
uniform bool useNormalMap = false;

out vec4 FragColor;

subroutine vec4 LightModel();
subroutine uniform LightModel lighter;

subroutine vec3 NormalCalc();
subroutine uniform NormalCalc getNorm;

//@OGLU@Property("envAmbient", COLOR, "environment ambient color")
uniform vec4 envAmbient;

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
vec3 both()
{
    return useNormalMap ? mappedNormal() : vertedNormal();
}


subroutine(LightModel)
vec4 tex0()
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
vec4 normal()
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

// this is blinn phong
void bilinnPhong(out lowp vec3 diffuseColor, out lowp vec3 specularColor)
{
    const vec3 eyeRay = normalize(cam2pt);
    const vec3 ptNorm = getNorm();
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
            const vec3 atval = lights[id].attenuation.xyz;
            p2l = lights[id].position - pos;
            const float dist2 = dot(p2l, p2l);
            const float dist = sqrt(dist2);
            p2l /= dist; // normalize
            atten = 1.0f / dot(atval, vec3(1.0f, dist, dist2));
        }
        else
            continue;

        const float lambertian = max(dot(ptNorm, p2l), 0.0f) * atten;
        const vec3 halfDir = normalize(p2l - eyeRay);
        const float specAngle = max(dot(halfDir, ptNorm), 0.0f);
        const float specular = pow(specAngle, 2/*shininess*/) * atten;

        const lowp vec3 lgtColor = lights[id].color.rgb;
        diffuseColor += lambertian * lgtColor;
        specularColor += specular * lgtColor;
    }
}

subroutine(LightModel)
vec4 basic()
{
    const lowp vec3 ambientColor = envAmbient.rgb;
    lowp vec3 diffuseColor = vec3(0.0f);
    lowp vec3 specularColor = vec3(0.0f);
    bilinnPhong(diffuseColor, specularColor);
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
    bilinnPhong(diffuseColor, specularColor);
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
    bilinnPhong(diffuseColor, specularColor);
    lowp vec4 finalColor = texture(tex[0], tpos);
    finalColor.rgb *= ambientColor + specularColor;
    return finalColor;
}

void main() 
{
    FragColor = lighter();
}

#endif