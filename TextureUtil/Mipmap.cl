

float3 SRGBToLinear(const float3 color)
{
    float3 ret;
    ret.x = color.x <= 0.04045f ? (1.0f / 12.92f) * color.x : pow((1.0f / 1.055f) * (color.x + 0.055f), 2.4f);
    ret.y = color.y <= 0.04045f ? (1.0f / 12.92f) * color.y : pow((1.0f / 1.055f) * (color.y + 0.055f), 2.4f);
    ret.z = color.z <= 0.04045f ? (1.0f / 12.92f) * color.z : pow((1.0f / 1.055f) * (color.z + 0.055f), 2.4f);
    return ret;
}
float3 LinearToSRGB(const float3 color)
{
    float3 ret;
    ret.x = color.x <= 0.00304f ? 12.92f * color.x : 1.055f * native_powr(color.x, 1.0f / 2.4f) - 0.055f;
    ret.y = color.y <= 0.00304f ? 12.92f * color.y : 1.055f * native_powr(color.y, 1.0f / 2.4f) - 0.055f;
    ret.z = color.z <= 0.00304f ? 12.92f * color.z : 1.055f * native_powr(color.z, 1.0f / 2.4f) - 0.055f;
    return ret;
}

typedef struct Info
{
    ushort SrcWidth;
    ushort SrcHeight;
    ushort LimitX;
    ushort LimitY;
}Info;

#define COEF_D1  0.5625f
#define COEF_D3 -0.0625f

