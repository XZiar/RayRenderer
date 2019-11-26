#version 330 core
#extension GL_ARB_shading_language_420pack : require
precision mediump float;
//@OGLU@Stage("VERT", "FRAG")

GLVARY perVert
{
	vec2 pos;
	vec2 tpos;
	vec3 color;
};

#ifdef OGLU_VERT

//@OGLU@Mapping(VertPos, "vertPos")
in vec3 vertPos;
//@OGLU@Mapping(VertTexc, "vertTexc")
in vec2 vertTexc;
//@OGLU@Mapping(VertColor, "vertColor")
in lowp vec4 vertColor;

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
OGLU_ROUTINE(fontType, fontRenderer, vec4, const vec2 texpos)

//@OGLU@Property("distRange", RANGE, "range of dist", 0.0, 1.0)
uniform vec2 distRange = vec2(0.44f, 0.57f); 
//@OGLU@Property("fontColor", COLOR, "font color")
uniform lowp vec4 fontColor = vec4(1.0f);

OGLU_SUBROUTINE(fontType, plainFont)
{
	return vec4(fontColor.rgb * (1.0f - texture(tex, texpos).r), 1.0f);
}
OGLU_SUBROUTINE(fontType, sdfMid)
{
	const float dist = texture(tex, texpos).r;
	return fontColor * smoothstep(distRange.y, distRange.x, dist);//make foreground white most
}
OGLU_SUBROUTINE(fontType, compare)
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