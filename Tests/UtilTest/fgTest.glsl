//#extension GL_ARB_shading_language_420pack : require

//@OGLU@Stage("VERT", "FRAG")

#ifdef GL_ES
precision highp float;
precision highp int;
precision mediump sampler2D;
precision mediump sampler3D;
#endif

//Range compression
vec3 LinearToLogUE(const vec3 val)
{
    return log2(val) / 14.0f - log2(0.18) / 14.0f + 444.0f / 1023.0f;
}
vec3 LogUEToLinear(const vec3 val)
{
    return exp2((val - 444.0f / 1023.0f) * 14.0f) * 0.18f;
}

#if defined(OGLU_VERT) || defined(OGLU_FRAG)
GLVARY perVert
{
    vec2 pos;
    vec2 tpos;
};
#endif

#ifdef OGLU_VERT

//@OGLU@Mapping(VertPos, "vertPos")
layout(location = 0) in vec2 vertPos;
//@OGLU@Mapping(VertTexc, "vertTexc")
layout(location = 1) in vec2 vertTexc;

void main() 
{
    pos = vertPos;
    tpos = vertTexc;
    gl_Position = vec4(pos, 1.0f, 1.0f);
}

#endif

#ifdef OGLU_FRAG

OGLU_TEX uniform sampler3D lut;
uniform float lutZ;
uniform int shouldLut;
const vec2 lutOffset = vec2(63.0f / 64.0f, 0.5f / 64.0f);
out vec4 FragColor;

float getNoise()
{
	vec2 pos8k = tpos * 8192.f;
    uvec2 posi = uvec2(pos8k.x, pos8k.y);
    uint n = posi.y * 2048u + posi.x;
    return float(n * (n * n * 15731u + 789221u) + 1376312589u) / 4294967296.0f;
}

void main() 
{
    if (shouldLut > 0)
    {
        vec3 lutpos = vec3(tpos, lutZ);
        //lutpos = LinearToLogUE(FragColor.xyz);
        FragColor.xyz = texture(lut, lutpos * lutOffset.x + lutOffset.y).rgb;
    }
    else
    {
        FragColor.x = (pos.x + 1.0f)/2.0f;
        FragColor.y = (pos.y + 1.0f)/2.0f;
        FragColor.xy = tpos;
        FragColor.z = getNoise();
    }
    FragColor.w = 1.0f;
}

#endif



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

