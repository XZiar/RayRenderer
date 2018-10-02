
typedef struct Info
{
    uint SrcWidth;
    uint SrcHeight;
}Info;

#define CountX 8
#define CountY 4
constant uint ImgRowPix = CountX*4+2;

__attribute__((reqd_work_group_size(CountX, CountY, 1)))
kernel void Downsample4x4(global const uchar4* restrict src, constant Info* restrict info, global uchar4* restrict dst)
{
    private const ushort dstX = get_global_id(0), dstY = get_global_id(1);
    private const uchar grpX = get_local_id(0), grpY = get_local_id(1), lid = grpY*CountX+grpX;
    local uchar4 img[ImgRowPix * (CountY*4+2)];
    local uchar* restrict ptrImg = (local uchar*)(img + (grpY*4+1) * ImgRowPix + (grpX*4+1));
    global const uchar* restrict ptrSrc = (global const uchar*)(src + (dstY*4) * info->SrcWidth + (dstX*4));
    vstore16(vload16(0,                ptrSrc), 0, ptrImg);
    vstore16(vload16(info->SrcWidth,   ptrSrc), 0, ptrImg + ImgRowPix*4);
    vstore16(vload16(info->SrcWidth*2, ptrSrc), 0, ptrImg + ImgRowPix*8);
    vstore16(vload16(info->SrcWidth*3, ptrSrc), 0, ptrImg + ImgRowPix*12);
    barrier(CLK_LOCAL_MEM_FENCE);
    if (lid == CountY) // handle upper
    {
        local const uchar* restrict ptrFrom = (local const uchar*)img + ImgRowPix;
        local       uchar* restrict ptrTo   = (local       uchar*)img;
        for(uint i=0; i<CountX; ++i)
            vstore16(vload16(i, ptrFrom), i, ptrTo);
        vstore8(vload8(16, ptrFrom), 16, ptrTo);
    }
    else if (lid == CountY+1) // handle bottom
    {
        local const uchar* restrict ptrFrom = (local const uchar*)img + ImgRowPix * (CountY*4);
        local       uchar* restrict ptrTo   = (local       uchar*)img + ImgRowPix * (CountY*4+1);
        for(uint i=0; i<CountX; ++i)
            vstore16(vload16(i, ptrFrom), i, ptrTo);
        vstore8(vload8(16, ptrFrom), 16, ptrTo);
    }
    else // handle left&right
    {
        img[lid*ImgRowPix]       = img[lid*ImgRowPix+1];
        img[lid*(ImgRowPix+1)-1] = img[lid*(ImgRowPix+1)-2];
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    event_t evt = async_work_group_copy((global uchar*)dst, (local const uchar*)img, ImgRowPix * (CountY*4+2) * 4, 0);
    wait_group_events(1, &evt);
}