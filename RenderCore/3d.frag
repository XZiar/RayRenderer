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
	FragColor.rgb = dat.xyz * vec3(0.5f) + vec3(0.5f);
	FragColor.a = 1.0f;
}