#define CountX 8
#define CountY 8
__attribute__((reqd_work_group_size(8, 8, 1)))
kernel void Downsample_Src(global const uchar4* restrict src, constant const Info* info, const uchar level, global uchar8* restrict mid, global uchar4* restrict dst)
{
    private const ushort dstX = get_global_id(0), dstY = get_global_id(1);
    private const uchar lidX = get_local_id(0), lidY = get_local_id(1), lid = lidY * CountX + lidX;
    local float4 sharedImg1[CountX*CountY], sharedImg2[CountX*CountY];

    global const uchar* restrict ptrSrc = (global const uchar*)(src + (dstY * 4) * info[level].SrcWidth + (dstX * 4));

    float8 res[4];
    #define LOOP_LINE(line) \
    { \
        private float16 tmp = convert_float16(vload16(0, ptrSrc)) * (1.f/255.f); \
        tmp.s012 = SRGBToLinear(tmp.s012); tmp.s456 = SRGBToLinear(tmp.s456); tmp.s89a = SRGBToLinear(tmp.s89a); tmp.scde = SRGBToLinear(tmp.scde); \
        barrier(CLK_LOCAL_MEM_FENCE); \
        sharedImg1[lid] = tmp.s0123; sharedImg2[lid] = tmp.scdef; \
        barrier(CLK_LOCAL_MEM_FENCE); \
        private float4 leftPix, rightPix; \
        if (lidX == 0) \
        { \
            if (dstX == 0) \
                leftPix = tmp.s0123; \
            else \
            { \
                leftPix = convert_float4(vload4(0, ptrSrc - 4)) * (1.f/255.f); \
                leftPix.xyz = SRGBToLinear(leftPix.xyz); \
            } \
        } \
        else \
            leftPix = sharedImg2[lid - 1]; \
        if (lidX == CountX-1) \
        { \
            if (dstX == info[level].LimitX) \
                rightPix = tmp.scdef; \
            else \
            { \
                rightPix = convert_float4(vload4(0, ptrSrc + 16)) * (1.f/255.f); \
                rightPix.xyz = SRGBToLinear(rightPix.xyz); \
            } \
        } \
        else \
            rightPix = sharedImg1[lid + 1]; \
        res[line].lo = clamp(leftPix   * COEF_D3 + tmp.s0123 * COEF_D1 + tmp.s4567 * COEF_D1 + tmp.s89ab * COEF_D3, 0.f, 1.f); \
        res[line].hi = clamp(tmp.s4567 * COEF_D3 + tmp.s89ab * COEF_D1 + tmp.scdef * COEF_D1 + rightPix  * COEF_D3, 0.f, 1.f); \
        ptrSrc += info[level].SrcWidth * 4; \
    }
    LOOP_LINE(0)
    LOOP_LINE(1)
    LOOP_LINE(2)
    LOOP_LINE(3)
    {
        float8 upPix, downPix;
        if (lidY == 0)
        {
            if (dstY == 0)
            {
                upPix = res[0];
            }
            else
            {
                float16 tmp = convert_float16(vload16(0, ptrSrc - info[level].SrcWidth * 20)) * (1.f/255.f);
                tmp.s012 = SRGBToLinear(tmp.s012); tmp.s456 = SRGBToLinear(tmp.s456); tmp.s89a = SRGBToLinear(tmp.s89a); tmp.scde = SRGBToLinear(tmp.scde);
                upPix = tmp.s012389ab * 0.5f + tmp.s4567cdef * 0.5f;
            }
        }
        else if (lidY == CountY-1)
        {
            if (dstY == info[level].LimitY)
            {
                downPix = res[3];
            }
            else
            {
                float16 tmp = convert_float16(vload16(0, ptrSrc)) * (1.f/255.f);
                tmp.s012 = SRGBToLinear(tmp.s012); tmp.s456 = SRGBToLinear(tmp.s456); tmp.s89a = SRGBToLinear(tmp.s89a); tmp.scde = SRGBToLinear(tmp.scde);
                downPix = tmp.s012389ab * 0.5f + tmp.s4567cdef * 0.5f;
            }
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = res[0].lo; sharedImg2[lid] = res[3].lo;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY > 0)
            upPix.lo = sharedImg2[lid - CountX];
        if (lidY < CountY - 1)
            downPix.lo = sharedImg1[lid + CountX];
        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = res[0].hi; sharedImg2[lid] = res[3].hi;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY > 0)
            upPix.hi = sharedImg2[lid - CountX];
        if (lidY < CountY - 1)
            downPix.hi = sharedImg1[lid + CountX];
        res[0] = clamp(upPix  * COEF_D3 + res[0] * COEF_D1 + res[1] * COEF_D1 + res[2]  * COEF_D3, 0.f, 1.f);
        res[3] = clamp(res[1] * COEF_D3 + res[3] * COEF_D1 + res[3] * COEF_D1 + downPix * COEF_D3, 0.f, 1.f);
    }
    global half* restrict ptrMid = (global half*)(mid + (dstY * 2) * info[level].SrcWidth / 2 + (dstX * 2));
    global uchar* restrict ptrDst = (global uchar*)(dst + (dstY * 2) * info[level].SrcWidth / 2 + (dstX * 2));
    vstorea_half8(res[0], 0, ptrMid); vstorea_half8(res[3], info[level].SrcWidth / 4, ptrMid);
    res[0].s012 = LinearToSRGB(res[0].s012); res[0].s456 = LinearToSRGB(res[0].s456);
    res[3].s012 = LinearToSRGB(res[3].s012); res[3].s456 = LinearToSRGB(res[3].s456);
    vstore8(convert_uchar8(res[0] * 255.f), 0, ptrDst); vstore8(convert_uchar8(res[3] * 255.f), info[level].SrcWidth / 4, ptrDst);
}


