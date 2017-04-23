#version 430

//@@$$VERT|FRAG

GLVARY perVert
{
	vec2 pos;
	vec2 tpos;
	vec3 color;
};

uniform sampler2D tex;

#ifdef OGLU_VERT
layout(location = 0) in vec2 vertPos;
layout(location = 1) in vec2 vertTexc;
layout(location = 2) in vec3 vertColor;

void main() 
{
	pos = vertPos;
	tpos = vertTexc;
	color = vertColor;
	gl_Position = vec4(vertPos, 1.0f, 1.0f);
}
#endif

#ifdef OGLU_FRAG
out vec4 FragColor;

void main() 
{
	float grey = (texture(tex, tpos).r + 1.0f) / 2.0f;
	FragColor.rgb = color * grey;
	FragColor.w = 1.0f;
}
#endif