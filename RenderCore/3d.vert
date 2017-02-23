#version 430

uniform mat4 matProj;
uniform mat4 matView;
uniform mat4 matModel;
uniform mat4 matMVP;

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertNorm;
layout(location = 2) in vec2 texPos;

out perVert
{
	vec3 pos;
	vec2 tpos;
};

void main() 
{
	//const mat4 matMVP2 = matProj * matView * matModel;
	gl_Position = matMVP * vec4(vertPos, 1.0f);
	pos = gl_Position.xyz / gl_Position.w;
	tpos = texPos;
	//tpos = vec2((vertPos.x + 1.0f)/2, (vertPos.y + 1.0f)/2);
}