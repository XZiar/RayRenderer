#version 430

//@@$$VERT|FRAG

GLVARY perVert
{
	vec2 pos;
	vec2 tpos;
	vec3 color;
};

uniform sampler2D tex;

#ifdef OGLU_VERT
layout(location = 0) in vec2 vertPos;
layout(location = 1) in vec2 vertTexc;
layout(location = 2) in vec3 vertColor;

void main() 
{
	pos = vertPos;
	tpos = vertTexc;
	color = vertColor;
	gl_Position = vec4(vertPos, 1.0f, 1.0f);
}
#endif

#ifdef OGLU_FRAG
out vec4 FragColor;

void main() 
{
	vec2 texpos = tpos;
	texpos = vec2((pos.x + 1.0f)/2, (-pos.y + 1.0f)/2);
	//float grey = (texture(tex, tpos).r + 1.0f) / 2.0f;
	float grey = texture(tex, tpos).r;
	//FragColor.rgb = color * grey;
	FragColor.rgb = vec3(grey);
	FragColor.w = 1.0f;
}
#endif