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

float getNoise()
{
    uvec2 posi = uvec2(tpos * 8192);
    uint n = posi.y * 2048u + posi.x;
    return (n * (n * n * 15731u + 789221u) + 1376312589u) / 4294967296.0f;
}

void main() 
{
    FragColor.x = (pos.x + 1.0f)/2.0f;
    FragColor.y = (pos.y + 1.0f)/2.0f;
    FragColor.z = getNoise();// (pos.x + pos.y) / 2.0f;
    //FragColor.xyz = vec3(getNoise());
    FragColor.w = 1.0f;
}

#endif