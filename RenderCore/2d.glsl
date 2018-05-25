#version 430 core

//@@$$VERT|FRAG


GLVARY perVert
{
    vec2 pos;
    vec2 tpos;
};

#ifdef OGLU_VERT

//@@->VertPos|vertPos
layout(location = 0) in vec3 vertPos;
//@@->VertTexc|vertTexc
layout(location = 1) in vec2 vertTexc;

void main() 
{
    pos = vertPos.xy;
    tpos = vertTexc;
    gl_Position = vec4(pos, 1.0f, 1.0f);
}

#endif

#ifdef OGLU_FRAG

uniform sampler2D tex[16];

out vec4 FragColor;

void main() 
{
    //vec2 tpos = vec2((pos.x + 1.0f)/2, (-pos.y + 1.0f)/2);
    FragColor = texture(tex[0], tpos);
    FragColor.w = 1.0f;
}

#endif