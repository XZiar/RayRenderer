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

void main()
{
    pos = vertPos.xy;
    tpos = vertTexc;
    gl_Position = vec4(pos, 1.0f, 1.0f);
}

#endif

#ifdef OGLU_FRAG

uniform sampler2D tex;

out vec4 FragColor;

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

void main()
{
    FragColor = ColorConv(texture(tex, tpos));
}

#endif