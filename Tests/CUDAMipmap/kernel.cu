
#include "cuda_runtime.h"
#include "cuda_fp16.h"
#include "device_launch_parameters.h"
#include <cstdio>
#include <cmath>
#include "CUDAMipmap.h"

#define uchar uint8_t
#define uchar8 half
#define ushort uint16_t
#define kernel __global__
#define restrict __restrict__
#define global  
#define constant  
#define local __shared__  
#define private  
#define barrier __syncthreads
#define CLK_LOCAL_MEM_FENCE 
inline __device__ float clamp(float f, float a, float b)
{
    return fmaxf(a, fminf(f, b));
}
inline __device__ float4 clamp(float4 v, float a, float b)
{
    return make_float4(clamp(v.x, a, b), clamp(v.y, a, b), clamp(v.z, a, b), clamp(v.w, a, b));
}
inline __device__ void operator*=(float4 &a, float s)
{
    a.x *= s; a.y *= s; a.z *= s; a.w *= s;
}
inline __device__ float4 operator*(float4 a, float s)
{
    return make_float4(a.x * s, a.y * s, a.z * s, a.w * s);
}
inline __device__ float4 operator*(float s, float4 a)
{
    return make_float4(a.x * s, a.y * s, a.z * s, a.w * s);
}
inline __device__ float4 operator+(float4 a, float4 b)
{
    return make_float4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}
inline __device__ void operator+=(float4 &a, float4 b)
{
    a.x += b.x; a.y += b.y; a.z += b.z; a.w += b.w;
}
inline __device__ float4 operator*(uchar4 a, float4 b)
{
    return make_float4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}
inline __device__ float4 operator*(float4 a, uchar4 b)
{
    return make_float4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}
inline __device__ void vstorea_half4(float4 dat, size_t offset, half* __restrict__ ptr)
{
    (ptr + offset * 4)[0] = dat.x;
    (ptr + offset * 4)[1] = dat.x;
    (ptr + offset * 4)[2] = dat.x;
    (ptr + offset * 4)[3] = dat.x;
}
inline __device__ uchar4 convert_uchar4(float4 dat)
{
    return make_uchar4(dat.x, dat.y, dat.z, dat.w);
}


