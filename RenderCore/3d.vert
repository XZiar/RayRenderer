#version 430

struct LightInfo
{
	vec3 position, direction;
	vec4 color, attenuation;
	float coang, exponent;
	int type;
	bool isOn;
	float padding[4];
};
layout(std140) uniform lightBlock
{
	LightInfo lights[16];
};

uniform mat4 matProj;
uniform mat4 matView;
uniform mat4 matModel;
uniform mat4 matMVP;

layout(std140) uniform materialBlock
{
	vec4 ambient, diffuse, specular, emission;
} material[3];


layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertNorm;
layout(location = 2) in vec2 texPos;

out perVert
{
	vec3 pos;
	vec4 dat;
	vec2 tpos;
};

void main() 
{
	dat.w = material[1].ambient.x + material[2].diffuse.y;
	gl_Position = matMVP * vec4(vertPos, 1.0f);
	pos = gl_Position.xyz / gl_Position.w;
	tpos = texPos;
}