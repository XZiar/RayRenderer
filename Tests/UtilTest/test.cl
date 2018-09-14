
kernel void test(constant uchar* ptr, global uchar* out)
{
    uchar16 val1 = vload16(0, ptr), val2 = vload16(1, ptr);
    uchar4 left  = val1.s048c;
    uchar16 mid   = (uchar16)(val1.s0123, val1.s456789ab, val2.s0123);
    uchar4 right = (uchar4)(val1.s8c, val2.s04);
    float4 tmp = convert_float4(right) * (float)left.x;

    vstore4(left+right, 0, out);
}

constant float coeff51[] = 
{
    0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,
    0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f
};

kernel void test2(global const uchar* src, global float* dst)
{
    float16 output = (float16)0;
    float16 temp1 = convert_float16(vload16(0, src));
    output += temp1 * coeff51[0];
    float16 prev = temp1;
    for(uint i=1, j=1; i < 13; ++i)
    {
        float16 cur = convert_float16(vload16(i, src));
        output += (float16)(prev.s4567, prev.s89abcdef, cur.s0123) * coeff51[j++];
        output += (float16)(prev.s89abcdef, cur.s01234567) * coeff51[j++];
        output += (float16)(prev.scdef, cur.s01234567, cur.s89ab) * coeff51[j++];
        output += cur * coeff51[j++];
        prev = cur;
    }
    float16 temp14 = convert_float16(vload16(13, src));
    output += (float16)(prev.s4567, prev.s89abcdef, temp14.s0123) * coeff51[49];
    output += (float16)(prev.s89abcdef, temp14.s01234567) * coeff51[50];
    vstore16(output, 0, dst);
}

#define warmup 24
#define count 64
#define COEFF_B 0.345f
#define COEFF_b1 0.37f
#define COEFF_b2 0.679f
#define COEFF_b3 0.038f

kernel void iirblur_lr(global const uchar *src, global uchar *dst, const uint width, const uint height, const uint stride)
{
    global const uchar *ptr = src + get_global_id(0)*4 + get_global_id(1)*stride;
    global uchar *ptrDst = dst + get_global_id(0)*4 + get_global_id(1)*stride;
    float4 tmps = convert_float4(vload4(0, ptr));
    float4 tmp1 = tmps, tmp2 = tmps, tmp3 = tmps;
    for (int i=0; i<(warmup+count)/4; ++i)
    {
        float16 in4 = convert_float16(vload16(i, ptr));
        float16 out4;
        out4.s0123 = in4.s0123 * COEFF_B +       tmp1 * COEFF_b1 +       tmp2 * COEFF_b2 +       tmp3 * COEFF_b3;
        out4.s4567 = in4.s4567 * COEFF_B + out4.s0123 * COEFF_b1 +       tmp2 * COEFF_b2 +       tmp3 * COEFF_b3;
        out4.s89ab = in4.s89ab * COEFF_B + out4.s4567 * COEFF_b1 + out4.s0123 * COEFF_b2 +       tmp3 * COEFF_b3;
        out4.scdef = in4.scdef * COEFF_B + out4.s89ab * COEFF_b1 + out4.s4567 * COEFF_b2 + out4.s0123 * COEFF_b3;
        if (i >= warmup/4)
            vstore16(convert_uchar16_sat(out4), i-warmup/4, ptrDst);
        tmp3 = out4.s4567; tmp2 = out4.s89ab; tmp1 = out4.scdef;
    }
}