inline __device__ float SRGBToLinear(const float color)
{
    return color <= 0.04045f ? (1.0f / 12.92f) * color : pow((1.0f / 1.055f) * (color + 0.055f), 2.4f);
}
inline __device__ float4 SRGBToLinear(const float4 color)
{
    return make_float4(SRGBToLinear(color.x), SRGBToLinear(color.x), SRGBToLinear(color.x), color.w);
}
inline __device__ float LinearToSRGB(const float color)
{
    return color <= 0.00304f ? 12.92f * color : 1.055f * pow(color, 1.0f / 2.4f) - 0.055f;
}
inline __device__ float4 LinearToSRGB(const float4 color)
{
    return make_float4(LinearToSRGB(color.x), LinearToSRGB(color.x), LinearToSRGB(color.x), color.w);
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

#ifndef CountX
#   define CountX 8
#endif
#ifndef CountY
#   define CountY 8
#endif

__inline__ __device__ float4 loadU4_F4(const uchar4* ptr)
{
    const uchar4 dat = __ldg(ptr);
    return make_float4(dat.x, dat.y, dat.z, dat.w);
}

kernel void Downsample_Src(global const uchar4* restrict src, const Info* info, const uchar level, global uchar8* restrict mid, global uchar4* restrict dst)
{
    const ushort dstX = threadIdx.x + CountX * blockIdx.x, dstY = threadIdx.y + CountY * blockIdx.y;
    const uchar lidX = threadIdx.x, lidY = threadIdx.y, lid = lidY * CountX + lidX;
    local float4 sharedImg1[CountX*CountY], sharedImg2[CountX*CountY];

    const uchar* restrict ptrSrc = (global const uchar*)(src + (dstY * 4) * info[level].SrcWidth + (dstX * 4));

    float4 res[8];
#define LOOP_LINE(line) \
    { \
        float4 tmp1 = SRGBToLinear(loadU4_F4((const uchar4*)(ptrSrc   )) * (1.f/255.f)); \
        float4 tmp2 = SRGBToLinear(loadU4_F4((const uchar4*)(ptrSrc+ 4)) * (1.f/255.f)); \
        float4 tmp3 = SRGBToLinear(loadU4_F4((const uchar4*)(ptrSrc+ 8)) * (1.f/255.f)); \
        float4 tmp4 = SRGBToLinear(loadU4_F4((const uchar4*)(ptrSrc+12)) * (1.f/255.f)); \
        barrier(CLK_LOCAL_MEM_FENCE); \
        sharedImg1[lid] = tmp1; sharedImg2[lid] = tmp4; \
        barrier(CLK_LOCAL_MEM_FENCE); \
        float4 leftPix, rightPix; \
        if (lidX == 0) \
        { \
            if (dstX == 0) \
                leftPix = tmp1; \
            else \
                leftPix = SRGBToLinear(loadU4_F4((const uchar4*)(ptrSrc - 4)) * (1.f/255.f)); \
        } \
        else \
            leftPix = sharedImg2[lid - 1]; \
        if (lidX == CountX-1) \
        { \
            if (dstX == info[level].LimitX) \
                rightPix = tmp4; \
            else \
                rightPix = SRGBToLinear(loadU4_F4((const uchar4*)(ptrSrc + 16)) * (1.f/255.f)); \
        } \
        else \
            rightPix = sharedImg1[lid + 1]; \
        res[line*2  ] = clamp(leftPix   * COEF_D3 + tmp1 * COEF_D1 + tmp2 * COEF_D1 + tmp3      * COEF_D3, 0.f, 1.f); \
        res[line*2+1] = clamp(tmp2      * COEF_D3 + tmp3 * COEF_D1 + tmp4 * COEF_D1 + rightPix  * COEF_D3, 0.f, 1.f); \
        ptrSrc += info[level].SrcWidth * 4; \
    }
    LOOP_LINE(0)
    LOOP_LINE(1)
    LOOP_LINE(2)
    LOOP_LINE(3)
    {
        float4 upPix[2], downPix[2];
        if (lidY == 0)
        {
            if (dstY == 0)
            {
                upPix[0] = res[0]; upPix[1] = res[1];
            }
            else
            {
                float4 tmp1 = SRGBToLinear(loadU4_F4((const uchar4*)(ptrSrc - info[level].SrcWidth * 20)) * (1.f / 255.f));
                float4 tmp2 = SRGBToLinear(loadU4_F4((const uchar4*)(ptrSrc - info[level].SrcWidth * 16)) * (1.f / 255.f));
                float4 tmp3 = SRGBToLinear(loadU4_F4((const uchar4*)(ptrSrc - info[level].SrcWidth * 12)) * (1.f / 255.f));
                float4 tmp4 = SRGBToLinear(loadU4_F4((const uchar4*)(ptrSrc - info[level].SrcWidth *  8)) * (1.f / 255.f));
                upPix[0] = tmp1 * 0.5f + tmp2 * 0.5f; upPix[1] = tmp3 * 0.5f + tmp4 * 0.5f;
            }
        }
        else if (lidY == CountY - 1)
        {
            if (dstY == info[level].LimitY)
            {
                downPix[0] = res[6]; downPix[1] = res[7];
            }
            else
            {
                float4 tmp1 = SRGBToLinear(loadU4_F4((const uchar4*)(ptrSrc +  0)) * (1.f / 255.f));
                float4 tmp2 = SRGBToLinear(loadU4_F4((const uchar4*)(ptrSrc +  4)) * (1.f / 255.f));
                float4 tmp3 = SRGBToLinear(loadU4_F4((const uchar4*)(ptrSrc +  8)) * (1.f / 255.f));
                float4 tmp4 = SRGBToLinear(loadU4_F4((const uchar4*)(ptrSrc + 12)) * (1.f / 255.f));
                downPix[0] = tmp1 * 0.5f + tmp2 * 0.5f; downPix[1] = tmp3 * 0.5f + tmp4 * 0.5f;
            }
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = res[0]; sharedImg2[lid] = res[6];
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY > 0)
            upPix[0] = sharedImg2[lid - CountX];
        if (lidY < CountY - 1)
            downPix[0] = sharedImg1[lid + CountX];
        barrier(CLK_LOCAL_MEM_FENCE);
        sharedImg1[lid] = res[1]; sharedImg2[lid] = res[7];
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lidY > 0)
            upPix[1] = sharedImg2[lid - CountX];
        if (lidY < CountY - 1)
            downPix[1] = sharedImg1[lid + CountX];
        res[0] = clamp(upPix[0] * COEF_D3 + res[0] * COEF_D1 + res[2] * COEF_D1 + res[4] * COEF_D3, 0.f, 1.f);
        res[1] = clamp(upPix[1] * COEF_D3 + res[1] * COEF_D1 + res[3] * COEF_D1 + res[5] * COEF_D3, 0.f, 1.f);
        res[6] = clamp(res[2] * COEF_D3 + res[4] * COEF_D1 + res[6] * COEF_D1 + downPix[0] * COEF_D3, 0.f, 1.f);
        res[7] = clamp(res[3] * COEF_D3 + res[5] * COEF_D1 + res[7] * COEF_D1 + downPix[1] * COEF_D3, 0.f, 1.f);
    }
    global half* restrict ptrMid = (global half*)(mid + (dstY * 2) * info[level].SrcWidth / 2 + (dstX * 2));
    global uchar4* restrict ptrDst = (global uchar4*)(dst + (dstY * 2) * info[level].SrcWidth / 2 + (dstX * 2));
    vstorea_half4(res[0], 0, ptrMid); vstorea_half4(res[1], 1, ptrMid); 
    vstorea_half4(res[6], info[level].SrcWidth / 2, ptrMid); vstorea_half4(res[7], info[level].SrcWidth / 2 + 1, ptrMid);
    res[0] = LinearToSRGB(res[0]); res[1] = LinearToSRGB(res[1]);
    res[6] = LinearToSRGB(res[6]); res[7] = LinearToSRGB(res[7]);
    ptrDst[0] = convert_uchar4(res[0] * 255.0f); ptrDst[1] = convert_uchar4(res[1] * 255.0f);
    ptrDst[info[level].SrcWidth / 2] = convert_uchar4(res[6] * 255.0f); ptrDst[info[level].SrcWidth / 2 + 1] = convert_uchar4(res[7] * 255.0f);
}


