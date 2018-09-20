#version 330
#extension GL_ARB_shading_language_420pack : require
#extension GL_ARB_shader_subroutine : require
#extension GL_ARB_gpu_shader5 : require
//@OGLU@Stage("COMP")

//Color space conversion

vec3 LinearToSRGB(const vec3 color)
{
    vec3 ret;
    ret.r = color.r <= 0.00304f ? 12.92f * color.r : 1.055f * pow(color.r, 1.0f / 2.4f) - 0.055f;
    ret.g = color.g <= 0.00304f ? 12.92f * color.g : 1.055f * pow(color.g, 1.0f / 2.4f) - 0.055f;
    ret.b = color.b <= 0.00304f ? 12.92f * color.b : 1.055f * pow(color.b, 1.0f / 2.4f) - 0.055f;
    return ret;
}
vec3 SRGBToLinear(const vec3 color)
{
    vec3 ret;
    ret.r = color.r <= 0.04045f ? (1.0f / 12.92f) * color.r : pow((1.0f / 1.055f) * (color.r + 0.055f), 2.4f);
    ret.g = color.g <= 0.04045f ? (1.0f / 12.92f) * color.g : pow((1.0f / 1.055f) * (color.g + 0.055f), 2.4f);
    ret.b = color.b <= 0.04045f ? (1.0f / 12.92f) * color.b : pow((1.0f / 1.055f) * (color.b + 0.055f), 2.4f);
    return ret;
}

//Tonemappings
#ifdef OGLU_COMP

//@OGLU@Property("exposure", FLOAT, "exposure luminunce", 0.4, 5.0)
uniform float exposure = 1.0f;

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

//Range compression

vec3 LinearToLogP1(const vec3 val)
{
    return val / (val + 1.0f);
}
vec3 LogP1ToLinear(const vec3 val)
{
    return val / (1.0f - val);
}

vec3 PosToLogP1Color(const uvec3 pos, const float step)
{
    const vec3 fpos = pos * step;
    return /*(1.0f - step) - */fpos;
}


writeonly uniform image3D result;
uniform float step;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main()
{
    const vec3 srcColor = PosToLogP1Color(gl_GlobalInvocationID.xyz, step);
    const vec3 linearColor = LogP1ToLinear(srcColor);
    const vec3 acesColor = ToneMap(linearColor);
    const vec3 srgbColor = LinearToSRGB(acesColor);
    //const vec4 color = vec4(srgbColor, 1.0f);
    const vec4 color = vec4(linearColor, 1.0f);
    imageStore(result, ivec3(gl_GlobalInvocationID.xyz), color);
}


#endif