kernel void iirblur_lr2(global const uchar *src, global uchar *dst, const uint width, const uint height, const uint stride)
{
    global const uchar *ptr = src + get_global_id(0)*4 + get_global_id(1)*stride;
    global uchar *ptrDst = dst + get_global_id(0)*4 + get_global_id(1)*stride;
    float4 tmps = convert_float4(vload4(0, ptr));
    float4 tmp1 = tmps, tmp2 = tmps, tmp3 = tmps;
    for (int i=0; i<(warmup+count)/4; i+=2)
    {
        float16 in1 = convert_float16(vload16(i, ptr));
        float16 in2 = convert_float16(vload16(i+1, ptr));
        float16 out1;
        out1.s0123 = in1.s0123 * COEFF_B +       tmp1 * COEFF_b1 +       tmp2 * COEFF_b2 +       tmp3 * COEFF_b3;
        out1.s4567 = in1.s4567 * COEFF_B + out1.s0123 * COEFF_b1 +       tmp1 * COEFF_b2 +       tmp2 * COEFF_b3;
        out1.s89ab = in1.s89ab * COEFF_B + out1.s4567 * COEFF_b1 + out1.s0123 * COEFF_b2 +       tmp1 * COEFF_b3;
        out1.scdef = in1.scdef * COEFF_B + out1.s89ab * COEFF_b1 + out1.s4567 * COEFF_b2 + out1.s0123 * COEFF_b3;
        if (i >= warmup/4)
            vstore16(convert_uchar16_sat(out1), i-warmup/4, ptrDst);
        
        in1.s0123 = in2.s0123 * COEFF_B + out1.scdef * COEFF_b1 + out1.s89ab * COEFF_b2 + out1.s4567 * COEFF_b3;
        in1.s4567 = in2.s4567 * COEFF_B +  in1.s0123 * COEFF_b1 + out1.scdef * COEFF_b2 + out1.s89ab * COEFF_b3;
        in1.s89ab = in2.s89ab * COEFF_B +  in1.s4567 * COEFF_b1 +  in1.s0123 * COEFF_b2 + out1.s4567 * COEFF_b3;
        in1.scdef = in2.scdef * COEFF_B +  in1.s89ab * COEFF_b1 +  in1.s4567 * COEFF_b2 +  in1.s0123 * COEFF_b3;
        if (i >= warmup/4)
            vstore16(convert_uchar16_sat(in1), i-warmup/4 + 1, ptrDst);
        tmp3 = in1.s4567; tmp2 = in1.s89ab; tmp1 = in1.scdef;
    }
}

kernel void fastblur_hor(global const uchar *src, global uchar *dst, const uint width, const uint height, const uint stride)
{
    global const uchar *ptr = src + get_global_id(0)*4 + get_global_id(1)*stride;
    global uchar *ptrDst = dst + get_global_id(0)*4 + get_global_id(1)*stride;
    float4 sumLeft = 0, sumRight = 0, color = 0;
    const float N = 1.0f/(26*26);
    float j = 1.0f;
    for (uint i=0; i<6; ++i, j+=4.0f)
    {
        float16 tmp = convert_float16(vload16(i, ptr)) * N;
        color += j*tmp.s0123 + (j+1.0f)*tmp.s4567 + (j+2.0f)*tmp.s89ab + (j+3.0f)*tmp.scdef;
        sumLeft += tmp.s0123 + tmp.s4567 + tmp.s89ab + tmp.scdef;
    }
    {
        float8 tmp = convert_float8(vload8(12, ptr)) * N;
        color += 24.0f*tmp.s0123 + 25.0f*tmp.s4567;
        sumLeft += tmp.s0123 + tmp.s4567;
    }
    j = 25.0f;
    for(uint i=6; i<12; ++i, j-=4.0f)
    {
        float16 tmp = convert_float16(vload16(i, ptr+8)) * N;
        color += j*tmp.s0123 + (j-1.0f)*tmp.s4567 + (j-2.0f)*tmp.s89ab + (j-3.0f)*tmp.scdef;
        sumRight += tmp.s0123 + tmp.s4567 + tmp.s89ab + tmp.scdef;
    }
    {
        float4 tmp = convert_float4(vload4(50, ptr)) * N;
        color += tmp; sumRight += tmp;
        vstore4(color, 0, (global float*)ptrDst);
    }
    for (uint i=1; i<16; ++i)
    {
        const float4 right = convert_float4(vload4(i+50, ptr)) * N;
        sumRight += right;
        color += sumRight - sumLeft;
        vstore4(color, i, (global float*)ptrDst);
        const float4 middle = convert_float4(vload4(i+25, ptr)) * N;
        sumLeft += middle; sumRight -= middle;
        const float4 left = convert_float4(vload4(i-1, ptr)) * N;
        sumLeft -= left;
    }
}

