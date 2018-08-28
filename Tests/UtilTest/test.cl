
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