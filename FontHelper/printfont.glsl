#version 430 core
precision mediump float;
//@@$$VERT|FRAG

GLVARY perVert
{
	vec2 pos;
	vec2 tpos;
	vec3 color;
};

#ifdef OGLU_VERT

//@@->VertPos|vertPos
layout(location = 0) in vec3 vertPos;
//@@->VertTexc|vertTexc
layout(location = 1) in vec2 vertTexc;
//@@->VertColor|vertColor
layout(location = 2) in lowp vec4 vertColor;
void main() 
{
	pos = vertPos.xy;
	tpos = vertTexc;
	color = vertColor.rgb;
	gl_Position = vec4(vertPos.xy, 1.0f, 1.0f);
}
#endif

#ifdef OGLU_FRAG
uniform sampler2D tex;
out vec4 FragColor;
subroutine vec4 fontType(vec2 texpos);
subroutine uniform fontType fontRenderer;

//@@##distRange|RANGE|range of dist|0.0|1.0
uniform vec2 distRange = vec2(0.44f, 0.57f); 
//@@##fontColor|COLOR|font color
uniform lowp vec4 fontColor = vec4(1.0f);

subroutine(fontType)
vec4 plainFont(in vec2 texpos)
{
	return vec4(fontColor.rgb * (1.0f - texture(tex, texpos).r), 1.0f);
}
subroutine(fontType)
vec4 sdfMid(in vec2 texpos)
{
	const float dist = texture(tex, texpos).r;
	return fontColor * smoothstep(distRange.y, distRange.x, dist);//make foreground white most
}
subroutine(fontType)
vec4 compare(in vec2 texpos)
{
	if (texpos.x < 0.5f)
		return plainFont(vec2(texpos.x * 2.0f, texpos.y));
	else
		return sdfMid(vec2(texpos.x * 2.0f - 1.0f, texpos.y));
}
void main() 
{
	FragColor = fontRenderer(tpos);
}
#endif