#ifdef cl_khr_fp16
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#endif
#ifdef cl_intel_subgroups
#pragma OPENCL EXTENSION cl_intel_subgroups : enable
#endif

typedef struct Info
{
    uint SrcOffset;
    uint DstOffset;
    ushort SrcWidth;
    ushort SrcHeight;
    ushort LimitX;
    ushort LimitY;
}Info;


#ifndef CountX
#   define CountX 8
#endif
#ifndef CountY
#   define CountY 8
#endif


float3 SRGBToLinear(const float3 color)
{
    float3 ret;
    ret.x = color.x <= 0.04045f ? (1.0f / 12.92f) * color.x : native_powr((1.0f / 1.055f) * (color.x + 0.055f), 2.4f);
    ret.y = color.y <= 0.04045f ? (1.0f / 12.92f) * color.y : native_powr((1.0f / 1.055f) * (color.y + 0.055f), 2.4f);
    ret.z = color.z <= 0.04045f ? (1.0f / 12.92f) * color.z : native_powr((1.0f / 1.055f) * (color.z + 0.055f), 2.4f);
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
#define COEF_D1  0.5625f
#define COEF_D3 -0.0625f

#ifdef cl_intel_subgroups
//stage1,SRGB,Subgroup opt
kernel void Downsample_SrcSG(global const uchar4* restrict src, constant const Info* info, const uchar level, global uchar8* restrict mid, global uchar4* restrict dst)
{
    private const uchar lidX = get_sub_group_local_id(), lidY = get_sub_group_id(), lid = lidY * get_sub_group_size() + lidX;
    private const ushort dstX = get_group_id(0)*get_sub_group_size() + lidX, dstY = get_group_id(1)*get_num_sub_groups() + lidY;

    global const uchar* restrict ptrSrc = (global const uchar*)(src + (dstY * 4) * info[level].SrcWidth + (dstX * 4));

    float8 res[4];
    #define LOOP_LINE(line) \
    { \
        private float16 tmp = convert_float16(vload16(0, ptrSrc)) * (1.f/255.f); \
        tmp.s012 = SRGBToLinear(tmp.s012); tmp.s456 = SRGBToLinear(tmp.s456); tmp.s89a = SRGBToLinear(tmp.s89a); tmp.scde = SRGBToLinear(tmp.scde); \
        private float4 leftPix  = intel_sub_group_shuffle(tmp.scdef, lidX-1); \
        private float4 rightPix = intel_sub_group_shuffle(tmp.s0123, lidX+1); \
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
        else if (lidX == get_sub_group_size()-1) \
        { \
            if (dstX == info[level].LimitX) \
                rightPix = tmp.scdef; \
            else \
            { \
                rightPix = convert_float4(vload4(0, ptrSrc + 16)) * (1.f/255.f); \
                rightPix.xyz = SRGBToLinear(rightPix.xyz); \
            } \
        } \
        res[line].lo = clamp(leftPix   * COEF_D3 + tmp.s0123 * COEF_D1 + tmp.s4567 * COEF_D1 + tmp.s89ab * COEF_D3, 0.f, 1.f); \
        res[line].hi = clamp(tmp.s4567 * COEF_D3 + tmp.s89ab * COEF_D1 + tmp.scdef * COEF_D1 + rightPix  * COEF_D3, 0.f, 1.f); \
        ptrSrc += info[level].SrcWidth * 4; \
    }
    LOOP_LINE(0)
    LOOP_LINE(1)
    LOOP_LINE(2)
    LOOP_LINE(3)
    #undef LOOP_LINE
    local float8 sharedImg[64];
    {
        float8 upPix, downPix;
        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg[lid] = res[0];
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY == get_num_sub_groups() - 1)
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
        else
            downPix = sharedImg[lid + get_sub_group_size()];
        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg[lid] = res[3];
        barrier(CLK_LOCAL_MEM_FENCE);
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
        else
            upPix = sharedImg[lid - get_sub_group_size()];

        res[0] = clamp(upPix  * COEF_D3 + res[0] * COEF_D1 + res[1] * COEF_D1 + res[2]  * COEF_D3, 0.f, 1.f);
        res[3] = clamp(res[1] * COEF_D3 + res[2] * COEF_D1 + res[3] * COEF_D1 + downPix * COEF_D3, 0.f, 1.f);
    }
    global half* restrict ptrMid = (global half*)(mid + (dstY * 2) * info[level].SrcWidth / 2 + (dstX * 2));
    global uchar* restrict ptrDst = (global uchar*)(dst + info[level].DstOffset + (dstY * 2) * info[level].SrcWidth / 2 + (dstX * 2));
    vstorea_half8(res[0], 0, ptrMid); vstorea_half8(res[3], info[level].SrcWidth / 4, ptrMid);
    res[0].s012 = LinearToSRGB(res[0].s012); res[0].s456 = LinearToSRGB(res[0].s456);
    res[3].s012 = LinearToSRGB(res[3].s012); res[3].s456 = LinearToSRGB(res[3].s456);
    vstore8(convert_uchar8(res[0] * 255.f), 0, ptrDst); vstore8(convert_uchar8(res[3] * 255.f), info[level].SrcWidth / 4, ptrDst);
}
#endif


