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

uniform sampler2D tex[16];

in perVert
{
	vec3 pos;
	vec4 dat;
	vec2 tpos;
};
out vec4 FragColor;

void main() 
{
	FragColor = texture(tex[0], tpos);
	FragColor.a = 1.0f;
	for(int id = 0; id < 16; id++)
	{
		if(lights[id].isOn)
			FragColor.rgb *= 1.1f;
	}
}