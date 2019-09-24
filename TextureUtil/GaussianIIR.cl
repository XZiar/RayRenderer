#ifdef cl_khr_fp16
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#endif


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
uchar4 LinearToSRGBA8(const float4 color)
{
    return convert_uchar4((float4)(LinearToSRGB(color.xyz), color.w));
}

//stageX,Linear
kernel void IIR_RawX(global const uchar* restrict src, global float* restrict mid, global float* restrict dst,
                  const uint width, const uint height, const uint stride, const float4 coeff)
{
    float16 in4, tmp;
    {
        global const uchar * restrict ptrIn  = src + get_global_id(0) * stride * 4;
        global       float * restrict ptrOut = mid + get_global_id(0) * width * 4;
        in4.s0123 = convert_float4(vload4(0, ptrIn));
        tmp = (float16)(in4.s0123, in4.s0123, in4.s0123, in4.s0123);
        for (uint i=0, j=0; j < width; j+=4)
        {
            in4 = convert_float16(vload16(i, ptrIn));
            tmp.s0123 = in4.s0123 * coeff.x + tmp.scdef * coeff.y + tmp.s89ab * coeff.z + tmp.s4567 * coeff.w;
            tmp.s4567 = in4.s4567 * coeff.x + tmp.s0123 * coeff.y + tmp.scdef * coeff.z + tmp.s89ab * coeff.w;
            tmp.s89ab = in4.s89ab * coeff.x + tmp.s4567 * coeff.y + tmp.s0123 * coeff.z + tmp.scdef * coeff.w;
            tmp.scdef = in4.scdef * coeff.x + tmp.s89ab * coeff.y + tmp.s4567 * coeff.z + tmp.s0123 * coeff.w;
            vstore16(tmp, i++, ptrOut);
        }
    }
    {
        global const float * restrict ptrIn  = mid + get_global_id(0) * width * 4;
        global       float * restrict ptrOut = dst + get_global_id(0)*4 + height * (width-1) * 4;
        in4.scdef = vload4(width-1, ptrIn);
        tmp = (float16)(in4.scdef, in4.scdef, in4.scdef, in4.scdef);
        for (uint i = width/4-1, j=0; j < width; j+=4)
        {
            in4 = vload16(i--, ptrIn);
            tmp.scdef = in4.scdef * coeff.x + tmp.s0123 * coeff.y + tmp.s4567 * coeff.z + tmp.s89ab * coeff.w;
            vstore4(tmp.scdef, 0, ptrOut); ptrOut -= height * 4;
            tmp.s89ab = in4.s89ab * coeff.x + tmp.scdef * coeff.y + tmp.s0123 * coeff.z + tmp.s4567 * coeff.w;
            vstore4(tmp.s89ab, 0, ptrOut); ptrOut -= height * 4;
            tmp.s4567 = in4.s4567 * coeff.x + tmp.s89ab * coeff.y + tmp.scdef * coeff.z + tmp.s0123 * coeff.w;
            vstore4(tmp.s4567, 0, ptrOut); ptrOut -= height * 4;
            tmp.s0123 = in4.s0123 * coeff.x + tmp.s4567 * coeff.y + tmp.s89ab * coeff.z + tmp.scdef * coeff.w;
            vstore4(tmp.s0123, 0, ptrOut); ptrOut -= height * 4;
        }
    }
}


