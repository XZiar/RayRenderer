#version 430

layout(location = 0) in vec3 vertPos;

out perVert
{
	vec3 pos;
};

void main() 
{
	pos = vertPos;
	gl_Position = vec4(vertPos, 1.0f);
}