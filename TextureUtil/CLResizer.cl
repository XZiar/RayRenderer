
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

kernel void resizer(read_only image2d_t input, write_only image2d_t output, const uint isSrgb, constant const Info *info)
{
    private const int coordX = get_global_id(0), coordY = get_global_id(1);
    private const float2 texPos = (float2)(info[0].WidthStep * coordX, info[0].HeightStep * coordY);
    private const float4 color = read_imagef(input, defsampler, texPos);
    write_imagef(output, (int2)(coordX, coordY), color);
}
