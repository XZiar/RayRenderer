
kernel void blurX(global const uchar* restrict src, global float* restrict mid, global float* restrict dst,
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


kernel void blurY(global const float* restrict src, global float* restrict mid, global uchar* restrict dst,
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



// kernel void blurX(global const uchar* restrict src, global float* restrict dst, const uint width, const uint height, constant float* restrict coeff)
// {
//     global const uchar * restrict ptrline = src + get_global_id(0) * width * 4;
//     global       float * restrict ptrOut  = dst + get_global_id(0) * width * 4;
//     float16 output = (float16)0;
//     float16 prev = convert_float16(vload16(0, ptrline));
//     output = prev * coeff[0];
//     for (uint i=1, j=1; i < 13; ++i)
//     {
//         float16 cur = convert_float16(vload16(i, ptrline));
//         output += (float16)(prev.s4567, prev.s89abcdef, cur.s0123) * coeff[j++];
//         output += (float16)(prev.s89abcdef, cur.s01234567        ) * coeff[j++];
//         output += (float16)(prev.scdef,  cur.s01234567, cur.s89ab) * coeff[j++];
//         output += cur                                              * coeff[j++];
//         prev = cur;
//     }
//     float16 cur = convert_float16(vload16(13, ptrline));
//     output += (float16)(temp.s4567, temp.s89abcdef, cur.s0123) * coeff[49];
//     output += (float16)(temp.s89abcdef, cur.s01234567        ) * coeff[50];
//     vstore4(output.s0123,          0, dst);
//     vstore4(output.s4567, width *  4, dst);
//     vstore4(output.s89ab, width *  8, dst);
//     vstore4(output.scdef, width * 12, dst);
// }
