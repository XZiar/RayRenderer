#version 430

//@@$$VERT|FRAG

struct LightData
{
	vec3 position, direction;
	vec4 color, attenuation;
	float coang, exponent;
	int type;
	bool isOn;
};
layout(std140) uniform lightBlock
{
	LightData lights[16];
};

uniform mat4 matProj;
uniform mat4 matView;
uniform mat4 matModel;
uniform mat4 matMVP;

layout(std140) uniform materialBlock
{
	vec4 ambient, diffuse, specular, emission;
} material[3];


GLVARY perVert
{
	vec3 pos;
	vec4 dat;
	vec2 tpos;
};

#ifdef OGLU_VERT

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertNorm;
layout(location = 2) in vec2 texPos;

void main() 
{
	//dat.w = material[1].ambient.x + material[2].diffuse.y;
	gl_Position = matMVP * vec4(vertPos, 1.0f);
	pos = gl_Position.xyz / gl_Position.w;
	tpos = texPos;
}

#endif

#ifdef OGLU_FRAG

uniform sampler2D tex[16];

out vec4 FragColor;

void main() 
{
	FragColor = texture(tex[0], tpos);
	FragColor.a = 1.0f;
	vec3 clr = vec3(1.0f);
	for(int id = 0; id < 16; id++)
	{
		if(lights[id].isOn)
			clr = lights[id].color.rgb;
	}
	FragColor.rgb *= clr;
	//FragColor.rgb = vec3(pos.z);
}

#endif