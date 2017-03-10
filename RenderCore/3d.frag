#version 430

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
}