//stageY,Linear
kernel void IIR_RawY(global const float* restrict src, global float* restrict mid, global uchar* restrict dst,
                  const uint width, const uint height, const uint stride, const float4 coeff)
{
    float16 in4, tmp;
    {
        global const float * restrict ptrIn  = src + get_global_id(0) * height * 4;
        global       float * restrict ptrOut = mid + get_global_id(0) * height * 4;
        in4.s0123 = vload4(0, ptrIn);
        tmp = (float16)(in4.s0123, in4.s0123, in4.s0123, in4.s0123);
        for (uint i=0, j=0; j < height; j+=4)
        {
            in4 = vload16(i, ptrIn);
            tmp.s0123 = in4.s0123 * coeff.x + tmp.scdef * coeff.y + tmp.s89ab * coeff.z + tmp.s4567 * coeff.w;
            tmp.s4567 = in4.s4567 * coeff.x + tmp.s0123 * coeff.y + tmp.scdef * coeff.z + tmp.s89ab * coeff.w;
            tmp.s89ab = in4.s89ab * coeff.x + tmp.s4567 * coeff.y + tmp.s0123 * coeff.z + tmp.scdef * coeff.w;
            tmp.scdef = in4.scdef * coeff.x + tmp.s89ab * coeff.y + tmp.s4567 * coeff.z + tmp.s0123 * coeff.w;
            vstore16(tmp, i++, ptrOut);
        }
    }
    {
        global const float * restrict ptrIn  = mid + get_global_id(0) * height * 4;
        global       uchar * restrict ptrOut = dst + get_global_id(0)*4 + stride * (height-1) * 4;
        in4.scdef = vload4(height-1, ptrIn);
        tmp = (float16)(in4.scdef, in4.scdef, in4.scdef, in4.scdef);
        for (uint i=height/4-1,j=0; j < height; j+=4)
        {
            in4 = vload16(i--, ptrIn);
            tmp.scdef = in4.scdef * coeff.x + tmp.s0123 * coeff.y + tmp.s4567 * coeff.z + tmp.s89ab * coeff.w;
            vstore4(convert_uchar4(tmp.scdef), 0, ptrOut); ptrOut -= stride * 4;
            tmp.s89ab = in4.s89ab * coeff.x + tmp.scdef * coeff.y + tmp.s0123 * coeff.z + tmp.s4567 * coeff.w;
            vstore4(convert_uchar4(tmp.s89ab), 0, ptrOut); ptrOut -= stride * 4;
            tmp.s4567 = in4.s4567 * coeff.x + tmp.s89ab * coeff.y + tmp.scdef * coeff.z + tmp.s0123 * coeff.w;
            vstore4(convert_uchar4(tmp.s4567), 0, ptrOut); ptrOut -= stride * 4;
            tmp.s0123 = in4.s0123 * coeff.x + tmp.s4567 * coeff.y + tmp.s89ab * coeff.z + tmp.scdef * coeff.w;
            vstore4(convert_uchar4(tmp.s0123), 0, ptrOut); ptrOut -= stride * 4;
        }
    }
}


//stageX,SRGB,Linear
kernel void IIR_SRGBX(global const uchar* restrict src, global float* restrict mid, global float* restrict dst,
                  const uint width, const uint height, const uint stride, const float4 coeff)
{
    float16 in4, tmp;
    {
        global const uchar * restrict ptrIn  = src + get_global_id(0) * stride * 4;
        global       float * restrict ptrOut = mid + get_global_id(0) * width * 4;
        in4.s0123 = convert_float4(vload4(0, ptrIn));
        tmp = (float16)(in4.s0123, in4.s0123, in4.s0123, in4.s0123);
        for (uint i=0, j=0; j < width; j+=4)
        {
            in4 = convert_float16(vload16(i, ptrIn));
            in4.s012 = SRGBToLinear(in4.s012); in4.s456 = SRGBToLinear(in4.s456); in4.s89a = SRGBToLinear(in4.s89a); in4.scde = SRGBToLinear(in4.scde);
            tmp.s0123 = in4.s0123 * coeff.x + tmp.scdef * coeff.y + tmp.s89ab * coeff.z + tmp.s4567 * coeff.w;
            tmp.s4567 = in4.s4567 * coeff.x + tmp.s0123 * coeff.y + tmp.scdef * coeff.z + tmp.s89ab * coeff.w;
            tmp.s89ab = in4.s89ab * coeff.x + tmp.s4567 * coeff.y + tmp.s0123 * coeff.z + tmp.scdef * coeff.w;
            tmp.scdef = in4.scdef * coeff.x + tmp.s89ab * coeff.y + tmp.s4567 * coeff.z + tmp.s0123 * coeff.w;
            vstore16(tmp, i++, ptrOut);
        }
    }
    {
        global const float * restrict ptrIn  = mid + get_global_id(0) * width * 4;
        global       float * restrict ptrOut = dst + get_global_id(0)*4 + height * (width-1) * 4;
        in4.scdef = vload4(width-1, ptrIn);
        tmp = (float16)(in4.scdef, in4.scdef, in4.scdef, in4.scdef);
        for (uint i = width/4-1, j=0; j < width; j+=4)
        {
            in4 = vload16(i--, ptrIn);
            tmp.scdef = in4.scdef * coeff.x + tmp.s0123 * coeff.y + tmp.s4567 * coeff.z + tmp.s89ab * coeff.w;
            vstore4(tmp.scdef, 0, ptrOut); ptrOut -= height * 4;
            tmp.s89ab = in4.s89ab * coeff.x + tmp.scdef * coeff.y + tmp.s0123 * coeff.z + tmp.s4567 * coeff.w;
            vstore4(tmp.s89ab, 0, ptrOut); ptrOut -= height * 4;
            tmp.s4567 = in4.s4567 * coeff.x + tmp.s89ab * coeff.y + tmp.scdef * coeff.z + tmp.s0123 * coeff.w;
            vstore4(tmp.s4567, 0, ptrOut); ptrOut -= height * 4;
            tmp.s0123 = in4.s0123 * coeff.x + tmp.s4567 * coeff.y + tmp.s89ab * coeff.z + tmp.scdef * coeff.w;
            vstore4(tmp.s0123, 0, ptrOut); ptrOut -= height * 4;
        }
    }
}


