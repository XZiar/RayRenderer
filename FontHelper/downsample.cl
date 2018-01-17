typedef struct Info
{
	uint offset;
	uchar w;
	uchar h;
}Info;


#define MAX_PIX 128
kernel void avg16(global const Info * const restrict info, global const short * const restrict distimg, global uchar * const restrict result)
{
	private const uint gid = get_group_id(0);
	private const uint lid = get_local_id(0), ridx = lid * 4;
	private const uint w = info[gid].w, h = info[gid].h, goff = info[gid].offset, rstride = w / 4;
	global const short * const curimg = distimg + goff;
	if (ridx < h)
	{
		global const short * const rowPtr0 = curimg + ridx * w;
		global const short * const rowPtr1 = curimg + (ridx + 1) * w;
		global const short * const rowPtr2 = curimg + (ridx + 2) * w;
		global const short * const rowPtr3 = curimg + (ridx + 3) * w;
		global uchar * const output = result + goff / 16 + lid * rstride;
		for (uint x = 0; x < rstride; x++)
		{
			const float4 data0 = convert_float4(vload4(x, rowPtr0));
			const float4 data1 = convert_float4(vload4(x, rowPtr1));
			const float4 data2 = convert_float4(vload4(x, rowPtr2));
			const float4 data3 = convert_float4(vload4(x, rowPtr3));

			const float4 dataSum = (data0 + data1) + (data2 + data3);
			const float4 ones = 1.0f;
			const float avg = dot(dataSum, ones) / 16.0f;
			output[x] = convert_uchar(clamp(avg / 16.0f + 128.0f, 0.0f, 255.0f));
		}

	}
}

kernel void downsample4(global const Info * const restrict info, global const short * const restrict distimg, global uchar * const restrict result)
{
	private const uint gid = get_group_id(0);
	private const uint lid = get_local_id(0), ridx = lid * 4;
	private const uint w = info[gid].w, h = info[gid].h, goff = info[gid].offset, rstride = w / 4;
	global const short * const curimg = distimg + goff;
	if (ridx < h)
	{
		global const short * const rowPtr0 = curimg + ridx * w;
		global const short * const rowPtr1 = curimg + (ridx + 1) * w;
		global const short * const rowPtr2 = curimg + (ridx + 2) * w;
		global const short * const rowPtr3 = curimg + (ridx + 3) * w;
		global uchar * const output = result + goff / 16 + lid * rstride;
		for (uint x = 0; x < rstride; x++)
		{
			const float4 data0 = convert_float4(vload4(x, rowPtr0));
			const float4 data1 = convert_float4(vload4(x, rowPtr1));
			const float4 data2 = convert_float4(vload4(x, rowPtr2));
			const float4 data3 = convert_float4(vload4(x, rowPtr3));

			const float4 middle = shuffle2(data1, data2, (uint4)(1, 2, 5, 6));//5,6,9,10
			const float4 corner = shuffle2(data0, data3, (uint4)(0, 3, 4, 7));//0,3,12,15
			const float avg4 = (middle.x + middle.y + middle.z + middle.w) * 0.5f;
			const float4 distsX = middle * 3.0f - corner;

			const float4 middle0 = shuffle2(data1, data2, (uint4)(1, 2, 1, 3));//5,6,5,9
			const float4 middle1 = shuffle2(data1, data2, (uint4)(5, 6, 2, 6));//9,10,6,10
			const float4 upper = shuffle2(data0, data1, (uint4)(1, 2, 4, 7));//1,2,4,7
			const float4 lower = shuffle2(data2, data3, (uint4)(0, 3, 5, 6));//8,11,13,14
			const float4 corner0 = shuffle2(upper, lower, (uint4)(2, 3, 0, 6));//4,7,1,13
			const float4 corner1 = shuffle2(upper, lower, (uint4)(4, 5, 1, 7));//8,11,2,14
			const float4 distsC = ((middle0 + middle1) * 3.0f - (corner0 + corner1)) * 0.5f;

			const int4 isnegX = signbit(distsX), isnegC = signbit(distsC);
			const float4 empty = 0.0f;
			const float4 negX = select(empty, distsX, isnegX), negC = select(empty, distsC, isnegC);
			const float4 posX = select(distsX, empty, isnegX), posC = select(distsC, empty, isnegC);
			const int4 isneg2 = isnegX + isnegC;
			const float4 neg2 = negX + negC, pos2 = posX + posC;
			const int signCnt = isneg2.x + isneg2.y + isneg2.z + isneg2.w;
			const float negCnt = -signCnt, posCnt = 8.0f - signCnt;
			const float posSum = pos2.x + pos2.y + pos2.z + pos2.w;
			const float negSum = neg2.x + neg2.y + neg2.z + neg2.w;
			if (signCnt == 0)
			{
				output[x] = convert_uchar(clamp(posSum / (posCnt * 32.0f) + 128.0f, 0.0f, 255.0f));
			}
			else if (signCnt == -8)
			{
				output[x] = convert_uchar(clamp(negSum * 3.0f / (negCnt * 64.0f) + 128.0f, 0.0f, 255.0f));
			}
			else
			{
				const float distsum = avg4 <= 0.0f ?
					((negSum + avg4) * 4 / (negCnt + 1)) :
					((posSum + avg4) * 2 / (posCnt + 1));
				output[x] = convert_uchar(clamp(distsum / 64.0f + 128.0f, 0.0f, 255.0f));
			}
		}
		
	}
	
}