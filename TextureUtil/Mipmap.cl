
typedef struct Info
{
    uint SrcWidth;
    uint SrcHeight;
}Info;

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

#define CountX 8
#define CountY 4
#define ImgRowPix 34

__attribute__((reqd_work_group_size(CountX, CountY, 1)))
kernel void Downsample4x4(global const uchar4* restrict src, constant Info* restrict info, global uchar4* restrict dst)
{
    private const ushort dstX = get_global_id(0), dstY = get_global_id(1);
    private const uchar grpX = get_local_id(0), grpY = get_local_id(1), lid = grpY*CountX+grpX;
    local uchar4 img[ImgRowPix * (CountY*4+2)];
    if (lid == 0)
    {
        for (uint i = 0; i < ImgRowPix * (CountY * 4 + 2); ++i)
            img[i] = (uchar4)(255,255,255,255);
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    local uchar* restrict ptrImg = (local uchar*)(img + (grpY*4+1) * ImgRowPix + (grpX*4+1));
    global const uchar* restrict ptrSrc = (global const uchar*)(src + (dstY*4) * info->SrcWidth + (dstX*4));
    vstore16(vload16(0, ptrSrc                   ), 0, ptrImg);
    vstore16(vload16(0, ptrSrc + info->SrcWidth*4 ), 0, ptrImg + ImgRowPix*4);
    vstore16(vload16(0, ptrSrc + info->SrcWidth*8 ), 0, ptrImg + ImgRowPix*8);
    vstore16(vload16(0, ptrSrc + info->SrcWidth*12), 0, ptrImg + ImgRowPix*12);
    barrier(CLK_LOCAL_MEM_FENCE);
    private const ushort gidX = get_group_id(0), gidY = get_group_id(1);
    if (lid < CountY * 4)
    {
        if (gidX != 0)
        {
            //img[(1 + lid)*ImgRowPix] = src[(lid + gidY * CountY * 4)*info->SrcWidth + 0];
        }
        else // copy left
        {
            img[(1+lid)*ImgRowPix  ] = img[(1+lid)*ImgRowPix+1];
        }
    }
    else
    {

    }
    if (lid == 0) // handle upper
    {
        /*local const uchar* restrict ptrFrom = (local const uchar*)img + ImgRowPix;
        local       uchar* restrict ptrTo   = (local       uchar*)img;
        for(uint i=0; i<CountX; ++i)
            vstore16(vload16(i, ptrFrom), i, ptrTo);
        vstore8(vload8(16, ptrFrom), 16, ptrTo);*/
    }
    else if (lid < CountY * 4 + 1) // handle left&right
    {
        printf("execute [%2d] width [%2d,%2d]\n", lid, grpX, grpY);
        img[lid    *ImgRowPix]   = img[lid    *ImgRowPix+1];
        img[(1+lid)*ImgRowPix-1] = img[(1+lid)*ImgRowPix-2];
    }
    else if (lid == CountY * 4 +1) // handle bottom
    {
        /*local const uchar* restrict ptrFrom = (local const uchar*)img + ImgRowPix * (CountY*4);
        local       uchar* restrict ptrTo   = (local       uchar*)img + ImgRowPix * (CountY*4+1);
        for(uint i=0; i<CountX; ++i)
            vstore16(vload16(i, ptrFrom), i, ptrTo);
        vstore8(vload8(16, ptrFrom), 16, ptrTo);*/
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    event_t evt = async_work_group_copy((global uchar*)dst, (local const uchar*)img, ImgRowPix * (CountY*4+2) * 4, 0);
    wait_group_events(1, &evt);
}

__attribute__((reqd_work_group_size(8, 4, 1)))
kernel void Downsample_Src(global const uchar4* restrict src, const uint SrcWidth, const uint SrcHeight, global uchar8* restrict mid, global uchar4* restrict dst)
{
    private const ushort dstX = get_global_id(0), dstY = get_global_id(1);
    private const ushort gidX = get_group_id(0), gidY = get_group_id(1);
    private const uchar lidX = get_local_id(0), lidY = get_local_id(1), lid = lidY * 8 + lidX;
    local float4 sharedImg1[32], sharedImg2[32];

    private float4 leftPix, rightPix;
    global const uchar* restrict ptrSrc = (global const uchar*)(src + (dstY * 4) * SrcWidth + (dstX * 4));

    float4 res[8];
    {
        float16 tmp = convert_float16(vload16(0, ptrSrc));
        tmp.s012 = SRGBToLinear(tmp.s012); tmp.s456 = SRGBToLinear(tmp.s456); tmp.s89a = SRGBToLinear(tmp.s89a); tmp.scde = SRGBToLinear(tmp.scde);
        sharedImg1[lid] = tmp.s0123; sharedImg2[lid] = tmp.scdef;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidX == 0)
            leftPix = tmp.s0123;
        else
            leftPix = sharedImg2[lid-1];
        if (lidX == 7)
            rightPix = tmp.scdef;
        else
            rightPix = sharedImg1[lid+1];
        res[0] = leftPix   * -0.25f + tmp.s0123 * 0.75f + tmp.s4567 * 0.75f + tmp.s89ab * -0.25f;
        res[1] = tmp.s4567 * -0.25f + tmp.s89ab * 0.75f + tmp.scdef * 0.75f + rightPix  * -0.25f;
    }
    {
        float16 tmp = convert_float16(vload16(0, ptrSrc + SrcWidth * 4));
        tmp.s012 = SRGBToLinear(tmp.s012); tmp.s456 = SRGBToLinear(tmp.s456); tmp.s89a = SRGBToLinear(tmp.s89a); tmp.scde = SRGBToLinear(tmp.scde);
        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = tmp.s0123; sharedImg2[lid] = tmp.scdef;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidX == 0)
            leftPix = tmp.s0123;
        else
            leftPix = sharedImg2[lid-1];
        if (lidX == 7)
            rightPix = tmp.scdef;
        else
            rightPix = sharedImg1[lid+1];
        res[2] = leftPix   * -0.25f + tmp.s0123 * 0.75f + tmp.s4567 * 0.75f + tmp.s89ab * -0.25f;
        res[3] = tmp.s4567 * -0.25f + tmp.s89ab * 0.75f + tmp.scdef * 0.75f + rightPix  * -0.25f;
    }
    {
        float16 tmp = convert_float16(vload16(0, ptrSrc + SrcWidth * 8));
        tmp.s012 = SRGBToLinear(tmp.s012); tmp.s456 = SRGBToLinear(tmp.s456); tmp.s89a = SRGBToLinear(tmp.s89a); tmp.scde = SRGBToLinear(tmp.scde);
        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = tmp.s0123; sharedImg2[lid] = tmp.scdef;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidX == 0)
            leftPix = tmp.s0123;
        else
            leftPix = sharedImg2[lid-1];
        if (lidX == 7)
            rightPix = tmp.scdef;
        else
            rightPix = sharedImg1[lid+1];
        res[4] = leftPix   * -0.25f + tmp.s0123 * 0.75f + tmp.s4567 * 0.75f + tmp.s89ab * -0.25f;
        res[5] = tmp.s4567 * -0.25f + tmp.s89ab * 0.75f + tmp.scdef * 0.75f + rightPix  * -0.25f;
    }
    {
        float16 tmp = convert_float16(vload16(0, ptrSrc + SrcWidth * 12));
        tmp.s012 = SRGBToLinear(tmp.s012); tmp.s456 = SRGBToLinear(tmp.s456); tmp.s89a = SRGBToLinear(tmp.s89a); tmp.scde = SRGBToLinear(tmp.scde);
        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = tmp.s0123; sharedImg2[lid] = tmp.scdef;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidX == 0)
            leftPix = tmp.s0123;
        else
            leftPix = sharedImg2[lid-1];
        if (lidX == 7)
            rightPix = tmp.scdef;
        else
            rightPix = sharedImg1[lid+1];
        res[6] = leftPix   * -0.25f + tmp.s0123 * 0.75f + tmp.s4567 * 0.75f + tmp.s89ab * -0.25f;
        res[7] = tmp.s4567 * -0.25f + tmp.s89ab * 0.75f + tmp.scdef * 0.75f + rightPix  * -0.25f;
    }
    {
        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = res[0]; sharedImg2[lid] = res[6];
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY == 0)
            leftPix = res[0];
        else
            leftPix = sharedImg2[lid-8];
        if (lidY == 3)
            rightPix = res[6];
        else
            rightPix = sharedImg1[lid+8];
        res[0] = leftPix   * -0.25f + res[0] * 0.75f + res[2] * 0.75f + res[4]    * -0.25f;
        res[6] = res[2]    * -0.25f + res[4] * 0.75f + res[6] * 0.75f + rightPix  * -0.25f;
    }
    {
        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = res[1]; sharedImg2[lid] = res[7];
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY == 0)
            leftPix = res[1];
        else
            leftPix = sharedImg2[lid-8];
        if (lidY == 3)
            rightPix = res[7];
        else
            rightPix = sharedImg1[lid+8];
        res[1] = leftPix   * -0.25f + res[1] * 0.75f + res[3] * 0.75f + res[5]    * -0.25f;
        res[7] = res[3]    * -0.25f + res[5] * 0.75f + res[7] * 0.75f + rightPix  * -0.25f;
    }
    //printf("[%d,%d]: 0[%f], 1[%f]\n", lidX, lidY, res[0].x, res[1].x);
    global half* restrict ptrMid = (global half*)(mid + (dstY * 2) * SrcWidth / 2 + (dstX * 2));
    global uchar* restrict ptrDst = (global uchar*)(dst + (dstY * 2) * SrcWidth / 2 + (dstX * 2));
    float8 out1 = (float8)(res[0], res[1]);
    vstorea_half8(out1, 0, ptrMid);
    out1.s012 = LinearToSRGB(out1.s012); out1.s456 = LinearToSRGB(out1.s456);
    vstore8(convert_uchar8(out1), 0, ptrDst);
    out1 = (float8)(res[6], res[7]);
    vstorea_half8(out1, SrcWidth / 4, ptrMid);
    out1.s012 = LinearToSRGB(out1.s012); out1.s456 = LinearToSRGB(out1.s456);
    vstore8(convert_uchar8(out1), SrcWidth / 4, ptrDst);
}