#version 330 core
#extension GL_ARB_shading_language_420pack : require
#extension GL_ARB_shader_subroutine : require
#extension GL_ARB_gpu_shader5 : require

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
uniform sampler3D lut;

//@OGLU@Property("gamma", FLOAT, "gamma correction", 0.4, 3.2)
uniform float gamma = 2.2f;

//@OGLU@Property("exposure", FLOAT, "exposure luminunce", 0.4, 10.0)
uniform float exposure = 1.0f;

uniform vec2 lutOffset = vec2(31.0f / 32.0f, 0.5f / 32.0f);

OGLU_ROUTINE(ToneMapping, ToneMap, vec3, const vec3 color)

OGLU_SUBROUTINE(ToneMapping, NoTone)
{
    return color;
}

OGLU_SUBROUTINE(ToneMapping, Reinhard)
{
    const vec3 lum = exposure * color;
    return lum / (lum + 1.0f);
}

OGLU_SUBROUTINE(ToneMapping, ExpTone)
{
    return 1.0f - exp(color * -exposure);
}

OGLU_SUBROUTINE(ToneMapping, ACES)
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
vec3 LinearToSRGB(const vec3 color)
{
    vec3 ret;
    ret.r = color.r <= 0.00304f ? 12.92f * color.r : 1.055f * pow(color.r, 1.0f / 2.4f) - 0.055f;
    ret.g = color.g <= 0.00304f ? 12.92f * color.g : 1.055f * pow(color.g, 1.0f / 2.4f) - 0.055f;
    ret.b = color.b <= 0.00304f ? 12.92f * color.b : 1.055f * pow(color.b, 1.0f / 2.4f) - 0.055f;
    return ret;
}

//Range compression

vec3 LinearToLogUE(const vec3 val)
{
    return log2(val) / 14.0f - log2(0.18) / 14.0f + 444.0f / 1023.0f;
}
vec3 LogUEToLinear(const vec3 val)
{
    return exp2((val - 444.0f / 1023.0f) * 14.0f) * 0.18;
}

out vec4 FragColor;

void main() 
{
    //const vec3 color = ToneMap(texture(tex[0], tpos).rgb);
    const vec3 linColor = texture(tex[0], tpos).rgb;
    const vec3 logColor = LinearToLogUE(linColor);
    vec3 color;
    /*if (tpos.x < 0.5f)
        color = LinearToSRGB(ToneMap(linColor));
    else*/
        color = texture(lut, logColor * lutOffset.x + lutOffset.y).rgb;
    //FragColor.rgb = GammaCorrect(color);
    FragColor.rgb = color;
    FragColor.w = 1.0f;
}

#endif