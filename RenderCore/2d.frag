#version 430

uniform sampler2D tex[16];

in perVert
{
	vec3 pos;
};
out vec4 FragColor;

void main() 
{
	vec2 tpos = vec2((pos.x + 1.0f)/2, (pos.y + 1.0f)/2);
	FragColor = texture(tex[0], tpos);
	FragColor.w = 1.0f;
}