__attribute__((reqd_work_group_size(8, 8, 1)))
kernel void Downsample_Mid(global const uchar8* restrict src, constant const Info* info, const uchar level, global uchar8* restrict mid, global uchar4* restrict dst)
{
    private const ushort dstX = get_global_id(0), dstY = get_global_id(1);
    private const uchar lidX = get_local_id(0), lidY = get_local_id(1), lid = lidY * CountX + lidX;
    local float4 sharedImg1[CountX*CountY], sharedImg2[CountX*CountY];

    global const half* restrict ptrSrc = (global const half*)(src + (dstY * 4) * info[level].SrcWidth + (dstX * 4));

    float8 res[4];
    #define LOOP_LINE(line) \
    { \
        private float16 tmp = vloada_half16(0, ptrSrc); \
        barrier(CLK_LOCAL_MEM_FENCE); \
        sharedImg1[lid] = tmp.s0123; sharedImg2[lid] = tmp.scdef; \
        barrier(CLK_LOCAL_MEM_FENCE); \
        private float4 leftPix, rightPix; \
        if (lidX == 0) \
        { \
            if (dstX == 0) \
                leftPix = tmp.s0123; \
            else \
                leftPix = vloada_half4(0, ptrSrc - 4); \
        } \
        else \
            leftPix = sharedImg2[lid - 1]; \
        if (lidX == CountX-1) \
        { \
            if (dstX == info[level].LimitX) \
                rightPix = tmp.scdef; \
            else \
                rightPix = vloada_half4(0, ptrSrc + 16); \
        } \
        else \
            rightPix = sharedImg1[lid + 1]; \
        res[line].lo = clamp(leftPix   * COEF_D3 + tmp.s0123 * COEF_D1 + tmp.s4567 * COEF_D1 + tmp.s89ab * COEF_D3, 0.f, 1.f); \
        res[line].hi = clamp(tmp.s4567 * COEF_D3 + tmp.s89ab * COEF_D1 + tmp.scdef * COEF_D1 + rightPix  * COEF_D3, 0.f, 1.f); \
        ptrSrc += info[level].SrcWidth * 4; \
    }
    LOOP_LINE(0)
    LOOP_LINE(1)
    LOOP_LINE(2)
    LOOP_LINE(3)
    {
        float8 upPix, downPix;
        if (lidY == 0)
        {
            if (dstY == 0)
            {
                upPix = res[0];
            }
            else
            {
                float16 tmp = vloada_half16(0, ptrSrc - info[level].SrcWidth * 20);
                upPix = tmp.s012389ab * 0.5f + tmp.s4567cdef * 0.5f;
            }
        }
        else if (lidY == CountY-1)
        {
            if (dstY == info[level].LimitY)
            {
                downPix = res[3];
            }
            else
            {
                float16 tmp = vloada_half16(0, ptrSrc);
                downPix = tmp.s012389ab * 0.5f + tmp.s4567cdef * 0.5f;
            }
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = res[0].lo; sharedImg2[lid] = res[3].lo;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY > 0)
            upPix.lo = sharedImg2[lid - CountX];
        if (lidY < CountY - 1)
            downPix.lo = sharedImg1[lid + CountX];
        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = res[0].hi; sharedImg2[lid] = res[3].hi;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY > 0)
            upPix.hi = sharedImg2[lid - CountX];
        if (lidY < CountY - 1)
            downPix.hi = sharedImg1[lid + CountX];
        res[0] = clamp(upPix  * COEF_D3 + res[0] * COEF_D1 + res[1] * COEF_D1 + res[2]  * COEF_D3, 0.f, 1.f);
        res[3] = clamp(res[1] * COEF_D3 + res[3] * COEF_D1 + res[3] * COEF_D1 + downPix * COEF_D3, 0.f, 1.f);
    }
    global half* restrict ptrMid = (global half*)(mid + (dstY * 2) * info[level].SrcWidth / 2 + (dstX * 2));
    global uchar* restrict ptrDst = (global uchar*)(dst + (dstY * 2) * info[level].SrcWidth / 2 + (dstX * 2));
    vstorea_half8(res[0], 0, ptrMid); vstorea_half8(res[3], info[level].SrcWidth / 4, ptrMid);
    res[0].s012 = LinearToSRGB(res[0].s012); res[0].s456 = LinearToSRGB(res[0].s456);
    res[3].s012 = LinearToSRGB(res[3].s012); res[3].s456 = LinearToSRGB(res[3].s456);
    vstore8(convert_uchar8(res[0] * 255.f), 0, ptrDst); vstore8(convert_uchar8(res[3] * 255.f), info[level].SrcWidth / 4, ptrDst);
}