#version 430

//@@$$VERT|FRAG


GLVARY perVert
{
	vec3 pos;
};

#ifdef OGLU_VERT

//@@->VertPos|vertPos
layout(location = 0) in vec3 vertPos;

void main() 
{
	pos = vertPos;
	gl_Position = vec4(vertPos, 1.0f);
}

#endif

#ifdef OGLU_FRAG

uniform sampler2D tex[16];

out vec4 FragColor;

void main() 
{
	vec2 tpos = vec2((pos.x + 1.0f)/2, (-pos.y + 1.0f)/2);
	FragColor = texture(tex[0], tpos);
	FragColor.w = 1.0f;
}

#endif