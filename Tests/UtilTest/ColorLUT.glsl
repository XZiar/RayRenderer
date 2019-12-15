#extension GL_ARB_shading_language_420pack	: require
#if defined(OGLU_COMP)
#   extension GL_ARB_shader_image_load_store	: require
#endif
//@OGLU@Stage("VERT", "GEOM", "FRAG", "COMP")
//@OGLU@StageIf("GL_AMD_vertex_shader_layer", "!GEOM")


#if defined(OGLU_COMP) || defined(OGLU_FRAG)

//Tonemappings
//@OGLU@Property("exposure", FLOAT, "exposure luminunce", 0.4, 5.0)
uniform float exposure = 1.0f;


vec3 ToneMapping(const vec3 color)
{
    const float A = 2.51f;
    const float B = 0.03f;
    const float C = 2.43f;
    const float D = 0.59f;
    const float E = 0.14f;

    const vec3 lum = exposure * color;
    return (lum * (A * lum + B)) / (lum * (C * lum + D) + E);
}

const mat3x3 SRGB_XYZ = mat3x3
(
     0.4124564f,  0.2126729f,  0.0193339f, // column0
     0.3575761f,  0.7151522f,  0.1191920f, // column1
     0.1804375f,  0.0721750f,  0.9503041f  // column2
);
const mat3x3 XYZ_SRGB = mat3x3
(
     3.2409699419f, -0.9692436363f,  0.0556300797f, // column0
    -1.5373831776f,  1.8759675015f, -0.2039769589f, // column1
    -0.4986107603f,  0.0415550574f,  1.0569715142f  // column2
);
const mat3x3 XYZ_AP0 = mat3x3
(
     1.0498110175f, -0.4959030231f,  0.0000000000f, // column0
     0.0000000000f,  1.3733130458f,  0.0000000000f, // column1
    -0.0000974845f,  0.0982400361f,  0.9912520182f  // column2
);
const mat3x3 AP0_XYZ = mat3x3
(
     0.9525523959f,  0.3439664498f,  0.0000000000f, // column0
     0.0000000000f,  0.7281660966f,  0.0000000000f, // column1
     0.0000936786f, -0.0721325464f,  1.0088251844f  // column2
);
const mat3x3 XYZ_AP1 = mat3x3
(
     1.6410233797f, -0.6636628587f,  0.0117218943f, // column0
    -0.3248032942f,  1.6153315917f, -0.0082844420f, // column1
    -0.2364246952f,  0.0167563477f,  0.9883948585f  // column2
);
const mat3x3 AP1_XYZ = mat3x3
(
     0.6624541811f,  0.2722287168f, -0.0055746495f, // column0
     0.1340042065f,  0.6740817658f,  0.0040607335f, // column1
     0.1561876870f,  0.0536895174f,  1.0103391003f  // column2
);
const mat3x3 D65_D60 = mat3x3
(
     1.01303f,     0.00769823f, -0.00284131f, // column0
     0.00610531f,  0.998165f,    0.00468516f, // column1
    -0.014971f,   -0.00503203f,  0.924507f    // column2
);
const mat3x3 D60_D65 = mat3x3
(
     0.987224f,   -0.00759836f,  0.00307257f, // column0
    -0.00611327f,  1.00186f,    -0.00509595f, // column1
     0.0159533f,   0.00533002f,  1.08168f     // column2
);

//Range compression
vec3 LinearToLogUE(const vec3 val)
{
    return log2(val) / 14.0f - log2(0.18) / 14.0f + 444.0f / 1023.0f;
}
vec3 LogUEToLinear(const vec3 val)
{
    return exp2((val - 444.0f / 1023.0f) * 14.0f) * 0.18f;
}

//Color space conversion
vec3 LinearToSRGB(const vec3 color)
{
    vec3 ret;
    ret.r = color.r <= 0.00304f ? 12.92f * color.r : 1.055f * pow(color.r, 1.0f / 2.4f) - 0.055f;
    ret.g = color.g <= 0.00304f ? 12.92f * color.g : 1.055f * pow(color.g, 1.0f / 2.4f) - 0.055f;
    ret.b = color.b <= 0.00304f ? 12.92f * color.b : 1.055f * pow(color.b, 1.0f / 2.4f) - 0.055f;
    return ret;
}
vec3 SRGBToLinear(const vec3 color)
{
    vec3 ret;
    ret.r = color.r <= 0.04045f ? (1.0f / 12.92f) * color.r : pow((1.0f / 1.055f) * (color.r + 0.055f), 2.4f);
    ret.g = color.g <= 0.04045f ? (1.0f / 12.92f) * color.g : pow((1.0f / 1.055f) * (color.g + 0.055f), 2.4f);
    ret.b = color.b <= 0.04045f ? (1.0f / 12.92f) * color.b : pow((1.0f / 1.055f) * (color.b + 0.055f), 2.4f);
    return ret;
}


uniform float step;
#endif


#if defined(OGLU_COMP)

OGLU_IMG writeonly uniform image3D result;
layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;
void main()
{
    const vec3 srcColor = gl_GlobalInvocationID.xyz * step;
    const vec3 linearColor = LogUEToLinear(srcColor);
    const vec3 acesColor = ToneMapping(linearColor);
    const vec3 srgbColor = LinearToSRGB(acesColor);
    const vec4 color = vec4(srgbColor, 1.0f);
    imageStore(result, ivec3(gl_GlobalInvocationID.xyz), color);
}

#endif


#if defined(OGLU_VERT)

//@OGLU@Mapping(VertPos, "vertPos")
layout(location = 0) in vec2 vertPos;

void main()
{
    gl_Position = vec4(vertPos.xy, 1.0f, 1.0f);
    ogluSetLayer(gl_InstanceID);
}

#endif


#if defined(OGLU_GEOM)

uniform int lutSize;

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

void main()
{
    for(int i = 0; i < gl_in.length(); ++i)
    {
        ogluSetLayer(ogluGetLayer());
        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}

#endif


#if defined(OGLU_FRAG)

uniform int lutSize;
in vec4 gl_FragCoord;
out vec4 FragColor;
void main()
{
    const int xIdx = int(gl_FragCoord.x);
    const int yIdx = int(lutSize - gl_FragCoord.y);
    const int zIdx = ogluLayer;
    const ivec3 lutPos = ivec3(xIdx, yIdx, zIdx);
    const vec3 srcColor = lutPos * step;
    const vec3 linearColor = LogUEToLinear(srcColor);
    const vec3 acesColor = ToneMapping(linearColor);
    const vec3 srgbColor = LinearToSRGB(acesColor);
    const vec4 color = vec4(srgbColor, 1.0f);
    FragColor = color;
}

#endif