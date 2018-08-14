
const sampler_t defsampler = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_NONE | CLK_FILTER_LINEAR;

typedef struct Info
{
    uint SrcWidth;
    uint SrcHeight;
    uint DestWidth;
    uint DestHeight;
    float WidthStep;
    float HeightStep;
}Info;

kernel void ResizeToImg(read_only image2d_t input, write_only image2d_t output, const uint isSrgb, Info info)
{
    private const int coordX = get_global_id(0), coordY = get_global_id(1);
    if (coordX < info.DestWidth && coordY < info.DestHeight)
    {
        private const float2 texPos = (float2)(info.WidthStep * coordX, info.HeightStep * coordY);
        private const float4 color = read_imagef(input, defsampler, texPos);
        write_imagef(output, (int2)(coordX, coordY), color);
    }
}

kernel void ResizeToDat4(read_only image2d_t input, global uchar* restrict output, const uint isSrgb, Info info)
{
    private const int coordX = get_global_id(0), coordY = get_global_id(1);
    if (coordX < info.DestWidth && coordY < info.DestHeight)
    {
        private const float2 texPos = (float2)(info.WidthStep * coordX, info.HeightStep * coordY);
        private const float4 color = read_imagef(input, defsampler, texPos);
        private const uchar4 bcolor = convert_uchar4_sat(color * 255.0f);
        private uint offset = coordY * info.DestWidth + coordX;
        vstore4(bcolor, offset, output);
    }
}
kernel void ResizeToDat3(read_only image2d_t input, global uchar* restrict output, const uint isSrgb, Info info)
{
    private const int coordX = get_global_id(0), coordY = get_global_id(1);
    if (coordX < info.DestWidth && coordY < info.DestHeight)
    {
        private const float2 texPos = (float2)(info.WidthStep * coordX, info.HeightStep * coordY);
        private const float3 color = read_imagef(input, defsampler, texPos).xyz;
        private const uchar3 bcolor = convert_uchar3_sat(color * 255.0f);
        private uint offset = coordY * info.DestWidth + coordX;
        vstore3(bcolor, offset, output);
    }
}