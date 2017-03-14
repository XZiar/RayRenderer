#version 430

uniform sampler2D tex;

in perVert
{
	vec2 pos;
	vec2 tpos;
	vec3 color;
};
out vec4 FragColor;

void main() 
{
	float grey = (texture(tex, tpos).r + 1.0f) / 2.0f;
	FragColor.rgb = color * grey;
	FragColor.w = 1.0f;
}