//stage1,SRGB
__attribute__((reqd_work_group_size(CountX, CountY, 1)))
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
    #undef LOOP_LINE
    global half* restrict ptrMid = (global half*)(mid + (dstY * 2) * info[level].SrcWidth / 2 + (dstX * 2));
    global uchar* restrict ptrDst = (global uchar*)(dst + info[level].DstOffset + (dstY * 2) * info[level].SrcWidth / 2 + (dstX * 2));
    {
        float8 thePix;
        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = res[3].lo; sharedImg2[lid] = res[3].hi;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY == 0)
        {
            if (dstY == 0)
                thePix = res[0];
            else
            {
                float16 tmp = convert_float16(vload16(0, ptrSrc - info[level].SrcWidth * 20)) * (1.f/255.f);
                tmp.s012 = SRGBToLinear(tmp.s012); tmp.s456 = SRGBToLinear(tmp.s456); tmp.s89a = SRGBToLinear(tmp.s89a); tmp.scde = SRGBToLinear(tmp.scde);
                thePix = tmp.s012389ab * 0.5f + tmp.s4567cdef * 0.5f;
            }
        }
        else
            thePix = (float8)(sharedImg1[lid - CountX], sharedImg2[lid - CountX]);
        thePix = clamp(thePix  * COEF_D3 + res[0] * COEF_D1 + res[1] * COEF_D1 + res[2]  * COEF_D3, 0.f, 1.f);
        vstorea_half8(thePix, 0, ptrMid);
        thePix.s012 = LinearToSRGB(thePix.s012); thePix.s456 = LinearToSRGB(thePix.s456);
        vstore8(convert_uchar8(thePix * 255.f), 0, ptrDst); 

        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = res[0].lo; sharedImg2[lid] = res[0].hi;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY == CountY - 1)
        {
            if (dstY == info[level].LimitY)
                thePix = res[3];
            else
            {
                float16 tmp = convert_float16(vload16(0, ptrSrc)) * (1.f/255.f);
                tmp.s012 = SRGBToLinear(tmp.s012); tmp.s456 = SRGBToLinear(tmp.s456); tmp.s89a = SRGBToLinear(tmp.s89a); tmp.scde = SRGBToLinear(tmp.scde);
                thePix = tmp.s012389ab * 0.5f + tmp.s4567cdef * 0.5f;
            }
        }
        else
            thePix = (float8)(sharedImg1[lid + CountX], sharedImg2[lid + CountX]);
        thePix = clamp(res[1] * COEF_D3 + res[2] * COEF_D1 + res[3] * COEF_D1 + thePix * COEF_D3, 0.f, 1.f);
        vstorea_half8(thePix, info[level].SrcWidth / 4, ptrMid);
        thePix.s012 = LinearToSRGB(thePix.s012); thePix.s456 = LinearToSRGB(thePix.s456);
        vstore8(convert_uchar8(thePix * 255.f), info[level].SrcWidth / 4, ptrDst);
    }
}


