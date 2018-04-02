#version 430

//@@$$VERT|FRAG

GLVARY perVert
{
	vec2 pos;
	vec2 tpos;
	vec3 color;
};

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
uniform sampler2D tex;
out vec4 FragColor;
subroutine vec3 fontType(vec2 texpos);
subroutine uniform fontType fontRenderer;

//@@##distLowbound|FLOAT|low bound of dist|0.0|1.0
uniform float distLowbound = 0.44f; 
//@@##distHighbound|FLOAT|high bound of dist|0.0|1.0
uniform float distHighbound = 0.57f; 

subroutine(fontType)
vec3 plainFont(in vec2 texpos)
{
	return texture(tex, texpos).rrr;
}
subroutine(fontType)
vec3 sdfMid(in vec2 texpos)
{
	const float dist = texture(tex, texpos).r;
	return vec3(smoothstep(distLowbound, distHighbound, dist));
}
subroutine(fontType)
vec3 compare(in vec2 texpos)
{
	if (texpos.x < 0.5f)
		return plainFont(vec2(texpos.x * 2.0f, texpos.y));
	else
		return sdfMid(vec2(texpos.x * 2.0f - 1.0f, texpos.y));
}
void main() 
{
	FragColor.rgb = fontRenderer(tpos);
	FragColor.a = 1.0f;
}
#endif