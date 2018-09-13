//#pragma OPENCL EXTENSION cl_khr_3d_image_writes : enable

//Color space conversion

float3 LinearToSRGB(const float3 color)
{
    return color <= 0.00304f ? 12.92f * color : 
        1.055f * native_powr(color, (float3)(1.0f/2.4f)) - 0.055f;
}
float3 SRGBToLinear(const float3 color)
{
    return color <= 0.04045f ? (1.0f/12.92f) * color : 
        native_powr((1.0f/1.055f) * (color + 0.055f), (float3)(2.4f));
}

//Tonemappings

float3 ToneMappingACES(const float3 color, const float exposure)
{
    const float A = 2.51f;
    const float B = 0.03f;
    const float C = 2.43f;
    const float D = 0.59f;
    const float E = 0.14f;
    const float3 lum = exposure * color;
    return (lum * (A * lum + B)) / (lum * (C * lum + D) + E);
}
float3 ToneMappingReinhard(const float3 color, const float exposure)
{
    const float3 lum = exposure * color;
    return lum / (lum + 1.0f);
}
float3 ToneMappingEXP(const float3 color, const float exposure)
{
    return 1.0f - native_exp(color * -exposure);
}

//Range compression

float3 LinearToLogP1(const float3 val)
{
    return val / (val+1.0f);
}
float3 LogP1ToLinear(const float3 val)
{
    return val / (1.0f-val);
}

kernel void GenerateLUT_ACES_SRGB(const float step, write_only image3d_t output, const float exposure)
{
    const int4 coord = (int4)(get_global_id(0), get_global_id(1), get_global_id(2), 0);
    const float3 color01 = convert_float3(coord.xyz) * step;
    const float3 linearColor = LogP1ToLinear(color01);
    const float3 acesColor = ToneMappingACES(linearColor, exposure);
    const float3 srgbColor = LinearToSRGB(acesColor);
    const float4 color = (float4)(srgbColor, 1.0f);
    write_imagef(output, coord, color);
}

kernel void GenerateLUT_REINHARD_SRGB(const float step, write_only image3d_t output, const float exposure)
{
    const int4 coord = (int4)(get_global_id(0), get_global_id(1), get_global_id(2), 0);
    const float3 color01 = convert_float3(coord.xyz) * step;
    const float3 linearColor = LogP1ToLinear(color01);
    const float3 acesColor = ToneMappingReinhard(linearColor, exposure);
    const float3 srgbColor = LinearToSRGB(acesColor);
    const float4 color = (float4)(srgbColor, 1.0f);
    write_imagef(output, coord, color);
}

kernel void GenerateLUT_EXP_SRGB(const float step, write_only image3d_t output, const float exposure)
{
    const int4 coord = (int4)(get_global_id(0), get_global_id(1), get_global_id(2), 0);
    const float3 color01 = convert_float3(coord.xyz) * step;
    const float3 linearColor = LogP1ToLinear(color01);
    const float3 acesColor = ToneMappingEXP(linearColor, exposure);
    const float3 srgbColor = LinearToSRGB(acesColor);
    const float4 color = (float4)(srgbColor, 1.0f);
    write_imagef(output, coord, color);
}