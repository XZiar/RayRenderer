#version 330 core
#extension GL_ARB_shading_language_420pack : require
#extension GL_ARB_shader_subroutine : require
#extension GL_ARB_gpu_shader5 : require

//@OGLU@Stage("VERT", "FRAG", "COMP")

#ifndef OGLU_COMP
GLVARY perVert
{
    vec2 pos;
    vec2 tpos;
};
#endif

#ifdef OGLU_VERT

//@OGLU@Mapping(VertPos, "vertPos")
layout(location = 0) in vec3 vertPos;
//@OGLU@Mapping(VertTexc, "vertTexc")
layout(location = 1) in vec2 vertTexc;

void main()
{
    pos = vertPos.xy;
    tpos = vertTexc;
    gl_Position = vec4(pos, 1.0f, 1.0f);
}

#endif

#if defined(OGLU_FRAG) || defined(OGLU_COMP)

uniform sampler2D tex;

OGLU_ROUTINE(ColorConvertor, ColorConv, vec4, const vec4 color)

OGLU_SUBROUTINE(ColorConvertor, PlainCopy)
{
    return color;
}

OGLU_SUBROUTINE(ColorConvertor, GA2RGBA)
{
    return color.rrrg;
}

OGLU_SUBROUTINE(ColorConvertor, G2RGBA)
{
    return color.rrra;
}
#endif

#ifdef OGLU_FRAG

out vec4 FragColor;

void main()
{
    FragColor = ColorConv(texture(tex, tpos));
}

#endif

#ifdef OGLU_COMP

layout(local_size_x = 16, local_size_y = 16) in;
uniform vec2 coordStep;
writeonly uniform image2D result;

void main() 
{
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    vec2 tpos = coord * coordStep;
    vec4 color = ColorConv(texture(tex, tpos));
    imageStore(result, coord, color);
}
#endif