//stage2,Linear
__attribute__((reqd_work_group_size(CountX, CountY, 1)))
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
    #undef LOOP_LINE
    global half* restrict ptrMid = (global half*)(mid + (dstY * 2) * info[level].SrcWidth / 2 + (dstX * 2));
    global uchar* restrict ptrDst = (global uchar*)(dst + info[level].DstOffset + (dstY * 2) * info[level].SrcWidth / 2 + (dstX * 2));
    {
        float8 thePix;
        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = res[3].lo; sharedImg2[lid] = res[3].hi;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY == 0)
        {
            if (dstY == 0)
                thePix = res[0];
            else
            {
                float16 tmp = vloada_half16(0, ptrSrc - info[level].SrcWidth * 20);
                thePix = tmp.s012389ab * 0.5f + tmp.s4567cdef * 0.5f;
            }
        }
        else
            thePix = (float8)(sharedImg1[lid - CountX], sharedImg2[lid - CountX]);
        thePix = clamp(thePix  * COEF_D3 + res[0] * COEF_D1 + res[1] * COEF_D1 + res[2]  * COEF_D3, 0.f, 1.f);
        vstorea_half8(thePix, 0, ptrMid);
        thePix.s012 = LinearToSRGB(thePix.s012); thePix.s456 = LinearToSRGB(thePix.s456);
        vstore8(convert_uchar8(thePix * 255.f), 0, ptrDst); 

        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = res[0].lo; sharedImg2[lid] = res[0].hi;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY == CountY - 1)
        {
            if (dstY == info[level].LimitY)
                thePix = res[3];
            else
            {
                float16 tmp = vloada_half16(0, ptrSrc);
                thePix = tmp.s012389ab * 0.5f + tmp.s4567cdef * 0.5f;
            }
        }
        else
            thePix = (float8)(sharedImg1[lid + CountX], sharedImg2[lid + CountX]);
        thePix = clamp(res[1] * COEF_D3 + res[2] * COEF_D1 + res[3] * COEF_D1 + thePix * COEF_D3, 0.f, 1.f);
        vstorea_half8(thePix, info[level].SrcWidth / 4, ptrMid);
        thePix.s012 = LinearToSRGB(thePix.s012); thePix.s456 = LinearToSRGB(thePix.s456);
        vstore8(convert_uchar8(thePix * 255.f), info[level].SrcWidth / 4, ptrDst);
    }
}


//Linear
#define COEF_D1I  (short)9
#define COEF_D3I (short)-1
__attribute__((reqd_work_group_size(CountX, CountY, 1)))
kernel void Downsample_Raw(global const uchar4* restrict src, constant const Info* info, const uchar level, global uchar4* restrict dst)
{
    private const ushort dstX = get_global_id(0), dstY = get_global_id(1);
    private const uchar lidX = get_local_id(0), lidY = get_local_id(1), lid = lidY * CountX + lidX;
    local short4 sharedImg1[CountX*CountY], sharedImg2[CountX*CountY];

    global const uchar* restrict ptrSrc = (global const uchar*)(src + info[level].SrcOffset + (dstY * 4) * info[level].SrcWidth + (dstX * 4));

    short8 res[4];
    #define LOOP_LINE(line) \
    { \
        private short16 tmp = convert_short16(vload16(0, ptrSrc)) << (short)2; \
        barrier(CLK_LOCAL_MEM_FENCE); \
        sharedImg1[lid] = tmp.s0123; sharedImg2[lid] = tmp.scdef; \
        barrier(CLK_LOCAL_MEM_FENCE); \
        private short4 leftPix, rightPix; \
        if (lidX == 0) \
        { \
            if (dstX == 0) \
                leftPix = tmp.s0123; \
            else \
                leftPix = convert_short4(vload4(0, ptrSrc - 4)) << (short)2; \
        } \
        else \
            leftPix = sharedImg2[lid - 1]; \
        if (lidX == CountX-1) \
        { \
            if (dstX == info[level].LimitX) \
                rightPix = tmp.scdef; \
            else \
                rightPix = convert_short4(vload4(0, ptrSrc + 16)) << (short)2; \
        } \
        else \
            rightPix = sharedImg1[lid + 1]; \
        res[line].lo = clamp((leftPix   * COEF_D3I) + (tmp.s0123 * COEF_D1I) + (tmp.s4567 * COEF_D1I) + (tmp.s89ab * COEF_D3I), (short)0, (short)16384) >> (short)4; \
        res[line].hi = clamp((tmp.s4567 * COEF_D3I) + (tmp.s89ab * COEF_D1I) + (tmp.scdef * COEF_D1I) + (rightPix  * COEF_D3I), (short)0, (short)16384) >> (short)4; \
        ptrSrc += info[level].SrcWidth * 4; \
    }
    LOOP_LINE(0)
    LOOP_LINE(1)
    LOOP_LINE(2)
    LOOP_LINE(3)
    #undef LOOP_LINE
    global uchar* restrict ptrDst = (global uchar*)(dst + info[level].DstOffset + (dstY * 2) * info[level].SrcWidth / 2 + (dstX * 2));
    {
        short8 thePix;
        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = res[3].lo; sharedImg2[lid] = res[3].hi;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY == 0)
        {
            if (dstY == 0)
                thePix = res[0];
            else
            {
                short16 tmp = convert_short16(vload16(0, ptrSrc - info[level].SrcWidth * 20));
                thePix = (tmp.s012389ab + tmp.s4567cdef) << (short)1;
            }
        }
        else
            thePix = (short8)(sharedImg1[lid - CountX], sharedImg2[lid - CountX]);
        thePix = clamp((thePix  * COEF_D3I) + (res[0] * COEF_D1I) + (res[1] * COEF_D1I) + (res[2]  * COEF_D3I), (short)0, (short)16384) >> (short)6;
        vstore8(convert_uchar8(thePix), 0, ptrDst);

        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = res[0].lo; sharedImg2[lid] = res[0].hi;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY == CountY - 1)
        {
            if (dstY == info[level].LimitY)
                thePix = res[3];
            else
            {
                short16 tmp = convert_short16(vload16(0, ptrSrc));
                thePix = (tmp.s012389ab + tmp.s4567cdef) << (short)1;
            }
        }
        else
            thePix = (short8)(sharedImg1[lid + CountX], sharedImg2[lid + CountX]);
        thePix = clamp((res[1] * COEF_D3I) + (res[2] * COEF_D1I) + (res[3] * COEF_D1I) + (thePix * COEF_D3I), (short)0, (short)16384) >> (short)6;
        vstore8(convert_uchar8(thePix), info[level].SrcWidth / 4, ptrDst);
    }
}