//stageY,SRGB,Linear
kernel void IIR_SRGBY(global const float* restrict src, global float* restrict mid, global uchar* restrict dst,
                  const uint width, const uint height, const uint stride, const float4 coeff)
{
    float16 in4, tmp;
    {
        global const float * restrict ptrIn  = src + get_global_id(0) * height * 4;
        global       float * restrict ptrOut = mid + get_global_id(0) * height * 4;
        in4.s0123 = vload4(0, ptrIn);
        tmp = (float16)(in4.s0123, in4.s0123, in4.s0123, in4.s0123);
        for (uint i=0, j=0; j < height; j+=4)
        {
            in4 = vload16(i, ptrIn);
            tmp.s0123 = in4.s0123 * coeff.x + tmp.scdef * coeff.y + tmp.s89ab * coeff.z + tmp.s4567 * coeff.w;
            tmp.s4567 = in4.s4567 * coeff.x + tmp.s0123 * coeff.y + tmp.scdef * coeff.z + tmp.s89ab * coeff.w;
            tmp.s89ab = in4.s89ab * coeff.x + tmp.s4567 * coeff.y + tmp.s0123 * coeff.z + tmp.scdef * coeff.w;
            tmp.scdef = in4.scdef * coeff.x + tmp.s89ab * coeff.y + tmp.s4567 * coeff.z + tmp.s0123 * coeff.w;
            vstore16(tmp, i++, ptrOut);
        }
    }
    {
        global const float * restrict ptrIn  = mid + get_global_id(0) * height * 4;
        global       uchar * restrict ptrOut = dst + get_global_id(0)*4 + stride * (height-1) * 4;
        in4.scdef = vload4(height-1, ptrIn);
        tmp = (float16)(in4.scdef, in4.scdef, in4.scdef, in4.scdef);
        for (uint i=height/4-1,j=0; j < height; j+=4)
        {
            in4 = vload16(i--, ptrIn);
            tmp.scdef = in4.scdef * coeff.x + tmp.s0123 * coeff.y + tmp.s4567 * coeff.z + tmp.s89ab * coeff.w;
            vstore4(LinearToSRGBA8(tmp.scdef), 0, ptrOut); ptrOut -= stride * 4;
            tmp.s89ab = in4.s89ab * coeff.x + tmp.scdef * coeff.y + tmp.s0123 * coeff.z + tmp.s4567 * coeff.w;
            vstore4(LinearToSRGBA8(tmp.s89ab), 0, ptrOut); ptrOut -= stride * 4;
            tmp.s4567 = in4.s4567 * coeff.x + tmp.s89ab * coeff.y + tmp.scdef * coeff.z + tmp.s0123 * coeff.w;
            vstore4(LinearToSRGBA8(tmp.s4567), 0, ptrOut); ptrOut -= stride * 4;
            tmp.s0123 = in4.s0123 * coeff.x + tmp.s4567 * coeff.y + tmp.s89ab * coeff.z + tmp.scdef * coeff.w;
            vstore4(LinearToSRGBA8(tmp.s0123), 0, ptrOut); ptrOut -= stride * 4;
        }
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
half3 LinearHToSRGB(const half3 color)
{
    half3 ret;
    ret.x = color.x <= 0.00304h ? 12.92h * color.x : 1.055h * native_powr(color.x, 1.0h / 2.4h) - 0.055h;
    ret.y = color.y <= 0.00304h ? 12.92h * color.y : 1.055h * native_powr(color.y, 1.0h / 2.4h) - 0.055h;
    ret.z = color.z <= 0.00304h ? 12.92h * color.z : 1.055h * native_powr(color.z, 1.0h / 2.4h) - 0.055h;
    return ret;
}
uchar4 LinearHToSRGBA8(const half4 color)
{
    return convert_uchar4((half4)(LinearHToSRGB(color.xyz), color.w));
}

//stageX,Linear,fp16 opt
kernel void IIR_RawXH(global const uchar* restrict src, global half* restrict mid, global half* restrict dst,
                  const uint width, const uint height, const uint stride, const half4 coeff)
{
    half16 in4, tmp;
    {
        global const uchar * restrict ptrIn  = src + get_global_id(0) * stride * 4;
        global       half * restrict ptrOut = mid + get_global_id(0) * width * 4;
        in4.s0123 = convert_half4(vload4(0, ptrIn));
        tmp = (half16)(in4.s0123, in4.s0123, in4.s0123, in4.s0123);
        for (uint i=0, j=0; j < width; j+=4)
        {
            in4 = convert_half16(vload16(i, ptrIn));
            tmp.s0123 = in4.s0123 * coeff.x + tmp.scdef * coeff.y + tmp.s89ab * coeff.z + tmp.s4567 * coeff.w;
            tmp.s4567 = in4.s4567 * coeff.x + tmp.s0123 * coeff.y + tmp.scdef * coeff.z + tmp.s89ab * coeff.w;
            tmp.s89ab = in4.s89ab * coeff.x + tmp.s4567 * coeff.y + tmp.s0123 * coeff.z + tmp.scdef * coeff.w;
            tmp.scdef = in4.scdef * coeff.x + tmp.s89ab * coeff.y + tmp.s4567 * coeff.z + tmp.s0123 * coeff.w;
            vstore16(tmp, i++, ptrOut);
        }
    }
    {
        global const half * restrict ptrIn  = mid + get_global_id(0) * width * 4;
        global       half * restrict ptrOut = dst + get_global_id(0)*4 + height * (width-1) * 4;
        in4.scdef = vload4(width-1, ptrIn);
        tmp = (half16)(in4.scdef, in4.scdef, in4.scdef, in4.scdef);
        for (uint i = width/4-1, j=0; j < width; j+=4)
        {
            in4 = vload16(i--, ptrIn);
            tmp.scdef = in4.scdef * coeff.x + tmp.s0123 * coeff.y + tmp.s4567 * coeff.z + tmp.s89ab * coeff.w;
            vstore4(tmp.scdef, 0, ptrOut); ptrOut -= height * 4;
            tmp.s89ab = in4.s89ab * coeff.x + tmp.scdef * coeff.y + tmp.s0123 * coeff.z + tmp.s4567 * coeff.w;
            vstore4(tmp.s89ab, 0, ptrOut); ptrOut -= height * 4;
            tmp.s4567 = in4.s4567 * coeff.x + tmp.s89ab * coeff.y + tmp.scdef * coeff.z + tmp.s0123 * coeff.w;
            vstore4(tmp.s4567, 0, ptrOut); ptrOut -= height * 4;
            tmp.s0123 = in4.s0123 * coeff.x + tmp.s4567 * coeff.y + tmp.s89ab * coeff.z + tmp.scdef * coeff.w;
            vstore4(tmp.s0123, 0, ptrOut); ptrOut -= height * 4;
        }
    }
}


//stageY,Linear,fp16 opt
kernel void IIR_RawYH(global const half* restrict src, global half* restrict mid, global uchar* restrict dst,
                  const uint width, const uint height, const uint stride, const half4 coeff)
{
    half16 in4, tmp;
    {
        global const half * restrict ptrIn  = src + get_global_id(0) * height * 4;
        global       half * restrict ptrOut = mid + get_global_id(0) * height * 4;
        in4.s0123 = vload4(0, ptrIn);
        tmp = (half16)(in4.s0123, in4.s0123, in4.s0123, in4.s0123);
        for (uint i=0, j=0; j < height; j+=4)
        {
            in4 = vload16(i, ptrIn);
            tmp.s0123 = in4.s0123 * coeff.x + tmp.scdef * coeff.y + tmp.s89ab * coeff.z + tmp.s4567 * coeff.w;
            tmp.s4567 = in4.s4567 * coeff.x + tmp.s0123 * coeff.y + tmp.scdef * coeff.z + tmp.s89ab * coeff.w;
            tmp.s89ab = in4.s89ab * coeff.x + tmp.s4567 * coeff.y + tmp.s0123 * coeff.z + tmp.scdef * coeff.w;
            tmp.scdef = in4.scdef * coeff.x + tmp.s89ab * coeff.y + tmp.s4567 * coeff.z + tmp.s0123 * coeff.w;
            vstore16(tmp, i++, ptrOut);
        }
    }
    {
        global const half  * restrict ptrIn  = mid + get_global_id(0) * height * 4;
        global       uchar * restrict ptrOut = dst + get_global_id(0)*4 + stride * (height-1) * 4;
        in4.scdef = vload4(height-1, ptrIn);
        tmp = (half16)(in4.scdef, in4.scdef, in4.scdef, in4.scdef);
        for (uint i=height/4-1,j=0; j < height; j+=4)
        {
            in4 = vload16(i--, ptrIn);
            tmp.scdef = in4.scdef * coeff.x + tmp.s0123 * coeff.y + tmp.s4567 * coeff.z + tmp.s89ab * coeff.w;
            vstore4(convert_uchar4(tmp.scdef), 0, ptrOut); ptrOut -= stride * 4;
            tmp.s89ab = in4.s89ab * coeff.x + tmp.scdef * coeff.y + tmp.s0123 * coeff.z + tmp.s4567 * coeff.w;
            vstore4(convert_uchar4(tmp.s89ab), 0, ptrOut); ptrOut -= stride * 4;
            tmp.s4567 = in4.s4567 * coeff.x + tmp.s89ab * coeff.y + tmp.scdef * coeff.z + tmp.s0123 * coeff.w;
            vstore4(convert_uchar4(tmp.s4567), 0, ptrOut); ptrOut -= stride * 4;
            tmp.s0123 = in4.s0123 * coeff.x + tmp.s4567 * coeff.y + tmp.s89ab * coeff.z + tmp.scdef * coeff.w;
            vstore4(convert_uchar4(tmp.s0123), 0, ptrOut); ptrOut -= stride * 4;
        }
    }
}


//stageX,SRGB,Linear,fp16 opt
kernel void IIR_SRGBXH(global const uchar* restrict src, global half* restrict mid, global half* restrict dst,
                  const uint width, const uint height, const uint stride, const half4 coeff)
{
    half16 in4, tmp;
    {
        global const uchar * restrict ptrIn  = src + get_global_id(0) * stride * 4;
        global       half * restrict ptrOut = mid + get_global_id(0) * width * 4;
        in4.s0123 = convert_half4(vload4(0, ptrIn));
        tmp = (half16)(in4.s0123, in4.s0123, in4.s0123, in4.s0123);
        for (uint i=0, j=0; j < width; j+=4)
        {
            in4 = convert_half16(vload16(i, ptrIn));
            in4.s012 = SRGBToLinearH(in4.s012); in4.s456 = SRGBToLinearH(in4.s456); in4.s89a = SRGBToLinearH(in4.s89a); in4.scde = SRGBToLinearH(in4.scde);
            tmp.s0123 = in4.s0123 * coeff.x + tmp.scdef * coeff.y + tmp.s89ab * coeff.z + tmp.s4567 * coeff.w;
            tmp.s4567 = in4.s4567 * coeff.x + tmp.s0123 * coeff.y + tmp.scdef * coeff.z + tmp.s89ab * coeff.w;
            tmp.s89ab = in4.s89ab * coeff.x + tmp.s4567 * coeff.y + tmp.s0123 * coeff.z + tmp.scdef * coeff.w;
            tmp.scdef = in4.scdef * coeff.x + tmp.s89ab * coeff.y + tmp.s4567 * coeff.z + tmp.s0123 * coeff.w;
            vstore16(tmp, i++, ptrOut);
        }
    }
    {
        global const half * restrict ptrIn  = mid + get_global_id(0) * width * 4;
        global       half * restrict ptrOut = dst + get_global_id(0)*4 + height * (width-1) * 4;
        in4.scdef = vload4(width-1, ptrIn);
        tmp = (half16)(in4.scdef, in4.scdef, in4.scdef, in4.scdef);
        for (uint i = width/4-1, j=0; j < width; j+=4)
        {
            in4 = vload16(i--, ptrIn);
            tmp.scdef = in4.scdef * coeff.x + tmp.s0123 * coeff.y + tmp.s4567 * coeff.z + tmp.s89ab * coeff.w;
            vstore4(tmp.scdef, 0, ptrOut); ptrOut -= height * 4;
            tmp.s89ab = in4.s89ab * coeff.x + tmp.scdef * coeff.y + tmp.s0123 * coeff.z + tmp.s4567 * coeff.w;
            vstore4(tmp.s89ab, 0, ptrOut); ptrOut -= height * 4;
            tmp.s4567 = in4.s4567 * coeff.x + tmp.s89ab * coeff.y + tmp.scdef * coeff.z + tmp.s0123 * coeff.w;
            vstore4(tmp.s4567, 0, ptrOut); ptrOut -= height * 4;
            tmp.s0123 = in4.s0123 * coeff.x + tmp.s4567 * coeff.y + tmp.s89ab * coeff.z + tmp.scdef * coeff.w;
            vstore4(tmp.s0123, 0, ptrOut); ptrOut -= height * 4;
        }
    }
}


//stageY,SRGB,Linear,fp16 opt
kernel void IIR_SRGBYH(global const half* restrict src, global half* restrict mid, global uchar* restrict dst,
                  const uint width, const uint height, const uint stride, const half4 coeff)
{
    half16 in4, tmp;
    {
        global const half * restrict ptrIn  = src + get_global_id(0) * height * 4;
        global       half * restrict ptrOut = mid + get_global_id(0) * height * 4;
        in4.s0123 = vload4(0, ptrIn);
        tmp = (half16)(in4.s0123, in4.s0123, in4.s0123, in4.s0123);
        for (uint i=0, j=0; j < height; j+=4)
        {
            in4 = vload16(i, ptrIn);
            tmp.s0123 = in4.s0123 * coeff.x + tmp.scdef * coeff.y + tmp.s89ab * coeff.z + tmp.s4567 * coeff.w;
            tmp.s4567 = in4.s4567 * coeff.x + tmp.s0123 * coeff.y + tmp.scdef * coeff.z + tmp.s89ab * coeff.w;
            tmp.s89ab = in4.s89ab * coeff.x + tmp.s4567 * coeff.y + tmp.s0123 * coeff.z + tmp.scdef * coeff.w;
            tmp.scdef = in4.scdef * coeff.x + tmp.s89ab * coeff.y + tmp.s4567 * coeff.z + tmp.s0123 * coeff.w;
            vstore16(tmp, i++, ptrOut);
        }
    }
    {
        global const half  * restrict ptrIn  = mid + get_global_id(0) * height * 4;
        global       uchar * restrict ptrOut = dst + get_global_id(0)*4 + stride * (height-1) * 4;
        in4.scdef = vload4(height-1, ptrIn);
        tmp = (half16)(in4.scdef, in4.scdef, in4.scdef, in4.scdef);
        for (uint i=height/4-1,j=0; j < height; j+=4)
        {
            in4 = vload16(i--, ptrIn);
            tmp.scdef = in4.scdef * coeff.x + tmp.s0123 * coeff.y + tmp.s4567 * coeff.z + tmp.s89ab * coeff.w;
            vstore4(LinearHToSRGBA8(tmp.scdef), 0, ptrOut); ptrOut -= stride * 4;
            tmp.s89ab = in4.s89ab * coeff.x + tmp.scdef * coeff.y + tmp.s0123 * coeff.z + tmp.s4567 * coeff.w;
            vstore4(LinearHToSRGBA8(tmp.s89ab), 0, ptrOut); ptrOut -= stride * 4;
            tmp.s4567 = in4.s4567 * coeff.x + tmp.s89ab * coeff.y + tmp.scdef * coeff.z + tmp.s0123 * coeff.w;
            vstore4(LinearHToSRGBA8(tmp.s4567), 0, ptrOut); ptrOut -= stride * 4;
            tmp.s0123 = in4.s0123 * coeff.x + tmp.s4567 * coeff.y + tmp.s89ab * coeff.z + tmp.scdef * coeff.w;
            vstore4(LinearHToSRGBA8(tmp.s0123), 0, ptrOut); ptrOut -= stride * 4;
        }
    }
}

#endif