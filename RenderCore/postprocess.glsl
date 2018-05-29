#version 430 core

//@OGLU@Stage("VERT", "FRAG")


GLVARY perVert
{
    vec2 pos;
    vec2 tpos;
};

#ifdef OGLU_VERT

//@OGLU@Mapping(VertPos, "vertPos")
layout(location = 0) in vec3 vertPos;
//@OGLU@Mapping(VertTexc, "vertTexc")
layout(location = 1) in vec2 vertTexc;

uniform float widthscale = 1.0f;

void main() 
{
    pos = vertPos.xy;
    pos.x = pos.x * widthscale;
    tpos = vertTexc;
    gl_Position = vec4(pos, 1.0f, 1.0f);
}

#endif

#ifdef OGLU_FRAG

uniform sampler2D tex[4];

//@OGLU@Property("gamma", FLOAT, "gamma correction", 0.4, 3.2)
uniform float gamma = 2.2f;

//@OGLU@Property("exposure", FLOAT, "exposure luminunce", 0.4, 5.0)
uniform float exposure = 1.0f;

subroutine vec3 ToneMapping(const vec3);
subroutine uniform ToneMapping ToneMap;

subroutine(ToneMapping)
vec3 NoTone(const vec3 color)
{
    return color;
}

subroutine(ToneMapping)
vec3 Reinhard(const vec3 color)
{
    const vec3 lum = exposure * color;
    return lum / (lum + 1.0f);
}

subroutine(ToneMapping)
vec3 ExpTone(const vec3 color)
{
    return 1.0f - exp(color * -exposure);
}

subroutine(ToneMapping)
vec3 ACES(const vec3 color)
{
    const float A = 2.51f;
    const float B = 0.03f;
    const float C = 2.43f;
    const float D = 0.59f;
    const float E = 0.14f;

    const vec3 lum = exposure * color;
    return (lum * (A * lum + B)) / (lum * (C * lum + D) + E);
}

vec3 GammaCorrect(const vec3 color)
{
    return pow(color, vec3(1.0f / gamma));
}

out vec4 FragColor;

void main() 
{
    const vec3 color = ToneMap(texture(tex[0], tpos).rgb);
    FragColor.rgb = GammaCorrect(color);
    FragColor.w = 1.0f;
}

#endif