#ifdef cl_khr_fp16

half3 SRGBToLinearH(const half3 color)
{
    half3 ret;
    ret.x = color.x <= 0.04045h ? (1.0h / 12.92h) * color.x : native_powr((1.0h / 1.055h) * (color.x + 0.055h), 2.4h);
    ret.y = color.y <= 0.04045h ? (1.0h / 12.92h) * color.y : native_powr((1.0h / 1.055h) * (color.y + 0.055h), 2.4h);
    ret.z = color.z <= 0.04045h ? (1.0h / 12.92h) * color.z : native_powr((1.0h / 1.055h) * (color.z + 0.055h), 2.4h);
    return ret;
}
half3 LinearToSRGBH(const half3 color)
{
    half3 ret;
    ret.x = color.x <= 0.00304h ? 12.92h * color.x : 1.055h * native_powr(color.x, 1.0h / 2.4h) - 0.055h;
    ret.y = color.y <= 0.00304h ? 12.92h * color.y : 1.055h * native_powr(color.y, 1.0h / 2.4h) - 0.055h;
    ret.z = color.z <= 0.00304h ? 12.92h * color.z : 1.055h * native_powr(color.z, 1.0h / 2.4h) - 0.055h;
    return ret;
}
#define COEF_D1H  0.5625h
#define COEF_D3H -0.0625h

