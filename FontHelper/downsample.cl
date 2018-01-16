typedef struct Info
{
	uint offset;
	uchar w;
	uchar h;
}Info;


#define MAX_PIX 128
kernel void downsample4(global const Info * const restrict info, global const short4 * const restrict distimg, global uchar * const restrict result)
{
	private const uint gid = get_group_id(0);
	private const uint lid = get_local_id(0), ridx = lid * 4;
	private const uint w = info[gid].w, h = info[gid].h, goff = info[gid].offset, rstride = w / 4;

	if (ridx < h)
	{
		for (uint x = 0; x < rstride; ++x)
		{
			const uint offIn = goff / 4 + ridx * rstride + x;
			const uint offOut = goff / 16 + lid * rstride + x;

			const float4 data0 = convert_float4(distimg[offIn]);
			const float4 data1 = convert_float4(distimg[offIn + rstride]);
			const float4 data2 = convert_float4(distimg[offIn + 2 * rstride]);
			const float4 data3 = convert_float4(distimg[offIn + 3 * rstride]);

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
				result[offOut] = convert_uchar(clamp(posSum / (posCnt * 32.0f) + 128.0f, 0.0f, 255.0f));
			}
			else if (signCnt == -8)
			{
				result[offOut] = convert_uchar(clamp(negSum * 3.0f / (negCnt * 64.0f) + 128.0f, 0.0f, 255.0f));
			}
			else
			{
				const float distsum = avg4 <= 0.0f ?
					((negSum + avg4) * 4 / (negCnt + 1)) :
					((posSum + avg4) * 2 / (posCnt + 1));
				result[offOut] = convert_uchar(clamp(distsum / 64.0f + 128.0f, 0.0f, 255.0f));
			}
		}
		
	}
	/*
	
	for (uint32_t y = 0, ylim = fi.h / 4; y < ylim; y++)
	{
		uint32_t opos = (starty + y) * rowstep + startx;
		size_t ipos = fi.offset + (fi.w * y * 4);
		for (uint32_t xlim = fi.w / 4; xlim--; ipos += 4, ++opos)
		{
			const uint64_t data64[4] = { *(uint64_t*)&distsq[ipos],*(uint64_t*)&distsq[ipos + fi.w] ,*(uint64_t*)&distsq[ipos + 2 * fi.w] ,*(uint64_t*)&distsq[ipos + 3 * fi.w] };
			const int16_t * __restrict const data16 = (int16_t*)data64;
			int32_t dists[8];
			dists[0] = data16[5] * 3 - data16[0], dists[1] = data16[6] * 3 - data16[3], dists[2] = data16[9] * 3 - data16[12], dists[3] = data16[10] * 3 - data16[15];
			dists[4] = ((data16[5] + data16[9]) * 3 - (data16[4] + data16[8])) / 2, dists[5] = ((data16[6] + data16[10]) * 3 - (data16[7] + data16[11])) / 2;
			dists[6] = ((data16[5] + data16[6]) * 3 - (data16[1] + data16[2])) / 2, dists[7] = ((data16[9] + data16[10]) * 3 - (data16[13] + data16[14])) / 2;
			int32_t avg4 = (data16[5] + data16[6] + data16[9] + data16[10]) / 2;
			int32_t negsum = 0, possum = 0, negcnt = 0, poscnt = 0;
			for (const auto dist : dists)
			{
				if (dist <= 0)
					negsum += dist, negcnt++;
				else
					possum += dist, poscnt++;
			}
			if (negcnt == 0)
				finPtr[opos] = std::clamp(possum / (poscnt * 32) + 128, 0, 255);
			else if (poscnt == 0)
				finPtr[opos] = std::clamp(negsum * 3 / (negcnt * 64) + 128, 0, 255);
			else
			{
				const bool isInside = avg4 < 0;
				const auto distsum = isInside ? ((negsum + avg4) * 4 / (negcnt + 1)) : ((possum + avg4) * 2 / (poscnt + 1));
				finPtr[opos] = std::clamp(distsum / 64 + 128, 0, 255);
			}
		}
	}
	*/
}