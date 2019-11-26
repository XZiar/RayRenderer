#version 330 core
#extension GL_ARB_shading_language_420pack : require

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
    //pos.x = pos.x * widthscale;
    tpos = vertTexc;
    gl_Position = vec4(pos, 1.0f, 1.0f);
}

#endif

#ifdef OGLU_FRAG

uniform sampler2D scene;
uniform sampler3D lut;

uniform vec2 lutOffset = vec2(31.0f / 32.0f, 0.5f / 32.0f);

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
    const vec3 linColor = texture(scene, tpos).rgb;
    const vec3 logColor = LinearToLogUE(linColor);
    vec3 color;
    color = texture(lut, logColor * lutOffset.x + lutOffset.y).rgb;
    FragColor.rgb = /*texture(lut, vec3(tpos, 0.5f)).rgb;//*/color;
    FragColor.w = 1.0f;
}

#endif