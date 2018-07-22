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

void main()
{
    FragColor = texture(tex, tpos);
}

#endif