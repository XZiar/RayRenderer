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
	vec3 dat;
	vec2 tpos;
};

void main() 
{
	//const mat4 matMVP2 = matProj * matView * matModel;
	gl_Position = matMVP * vec4(vertPos, 1.0f);
	pos = gl_Position.xyz / gl_Position.w;
	tpos = texPos;
	dat.x = (gl_VertexID / 256) / 16.0f;
	dat.y = ((gl_VertexID % 256) % 16) / 16.0f;
	dat.z = (gl_VertexID % 16) / 16.0f;
	//tpos = vec2((vertPos.x + 1.0f)/2, (vertPos.y + 1.0f)/2);
}