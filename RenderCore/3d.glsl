#version 430

//@@$$VERT|FRAG

struct LightData
{
    vec3 position, direction;
    vec4 color, attenuation;
    float coang, exponent;
    int type;
    bool isOn;
};
layout(std140) uniform lightBlock
{
    LightData lights[16];
};

layout(location = 0) uniform mat4 matProj;
layout(location = 1) uniform mat4 matView;
layout(location = 2) uniform mat4 matModel;
layout(location = 3) uniform mat4 matMVP;
//@@##envAmbient|COLOR|environment ambient color
layout(location = 4) uniform vec4 envAmbient;
layout(location = 5) uniform vec3 vecCamPos;

layout(std140) uniform materialBlock
{
    vec4 ambient, diffuse, specular, emission;
} material[3];


GLVARY perVert
{
    vec3 pos;
    vec3 cam2pt;
    vec3 norm;
    vec2 tpos;
};

////////////////
#ifdef OGLU_VERT

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNorm;
layout(location = 2) in vec2 texPos;

void main() 
{
    gl_Position = matMVP * vec4(vertPos, 1.0f);
    pos = (matModel * vec4(vertPos, 1.0f)).xyz;
    norm = (matModel * vec4(vertNorm, 0.0f)).xyz;
    tpos = texPos;
}

#endif


////////////////
#ifdef OGLU_FRAG

uniform sampler2D tex[16];

out vec4 FragColor;

subroutine vec4 LightModel();
subroutine uniform LightModel lighter;

subroutine(LightModel)
vec4 onlytex()
{
    vec4 texColor = texture(tex[0], tpos);
    return texColor;
}
subroutine(LightModel)
vec4 norm()
{
    return vec4((norm + 1.0f) * 0.5f, 1.0f);
}
subroutine(LightModel)
vec4 dist()
{
    float depth = pow(gl_FragCoord.z, 3);
    return vec4(vec3(depth), 1.0f);
}

subroutine(LightModel)
vec4 watcher()
{
    vec4 texColor = texture(tex[0], tpos);
    float cnt = 0.0f;
    vec3 sumclr = vec3(1.0f);
    for(int id = 0; id < 16; id++)
    {
        if(lights[id].isOn)
        {
            vec4 lgtClr = lights[id].color;
            sumclr += lgtClr.rgb * lgtClr.a;
            cnt += lgtClr.a;
        }
    }
    if(cnt > 0.0f)
    {
        sumclr /= cnt;
        texColor.rgb *= sumclr;
    }
    return texColor;
}



subroutine(LightModel)
vec4 basic()
{
    vec3 sumclr = envAmbient.rgb;
    for(int id = 0; id < 16; id++)
    {
        if(lights[id].isOn)
        {
            //parallel light
            vec3 eyeRay = normalize(pos - vecCamPos);
            vec3 p2l = lights[id].direction;
            vec3 lgtClr = lights[id].color.rgb;
            sumclr += lgtClr * max(dot(normalize(norm), p2l), 0.0f);
        }
    }
    vec4 texColor = texture(tex[0], tpos);
    texColor.rgb *= sumclr;
    return texColor;
}

void main() 
{
    FragColor = lighter();
}

#endif