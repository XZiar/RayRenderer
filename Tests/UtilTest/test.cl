//["huang-he-yang-26","ke-yu-45-73","li-zheng-yan-25-3","luka-liu","qie-you-65","song-guo-ke-85","sun-ran-96-27","xiaoying-fan-82","xym-57","zhu-shen-jie-18"]

kernel void main(constant uchar* ptr, global uchar* out)
{
    uchar16 val1 = vload16(0, ptr), val2 = vload16(1, ptr);
    uchar4 left  = val1.s048c;
    uchar16 mid   = (uchar16)(val1.s0123, val1.s456789ab, val2.s0123);
    uchar4 right = (uchar4)(val1.s8c, val2.s04);
    float4 tmp = convert_float4(right) * (float)left.x;

    vstore4(left+right, 0, out);
}