//stage1,SRGB,fp16 opt
__attribute__((reqd_work_group_size(CountX, CountY, 1)))
kernel void Downsample_SrcH(global const uchar4* restrict src, constant const Info* info, const uchar level, global half4* restrict mid, global uchar4* restrict dst)
{
    private const ushort dstX = get_global_id(0), dstY = get_global_id(1);
    private const uchar lidX = get_local_id(0), lidY = get_local_id(1), lid = lidY * CountX + lidX;
    local half4 sharedImg1[CountX*CountY], sharedImg2[CountX*CountY];

    global const uchar* restrict ptrSrc = (global const uchar*)(src + (dstY * 4) * info[level].SrcWidth + (dstX * 4));

    half8 res[4];
    #define LOOP_LINE(line) \
    { \
        private half16 tmp = convert_half16(vload16(0, ptrSrc)) * (1.h/255.h); \
        tmp.s012 = SRGBToLinearH(tmp.s012); tmp.s456 = SRGBToLinearH(tmp.s456); tmp.s89a = SRGBToLinearH(tmp.s89a); tmp.scde = SRGBToLinearH(tmp.scde); \
        barrier(CLK_LOCAL_MEM_FENCE); \
        sharedImg1[lid] = tmp.s0123; sharedImg2[lid] = tmp.scdef; \
        barrier(CLK_LOCAL_MEM_FENCE); \
        private half4 leftPix, rightPix; \
        if (lidX == 0) \
        { \
            if (dstX == 0) \
                leftPix = tmp.s0123; \
            else \
            { \
                leftPix = convert_half4(vload4(0, ptrSrc - 4)) * (1.h/255.h); \
                leftPix.xyz = SRGBToLinearH(leftPix.xyz); \
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
                rightPix = convert_half4(vload4(0, ptrSrc + 16)) * (1.h/255.h); \
                rightPix.xyz = SRGBToLinearH(rightPix.xyz); \
            } \
        } \
        else \
            rightPix = sharedImg1[lid + 1]; \
        res[line].lo = clamp(leftPix   * COEF_D3H + tmp.s0123 * COEF_D1H + tmp.s4567 * COEF_D1H + tmp.s89ab * COEF_D3H, 0.h, 1.h); \
        res[line].hi = clamp(tmp.s4567 * COEF_D3H + tmp.s89ab * COEF_D1H + tmp.scdef * COEF_D1H + rightPix  * COEF_D3H, 0.h, 1.h); \
        ptrSrc += info[level].SrcWidth * 4; \
    }
    LOOP_LINE(0)
    LOOP_LINE(1)
    LOOP_LINE(2)
    LOOP_LINE(3)
    #undef LOOP_LINE
    global half* restrict ptrMid = (global half*)(mid + (dstY * 2) * info[level].SrcWidth / 2 + (dstX * 2));
    global uchar* restrict ptrDst = (global uchar*)(dst + info[level].DstOffset + (dstY * 2) * info[level].SrcWidth / 2 + (dstX * 2));
    {
        half8 thePix;
        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = res[3].lo; sharedImg2[lid] = res[3].hi;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY == 0)
        {
            if (dstY == 0)
                thePix = res[0];
            else
            {
                half16 tmp = convert_half16(vload16(0, ptrSrc - info[level].SrcWidth * 20)) * (1.h/255.h);
                tmp.s012 = SRGBToLinearH(tmp.s012); tmp.s456 = SRGBToLinearH(tmp.s456); tmp.s89a = SRGBToLinearH(tmp.s89a); tmp.scde = SRGBToLinearH(tmp.scde);
                thePix = tmp.s012389ab * 0.5h + tmp.s4567cdef * 0.5h;
            }
        }
        else
            thePix = (half8)(sharedImg1[lid - CountX], sharedImg2[lid - CountX]);
        thePix = clamp(thePix  * COEF_D3H + res[0] * COEF_D1H + res[1] * COEF_D1H + res[2]  * COEF_D3H, 0.h, 1.h);
        vstore8(thePix, 0, ptrMid);
        thePix.s012 = LinearToSRGBH(thePix.s012); thePix.s456 = LinearToSRGBH(thePix.s456);
        vstore8(convert_uchar8(thePix * 255.h), 0, ptrDst); 

        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = res[0].lo; sharedImg2[lid] = res[0].hi;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY == CountY - 1)
        {
            if (dstY == info[level].LimitY)
                thePix = res[3];
            else
            {
                half16 tmp = convert_half16(vload16(0, ptrSrc)) * (1.h/255.h);
                tmp.s012 = SRGBToLinearH(tmp.s012); tmp.s456 = SRGBToLinearH(tmp.s456); tmp.s89a = SRGBToLinearH(tmp.s89a); tmp.scde = SRGBToLinearH(tmp.scde);
                thePix = tmp.s012389ab * 0.5h + tmp.s4567cdef * 0.5h;
            }
        }
        else
            thePix = (half8)(sharedImg1[lid + CountX], sharedImg2[lid + CountX]);
        thePix = clamp(res[1] * COEF_D3H + res[2] * COEF_D1H + res[3] * COEF_D1H + thePix * COEF_D3H, 0.h, 1.h);
        vstore8(thePix, info[level].SrcWidth / 4, ptrMid);
        thePix.s012 = LinearToSRGBH(thePix.s012); thePix.s456 = LinearToSRGBH(thePix.s456);
        vstore8(convert_uchar8(thePix * 255.h), info[level].SrcWidth / 4, ptrDst);
    }
}

