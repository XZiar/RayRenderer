#version 430

uniform sampler2D tex[16];
uniform mat4 matProj;
uniform mat4 matView;
uniform mat4 matModel;
uniform mat4 matNorm;

in perVert
{
	vec3 pos;
	vec2 tpos;
};
out vec4 FragColor;

void main() 
{
	FragColor = texture(tex[0], tpos);
	FragColor.w = 1.0f;
}