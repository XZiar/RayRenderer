#version 430

layout(location = 0) in vec2 vertPos;
layout(location = 1) in vec2 vertTexc;
layout(location = 2) in vec3 vertColor;

out perVert
{
	vec2 pos;
	vec2 tpos;
	vec3 color;
};

void main() 
{
	pos = vertPos;
	tpos = vertTex;
	color = vertColor;
	gl_Position = vec4(vertPos, 1.0f, 1.0f);
}