//stage2,Linear,fp16 opt
__attribute__((reqd_work_group_size(CountX, CountY, 1)))
kernel void Downsample_MidH(global const half4* restrict src, constant const Info* info, const uchar level, global half4* restrict mid, global uchar4* restrict dst)
{
    private const ushort dstX = get_global_id(0), dstY = get_global_id(1);
    private const uchar lidX = get_local_id(0), lidY = get_local_id(1), lid = lidY * CountX + lidX;
    local half4 sharedImg1[CountX*CountY], sharedImg2[CountX*CountY];

    global const half* restrict ptrSrc = (global const half*)(src + (dstY * 4) * info[level].SrcWidth + (dstX * 4));

    half8 res[4];
    #define LOOP_LINE(line) \
    { \
        private half16 tmp = vload16(0, ptrSrc); \
        barrier(CLK_LOCAL_MEM_FENCE); \
        sharedImg1[lid] = tmp.s0123; sharedImg2[lid] = tmp.scdef; \
        barrier(CLK_LOCAL_MEM_FENCE); \
        private half4 leftPix, rightPix; \
        if (lidX == 0) \
        { \
            if (dstX == 0) \
                leftPix = tmp.s0123; \
            else \
                leftPix = vload4(0, ptrSrc - 4); \
        } \
        else \
            leftPix = sharedImg2[lid - 1]; \
        if (lidX == CountX-1) \
        { \
            if (dstX == info[level].LimitX) \
                rightPix = tmp.scdef; \
            else \
                rightPix = vload4(0, ptrSrc + 16); \
        } \
        else \
            rightPix = sharedImg1[lid + 1]; \
        res[line].lo = clamp(leftPix   * COEF_D3H + tmp.s0123 * COEF_D1H + tmp.s4567 * COEF_D1H + tmp.s89ab * COEF_D3H, 0.h, 1.h); \
        res[line].hi = clamp(tmp.s4567 * COEF_D3H + tmp.s89ab * COEF_D1H + tmp.scdef * COEF_D1H + rightPix  * COEF_D3H, 0.h, 1.h); \
        ptrSrc += info[level].SrcWidth * 4; \
    }
    LOOP_LINE(0)
    LOOP_LINE(1)
    LOOP_LINE(2)
    LOOP_LINE(3)
    #undef LOOP_LINE
    global half* restrict ptrMid = (global half*)(mid + (dstY * 2) * info[level].SrcWidth / 2 + (dstX * 2));
    global uchar* restrict ptrDst = (global uchar*)(dst + info[level].DstOffset + (dstY * 2) * info[level].SrcWidth / 2 + (dstX * 2));
    {
        half8 thePix;
        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = res[3].lo; sharedImg2[lid] = res[3].hi;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY == 0)
        {
            if (dstY == 0)
                thePix = res[0];
            else
            {
                half16 tmp = vload16(0, ptrSrc - info[level].SrcWidth * 20);
                thePix = tmp.s012389ab * 0.5h + tmp.s4567cdef * 0.5h;
            }
        }
        else
            thePix = (half8)(sharedImg1[lid - CountX], sharedImg2[lid - CountX]);
        thePix = clamp(thePix  * COEF_D3H + res[0] * COEF_D1H + res[1] * COEF_D1H + res[2]  * COEF_D3H, 0.h, 1.h);
        vstore8(thePix, 0, ptrMid);
        thePix.s012 = LinearToSRGBH(thePix.s012); thePix.s456 = LinearToSRGBH(thePix.s456);
        vstore8(convert_uchar8(thePix * 255.h), 0, ptrDst); 

        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = res[0].lo; sharedImg2[lid] = res[0].hi;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY == CountY - 1)
        {
            if (dstY == info[level].LimitY)
                thePix = res[3];
            else
            {
                half16 tmp = vload16(0, ptrSrc);
                thePix = tmp.s012389ab * 0.5h + tmp.s4567cdef * 0.5h;
            }
        }
        else
            thePix = (half8)(sharedImg1[lid + CountX], sharedImg2[lid + CountX]);
        thePix = clamp(res[1] * COEF_D3H + res[2] * COEF_D1H + res[3] * COEF_D1H + thePix * COEF_D3H, 0.h, 1.h);
        vstore8(thePix, info[level].SrcWidth / 4, ptrMid);
        thePix.s012 = LinearToSRGBH(thePix.s012); thePix.s456 = LinearToSRGBH(thePix.s456);
        vstore8(convert_uchar8(thePix * 255.h), info[level].SrcWidth / 4, ptrDst);
    }
}


