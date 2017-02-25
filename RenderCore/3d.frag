#version 430

uniform sampler2D tex[16];
uniform mat4 matProj;
uniform mat4 matView;
uniform mat4 matModel;
uniform mat4 matNorm;

in perVert
{
	vec3 pos;
	vec3 dat;
	vec2 tpos;
};
out vec4 FragColor;

void main() 
{
	FragColor = texture(tex[0], tpos);
	FragColor.rgb = dat.xyz * vec3(0.5f) + vec3(0.5f);
	FragColor.a = 1.0f;
}