#ifdef __cplusplus
extern "C" {
#endif
// Helper function for using CUDA to add vectors in parallel.
void CUDAMIPMAPAPI DoMipmap(const void* src, void* dst, const uint32_t width, const uint32_t height)
{
    uchar4 *dev_src = 0;
    uchar8 *dev_mid = 0;
    Info  *dev_info = 0;
    uchar4 *dev_dst = 0;
    cudaError_t cudaStatus;
    Info info
    {
        static_cast<uint16_t>(width), static_cast<uint16_t>(height),
        static_cast<uint16_t>(width / 4 - 1),static_cast<uint16_t>(height / 4 - 1)
    };


    // Choose which GPU to run on, change this on a multi-GPU system.
    cudaStatus = cudaSetDevice(0);
    if (cudaStatus != cudaSuccess) 
    {
        printf("cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?");
        goto Error;
    }

    cudaStatus = cudaMalloc((void**)&dev_src, width*height * sizeof(uint32_t));
    if (cudaStatus != cudaSuccess) 
    {
        printf("cudaMalloc failed!");
        goto Error;
    }

    cudaStatus = cudaMalloc((void**)&dev_info, sizeof(Info));
    if (cudaStatus != cudaSuccess)
    {
        printf("cudaMalloc failed!");
        goto Error;
    }

    cudaStatus = cudaMalloc((void**)&dev_mid, width*height * sizeof(uint32_t));
    if (cudaStatus != cudaSuccess) 
    {
        printf("cudaMalloc failed!");
        goto Error;
    }

    cudaStatus = cudaMalloc((void**)&dev_dst, width*height * sizeof(uint32_t) / 4);
    if (cudaStatus != cudaSuccess) 
    {
        printf("cudaMalloc failed!");
        goto Error;
    }

    cudaStatus = cudaMemcpy(dev_src, src, width*height * sizeof(uint32_t), cudaMemcpyHostToDevice);
    if (cudaStatus != cudaSuccess) 
    {
        printf("cudaMemcpy failed!");
        goto Error;
    }

    cudaStatus = cudaMemcpy(dev_info, &info, sizeof(Info), cudaMemcpyHostToDevice);
    if (cudaStatus != cudaSuccess)
    {
        printf("cudaMemcpy failed!");
        goto Error;
    }

    // Launch a kernel on the GPU with one thread for each element.
    Downsample_Src<<<CountX, CountY>>>(dev_src, dev_info, 0, dev_mid, dev_dst);

    // Check for any errors launching the kernel
    cudaStatus = cudaGetLastError();
    if (cudaStatus != cudaSuccess) 
    {
        printf("addKernel launch failed: %s\n", cudaGetErrorString(cudaStatus));
        goto Error;
    }
    
    // cudaDeviceSynchronize waits for the kernel to finish, and returns
    // any errors encountered during the launch.
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess) 
    {
        printf("cudaDeviceSynchronize returned error code %d after launching addKernel!\n", cudaStatus);
        goto Error;
    }

    cudaMemcpy(dst, dev_dst, width*height, cudaMemcpyDeviceToHost);

Error:
    cudaFree(dev_src);
    cudaFree(dev_mid);
    cudaFree(dev_dst);
    
    // cudaDeviceReset must be called before exiting in order for profiling and
    // tracing tools such as Nsight and Visual Profiler to show complete traces.
    cudaStatus = cudaDeviceReset();
    if (cudaStatus != cudaSuccess) 
    {
        printf("cudaDeviceReset failed!");
    }
}

#ifdef __cplusplus
}
#endif