//Linear,fp16 opt
__attribute__((reqd_work_group_size(CountX, CountY, 1)))
kernel void Downsample_RawH(global const uchar4* restrict src, constant const Info* info, const uchar level, global uchar4* restrict dst)
{
    private const ushort dstX = get_global_id(0), dstY = get_global_id(1);
    private const uchar lidX = get_local_id(0), lidY = get_local_id(1), lid = lidY * CountX + lidX;
    local half4 sharedImg1[CountX*CountY], sharedImg2[CountX*CountY];

    global const uchar* restrict ptrSrc = (global const uchar*)(src + info[level].SrcOffset + (dstY * 4) * info[level].SrcWidth + (dstX * 4));

    half8 res[4];
    #define LOOP_LINE(line) \
    { \
        private half16 tmp = convert_half16(vload16(0, ptrSrc)); \
        barrier(CLK_LOCAL_MEM_FENCE); \
        sharedImg1[lid] = tmp.s0123; sharedImg2[lid] = tmp.scdef; \
        barrier(CLK_LOCAL_MEM_FENCE); \
        private half4 leftPix, rightPix; \
        if (lidX == 0) \
        { \
            if (dstX == 0) \
                leftPix = tmp.s0123; \
            else \
                leftPix = convert_half4(vload4(0, ptrSrc - 4)); \
        } \
        else \
            leftPix = sharedImg2[lid - 1]; \
        if (lidX == CountX-1) \
        { \
            if (dstX == info[level].LimitX) \
                rightPix = tmp.scdef; \
            else \
                rightPix = convert_half4(vload4(0, ptrSrc + 16)); \
        } \
        else \
            rightPix = sharedImg1[lid + 1]; \
        res[line].lo = clamp(leftPix   * COEF_D3H + tmp.s0123 * COEF_D1H + tmp.s4567 * COEF_D1H + tmp.s89ab * COEF_D3H, 0.h, 255.h); \
        res[line].hi = clamp(tmp.s4567 * COEF_D3H + tmp.s89ab * COEF_D1H + tmp.scdef * COEF_D1H + rightPix  * COEF_D3H, 0.h, 255.h); \
        ptrSrc += info[level].SrcWidth * 4; \
    }
    LOOP_LINE(0)
    LOOP_LINE(1)
    LOOP_LINE(2)
    LOOP_LINE(3)
    #undef LOOP_LINE
    global uchar* restrict ptrDst = (global uchar*)(dst + info[level].DstOffset + (dstY * 2) * info[level].SrcWidth / 2 + (dstX * 2));
    {
        half8 thePix;
        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = res[3].lo; sharedImg2[lid] = res[3].hi;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY == 0)
        {
            if (dstY == 0)
                thePix = res[0];
            else
            {
                half16 tmp = convert_half16(vload16(0, ptrSrc - info[level].SrcWidth * 20));
                thePix = tmp.s012389ab * 0.5h + tmp.s4567cdef * 0.5h;
            }
        }
        else
            thePix = (half8)(sharedImg1[lid - CountX], sharedImg2[lid - CountX]);
        thePix = clamp(thePix  * COEF_D3H + res[0] * COEF_D1H + res[1] * COEF_D1H + res[2]  * COEF_D3H, 0.h, 255.h);
        vstore8(convert_uchar8(thePix), 0, ptrDst);

        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = res[0].lo; sharedImg2[lid] = res[0].hi;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY == CountY - 1)
        {
            if (dstY == info[level].LimitY)
                thePix = res[3];
            else
            {
                half16 tmp = convert_half16(vload16(0, ptrSrc));
                thePix = tmp.s012389ab * 0.5h + tmp.s4567cdef * 0.5h;
            }
        }
        else
            thePix = (half8)(sharedImg1[lid + CountX], sharedImg2[lid + CountX]);
        thePix = clamp(res[1] * COEF_D3H + res[2] * COEF_D1H + res[3] * COEF_D1H + thePix * COEF_D3H, 0.h, 255.h);
        vstore8(convert_uchar8(thePix), info[level].SrcWidth / 4, ptrDst);
    }
}
#endif


