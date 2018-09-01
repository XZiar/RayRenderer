#version 330 core

//@OGLU@Stage("VERT", "FRAG")

GLVARY perVert
{
    vec2 pos;
    vec2 tpos;
};

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

out vec4 FragColor;

void main() 
{
    FragColor.x = (pos.x + 1.0f)/2.0f;
    FragColor.y = (pos.y + 1.0f)/2.0f;
    FragColor.z = (pos.x + pos.y)/2.0f;
    FragColor.w = 1.0f;
}

#endif