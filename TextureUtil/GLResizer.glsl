#version 430 core

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


subroutine vec4 ColorConvertor(const vec4);
subroutine uniform ColorConvertor ColorConv;

subroutine(ColorConvertor)
vec4 PlainCopy(const vec4 color)
{
    return color;
}

subroutine(ColorConvertor)
vec4 GA2RGBA(const vec4 color)
{
    return color.rrrg;
}

subroutine(ColorConvertor)
vec4 G2RGBA(const vec4 color)
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