//stage1,SRGB,share-mem opt
// much slower to use local mem
__attribute__((reqd_work_group_size(CountX, CountY, 1)))
kernel void Downsample_SrcSM(global const uchar4* restrict src, constant const Info* info, const uchar level, global uchar8* restrict mid, global uchar4* restrict dst)
{
    private const ushort dstX = get_global_id(0), dstY = get_global_id(1);
    private const uchar lidX = get_local_id(0), lidY = get_local_id(1), lid = lidY * CountX + lidX;
    const uint Count = CountX*CountY;
    local float4 sharedImg[CountX*CountY*8];


    global const uchar* restrict ptrSrc = (global const uchar*)(src + (dstY * 4) * info[level].SrcWidth + (dstX * 4));
    for(uchar i=0; i<4; ++i)
    {
        private float16 tmp = convert_float16(vload16(0, ptrSrc)) * (1.f/255.f);
        tmp.s012 = SRGBToLinear(tmp.s012); tmp.s456 = SRGBToLinear(tmp.s456); tmp.s89a = SRGBToLinear(tmp.s89a); tmp.scde = SRGBToLinear(tmp.scde);
        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg[Count*6+lid] = tmp.s0123; sharedImg[Count*7+lid] = tmp.scdef;
        barrier(CLK_LOCAL_MEM_FENCE);
        private float4 leftPix, rightPix;
        if (lidX == 0)
        {
            if (dstX == 0)
                leftPix = tmp.s0123;
            else
            {
                leftPix = convert_float4(vload4(0, ptrSrc - 4)) * (1.f/255.f);
                leftPix.xyz = SRGBToLinear(leftPix.xyz);
            }
        }
        else
            leftPix = sharedImg[Count*7+lid-1];
        if (lidX == CountX-1)
        {
            if (dstX == info[level].LimitX)
                rightPix = tmp.scdef;
            else
            {
                rightPix = convert_float4(vload4(0, ptrSrc + 16)) * (1.f/255.f);
                rightPix.xyz = SRGBToLinear(rightPix.xyz);
            }
        }
        else
            rightPix = sharedImg[Count*6+lid+1];
        sharedImg[Count*2*i    +lid] = clamp(leftPix   * COEF_D3 + tmp.s0123 * COEF_D1 + tmp.s4567 * COEF_D1 + tmp.s89ab * COEF_D3, 0.f, 1.f);
        sharedImg[Count*(2*i+1)+lid] = clamp(tmp.s4567 * COEF_D3 + tmp.s89ab * COEF_D1 + tmp.scdef * COEF_D1 + rightPix  * COEF_D3, 0.f, 1.f);
        ptrSrc += info[level].SrcWidth * 4;
    }
    float8 upPix, downPix;
    {
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY == CountY - 1)
        {
            if (dstY == info[level].LimitY)
            {
                downPix = (float8)(sharedImg[Count*6+lid], sharedImg[Count*7+lid]);
            }
            else
            {
                float16 tmp = convert_float16(vload16(0, ptrSrc)) * (1.f/255.f);
                tmp.s012 = SRGBToLinear(tmp.s012); tmp.s456 = SRGBToLinear(tmp.s456); tmp.s89a = SRGBToLinear(tmp.s89a); tmp.scde = SRGBToLinear(tmp.scde);
                downPix = tmp.s012389ab * 0.5f + tmp.s4567cdef * 0.5f;
            }
        }
        else
            downPix.lo = sharedImg[lid+CountX], downPix.hi = sharedImg[Count+lid+CountX];
        if (lidY == 0)
        {
            if (dstY == 0)
            {
                upPix = (float8)(sharedImg[lid], sharedImg[Count+lid]);
            }
            else
            {
                float16 tmp = convert_float16(vload16(0, ptrSrc - info[level].SrcWidth * 20)) * (1.f/255.f);
                tmp.s012 = SRGBToLinear(tmp.s012); tmp.s456 = SRGBToLinear(tmp.s456); tmp.s89a = SRGBToLinear(tmp.s89a); tmp.scde = SRGBToLinear(tmp.scde);
                upPix = tmp.s012389ab * 0.5f + tmp.s4567cdef * 0.5f;
            }
        }
        else
            upPix.lo = sharedImg[Count*6+lid-CountX], upPix.hi = sharedImg[Count*7+lid-CountX];

        upPix.lo   = clamp(upPix.lo   * COEF_D3 + sharedImg[Count*0+lid] * COEF_D1 + sharedImg[Count*2+lid] * COEF_D1 + sharedImg[Count*4+lid] * COEF_D3, 0.f, 1.f);
        upPix.hi   = clamp(upPix.hi   * COEF_D3 + sharedImg[Count*1+lid] * COEF_D1 + sharedImg[Count*3+lid] * COEF_D1 + sharedImg[Count*5+lid] * COEF_D3, 0.f, 1.f);
        downPix.lo = clamp(sharedImg[Count*2+lid] * COEF_D3 + sharedImg[Count*4+lid] * COEF_D1 + sharedImg[Count*6+lid] * COEF_D1 + downPix.lo * COEF_D3, 0.f, 1.f);
        downPix.hi = clamp(sharedImg[Count*3+lid] * COEF_D3 + sharedImg[Count*5+lid] * COEF_D1 + sharedImg[Count*7+lid] * COEF_D1 + downPix.hi * COEF_D3, 0.f, 1.f);
    }
    global half* restrict ptrMid = (global half*)(mid + (dstY * 2) * info[level].SrcWidth / 2 + (dstX * 2));
    global uchar* restrict ptrDst = (global uchar*)(dst + info[level].DstOffset + (dstY * 2) * info[level].SrcWidth / 2 + (dstX * 2));
    vstorea_half8(upPix, 0, ptrMid); vstorea_half8(downPix, info[level].SrcWidth / 4, ptrMid);
    upPix.s012 = LinearToSRGB(upPix.s012); upPix.s456 = LinearToSRGB(upPix.s456);
    downPix.s012 = LinearToSRGB(downPix.s012); downPix.s456 = LinearToSRGB(downPix.s456);
    vstore8(convert_uchar8(upPix * 255.f), 0, ptrDst); vstore8(convert_uchar8(downPix * 255.f), info[level].SrcWidth / 4, ptrDst);
}
