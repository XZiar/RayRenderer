typedef struct Info
{
	uint offset;
	uchar w;
	uchar h;
}Info;

kernel void bmpsdf(global const Info* restrict info, global uchar* restrict img, global ushort* restrict result)
{
	private const uint gid = get_group_id(0);
	private const uint lid = get_local_id(0);
	local ushort sqLUT[160];
	local uchar xdist[160 * 160];
	const uint w = info[gid].w, h = info[gid].h, offset = info[gid].offset;
	private uchar raw[160];
	//setup square-LUT
	sqLUT[lid] = lid * lid;
	//each row operation
	if (lid < h)
	{
		uint idx = lid * w;
		uchar dist = 64, adder = 0, curimg = 0;
		for (uint x = 0; x < w; ++x, ++idx, dist += adder)
		{
			uchar objimg = raw[x] = img[idx + offset];
			if (objimg != curimg)
				xdist[idx] = dist = 0, adder = 1, curimg = objimg;
			else
				xdist[idx] = dist;
		}
		dist = xdist[--idx], adder = 0, curimg = 0;
		for (uint x = w; x--; --idx, dist += adder)
		{
			uchar objimg = raw[x];
			if (objimg != curimg)
				xdist[idx] = dist = 0, adder = 1, curimg = objimg;
			else
				xdist[idx] = min(xdist[idx], dist);
		}
	}
	//synchronize
	barrier(CLK_LOCAL_MEM_FENCE);
	if (lid < w)
	{
		uint idx = lid;
		private ushort perCol[160];
		ushort obj = 32768;
		for (uint y = 0, tmpidx = lid + offset; y < h; ++y, tmpidx += w)
			raw[y] = img[tmpidx];
		perCol[0] = sqLUT[xdist[idx]];
		for (uint y = 1; y < h; perCol[y++] = obj)
		{
			uint testidx = idx;
			idx += w;
			uchar curimg = raw[y], curxdist = xdist[idx];
			if (curxdist == 0)// 0 dist = edge
			{
				obj = 0; continue;
			}
			obj = sqLUT[curxdist];
			ushort maxdy2 = min(obj, sqLUT[y + 1]);
			for (ushort dy = 1, dy2 = 1; dy2 < maxdy2; dy2 = sqLUT[++dy], testidx -= w)
			{
				uchar oimg = raw[y - dy], oxdist = xdist[testidx];
				if (oimg != curimg)
				{
					//dy^2 < obj
					obj = dy2;
					break;//further won't be shorter
				}
				ushort newdist = sqLUT[oxdist] + dy2;
				if (newdist < obj)
				{
					obj = newdist;
					maxdy2 = min(newdist, maxdy2);
				}
			}
		}
		for (uint y = h - 1; y--; perCol[y] = obj)
		{
			uint testidx = idx;
			idx -= w;
			uchar curimg = raw[y], curxdist = xdist[idx];
			if (curxdist == 0)// 0 dist = edge
				continue;
			obj = perCol[y];
			ushort maxdy2 = min(obj, sqLUT[h - y - 1]);
			for (ushort dy = 1, dy2 = 1; dy2 < maxdy2; dy2 = sqLUT[++dy], testidx += w)
			{
				uchar oimg = raw[y + dy], oxdist = xdist[testidx];
				if (oimg != curimg)
				{
					//dy^2 < obj
					obj = dy2;
					break;//further won't be shorter
				}
				ushort newdist = sqLUT[oxdist] + dy2;
				if (newdist < obj)
				{
					obj = newdist;
					maxdy2 = min(newdist, maxdy2);
				}
			}
		}
		idx = offset + lid;
		for (uint y = 0; y < h; ++y, idx += w)
			result[idx] = perCol[y];
		//result[idx] = sqLUT[xdist[idx - offset]];
	}
}



#define THREDHOLD 16
#define MAX_PIX 128
#define MAX_PIX4 32
#define XD_Stride 129
#define XD_Stride4 33


float min_hor(const float4 obj)
{
	return min(min(obj.s0, obj.s1), min(obj.s2, obj.s3));
}
short min_hor2(const short4 obj)
{
	return min(min(obj.s0, obj.s1), min(obj.s2, obj.s3));
}
float2 minmax_hor(const float4 obj)
{
	float2 tmp1, tmp2;
	tmp1 = obj.s0 < obj.s1 ? obj.xy : obj.yx;
	tmp2 = obj.s2 < obj.s3 ? obj.zw : obj.wz;
	return (float2)(min(tmp1.s0, tmp2.s0), max(tmp1.s1, tmp2.s1));
}
uchar distRounding2(const float fdist2, const float ydist)
{
	const float fdist = copysign(native_sqrt(fdist2), ydist);
	return convert_uchar_sat(fdist * (fdist < 0.0f ? 0.1053f/*1/9.5*/ : 0.0625f/*1/16*/) + 128.0f);
}
float incellTriDist2(const float4 xdist4_sqr)
{
	float4 distY = (float4)(147456.0f, 16384.0f, 16384.0f, 146456.0f);//sqr for (384,128,128,384)
	//float4 distY = (float4)(10240.0f, 4096.0f, 4096.0f, 10240.0f);//sqr for (320,64,64,320)
	const float4 dist_sqr = xdist4_sqr + distY;
	return min_hor(dist_sqr);
}

uchar bool4Pack(const char4 bools)
{
	const uchar4 bit4 = select((uchar4)(0, 0, 0, 0), (uchar4)(8, 4, 2, 1), bools);
	return (bit4.x + bit4.y) + (bit4.z + bit4.w);
}
constant char4 THE_BOOL_LUT[16] =
{
	(char4)( 0, 0, 0, 0), (char4)( 0, 0, 0,-1), (char4)( 0, 0,-1, 0), (char4)( 0, 0,-1,-1),
	(char4)( 0,-1, 0, 0), (char4)( 0,-1, 0,-1), (char4)( 0,-1,-1, 0), (char4)( 0,-1,-1,-1),
	(char4)(-1, 0, 0, 0), (char4)( 0, 0, 0,-1), (char4)(-1, 0,-1, 0), (char4)(-1, 0,-1,-1),
	(char4)(-1,-1, 0, 0), (char4)(-1,-1, 0,-1), (char4)(-1,-1,-1, 0), (char4)(-1,-1,-1,-1),
};

__attribute__((reqd_work_group_size(32, 1, 1)))
kernel void graysdf4(global const Info * const restrict info, global const uchar * const restrict img, global uchar * const restrict result)
{
	private const uint gid = get_group_id(0);
	private const uint lid = get_local_id(0), lid4 = lid * 4;
	local float xdistf4[MAX_PIX4 * XD_Stride];
	private const uint w = info[gid].w, h = info[gid].h, offset = info[gid].offset, rowstep = w / 4, ylim = h / 4;
	local float imgraw[MAX_PIX4 * XD_Stride4];
	local uchar imgcache[MAX_PIX4 * XD_Stride];

	local short distCache[MAX_PIX * XD_Stride4];
	local short * restrict const perRCCache = (local short*)distCache + lid * XD_Stride;
	/*
	if(lid < ylim)
	{
		local uchar4 * restrict const raw = (local uchar4*)imgraw + lid * XD_Stride4;
		for (uint row = 0; row < 4; ++row)//4 rows
		{
			{
				global const uchar * restrict const rowptr = img + offset + (lid4 + row) * w;
				for (uint x = 0, imgidx = lid4 + row; x < rowstep; ++x, imgidx += XD_Stride)//pre-transpose img
				{
					const uchar4 img4 = vload4(x, rowptr);
					raw[x] = img4;
					imgcache[imgidx] = convert_uchar_sat(dot(convert_float4(img4), (float4)(-0.25f, 0.75f, 0.75f, -0.25f)));
				}
			}
			local float * restrict const xdistRow = xdistf4 + lid4 + row;//pre-transposed
			int dist = 64 * 256;
			bool quickEnd = false;
			char4 thresLastLow = (char4)(-1, -1, -1, -1);
			for (uint x = 0, xidx = 0; x < rowstep; ++x, xidx += XD_Stride)
			{
				const uchar4 img4 = raw[x];
				const char4 thresLow = img4 < (uchar4)(THREDHOLD);
				if (abs(img4.y - img4.z) > THREDHOLD * 8)//assume edge
				{
					perRCCache[x] = 256 - (img4.y + img4.z);
					dist = thresLow.z == thresLow.w ? 768 - img4.y : 512 - img4.z;
				}
				else
				{
					const char4 eqMask = thresLow == thresLastLow;
					
					if (eqMask.s0)
					{
						if(eqMask.s1)
						{
							perRCCache[x] = dist = dist + 512;//just take as positive
						}
						else
						{
							perRCCache[x] = dist = img4.y;
						}
					}
					else if(eqMask.s1)
					{
						perRCCache[x] = dist = img4.y;
					}
					else
					{
						perRCCache[x] = dist = 256 + img4.x;
					}

					if (thresLow.y == thresLow.z)
					{
						if (thresLow.z == thresLow.w)
							dist += 512;
						else
							dist = thresLow.w ? 512 - img4.z : img4.w;
					}
					else
					{
						if (thresLow.z == thresLow.w)
							dist += 512;
						else
							dist = thresLow.w ? 256 - img4.w : img4.w;
					}
				}
				thresLastLow = thresLow.wwww;
			}
			dist = 64 * 256;
			thresLastLow = (char4)(-1, -1, -1, -1);
			for (uint x = rowstep, xidx = (rowstep - 1) * XD_Stride; x--; xidx -= XD_Stride)
			{
				const uchar4 img4 = raw[x];
				const char4 thresLow = img4 < (uchar4)(THREDHOLD);
				const int prevdist = perRCCache[x];
				if (abs(img4.y - img4.z) > THREDHOLD * 8)//assume edge
				{
					const float tmp = 256 - (img4.y + img4.z);
					xdistRow[xidx] = tmp * tmp;
					dist = thresLow.z == thresLow.w ? 768 - img4.z : 512 - img4.y;
				}
				else
				{
					const char4 eqMask = thresLow == thresLastLow;
					int tmp;
					if (eqMask.s0)
					{
						if (eqMask.s1)
						{
							tmp = dist = dist + 512;//just take as positive
						}
						else
						{
							tmp = dist = img4.y;//just take as positive
						}
					}
					else if (eqMask.s1)
					{
						tmp = dist = img4.y;//just take as positive
					}
					else
					{
						tmp = dist = 256 + img4.x;//just take as positive
					}
					tmp = min(prevdist, tmp);
					xdistRow[xidx] = tmp * tmp;

					if (thresLow.y == thresLow.z)
					{
						if (thresLow.z == thresLow.w)
							dist += 512;
						else
							dist = thresLow.w ? 512 - img4.z : img4.w;
					}
					else
					{
						if (thresLow.z == thresLow.w)
							dist += 512;
						else
							dist = thresLow.w ? 256 - img4.w : img4.w;
					}
				}
				thresLastLow = thresLow.wwww;
			}
		}
	}
	//*/
	///*
	if (lid4 < h)//each row operation
	{
		local uchar * restrict const raw = (local uchar*)imgraw + lid * XD_Stride;
		for (uint row = 0; row < 4; ++row)//4 rows
		{
			{
				global const uchar * restrict const rowptr = img + offset + (lid4 + row) * w;
				for (uint x = 0, imgidx = lid4 + row, xlim = w / 4; x < xlim; ++x, imgidx += XD_Stride)//pre-transpose img
				{
					const uchar4 img4 = vload4(x, rowptr);
					vstore4(img4, x, raw);
					imgcache[imgidx] = convert_uchar_sat(dot(convert_float4(img4), (float4)(-0.25f, 0.75f, 0.75f, -0.25f)));
				}
			}
			int dist = 64 * 256, adder = 0;
			uchar lastimg = 0;
			bool lastThresNL = false;
			for (uint x = 0; x < w; ++x, dist += adder)
			{
				const uchar objimg = raw[x];
				const bool objThresNL = objimg > THREDHOLD;
				if (lastThresNL == objThresNL)
					perRCCache[x] = dist;
				else
				{
					adder = 256;
					const int dEnter = objimg - 128;
					const int dLeave = (256 + 128) - lastimg;
					const bool isEnter = lastimg < objimg;
					dist = isEnter ? dEnter : dLeave;
					perRCCache[x] = abs(dist);
				}
				lastimg = objimg; lastThresNL = objThresNL;
			}
			dist = perRCCache[w - 1], adder = 0;
			lastimg = 0, lastThresNL = false;
			for (uint x = w; x--; dist += adder)
			{
				const int objPix = perRCCache[x];
				const uchar objimg = raw[x];
				const bool objThresNL = objimg > THREDHOLD;
				if (lastThresNL == objThresNL)
					perRCCache[x] = min(dist, objPix);
				else
				{
					adder = 256;
					const int dEnter = objimg - 128;
					const int dLeave = (256 + 128) - lastimg;
					const bool isEnter = lastimg < objimg;
					dist = isEnter ? dEnter : dLeave;
					perRCCache[x] = min((int)abs(dist), objPix);
				}
				lastimg = objimg; lastThresNL = objThresNL;
			}
			//downsample
			for (uint x = 0, xdidx = lid4 + row, xlim = w / 4; x < xlim; ++x, xdidx += XD_Stride)//pre-transpose xdist
			{
				const uchar4 objimg = vload4(x, raw);
				if (abs(objimg.y - objimg.z) > THREDHOLD * 8)//assume edge
				{
					const float tmp = 256 - (objimg.y + objimg.z);
					xdistf4[xdidx] = tmp * tmp;
					continue;
				}
				const float4 dist4 = convert_float4(vload4(x, perRCCache));
				const float mindist = min(dist4.y * 3.0f - dist4.x, dist4.z * 3.0f - dist4.w);
				xdistf4[xdidx] = mindist * mindist * 0.25f;
			}
		}
	}
	//*/
	//synchronize
	barrier(CLK_LOCAL_MEM_FENCE);

	if (lid4 < w)
	{
		global uchar * restrict const outCol = result + offset / 16 + lid;
		local const uchar * restrict const raw = imgcache + lid * XD_Stride;//pre-transposed
		local const float * restrict const xdistCol = xdistf4 + lid * XD_Stride;//pre-transposed

		local short * restrict const minPixLowUp = distCache + lid * XD_Stride4;
		local short * restrict const minPixHighUp = distCache + MAX_PIX4 * XD_Stride4 + lid * XD_Stride4;
		local short * restrict const minPixLowDown = distCache + 2 * MAX_PIX4 * XD_Stride4 + lid * XD_Stride4;
		local short * restrict const minPixHighDown = distCache + 3 * MAX_PIX4 * XD_Stride4 + lid * XD_Stride4;

		const float4 infTmp = 256.0f*128.0f;
		//prepare mindist cache
		for (uint y = 0; y < ylim; ++y)
		{
			const uint4 img4 = convert_uint4(vload4(y, raw));
			const int4 thresLow = img4 < (uint4)(THREDHOLD);
			const int4 thresHigh = img4 >= (uint4)(256 - THREDHOLD);
			const float4 img4f = convert_float4(img4);
			const float4 lowup = (float4)(896.0f, 640.0f, 384.0f, 128.0f) - img4f;
			const float4 highup = (float4)(640.0f, 384.0f, 128.0f, -128.0f) + img4f;
			const float nonLowUp = min_hor(select(lowup, infTmp, thresLow));
			const float nonHighUp = min_hor(select(highup, infTmp, thresHigh));
			const float4 lowdown = (float4)(128.0f, 384.0f, 640.0f, 896.0f) - img4f;
			const float4 highdown = (float4)(-128.0f, 128.0f, 384.0f, 640.0f) + img4f;
			const float nonLowDown = min_hor(select(lowup, infTmp, thresLow));
			const float nonHighDown = min_hor(select(highup, infTmp, thresHigh));

			minPixLowUp[y] = convert_short(nonLowUp);
			minPixHighUp[y] = convert_short(nonHighUp);
			minPixLowDown[y] = convert_short(nonLowDown);
			minPixHighDown[y] = convert_short(nonHighDown);
		}

		bool quickEnd = false;
		int dist = 64 * 256;
		float yf256 = 640.0f;
		for (uint y = 0, outidx = 0; y < ylim; y++, outidx += rowstep, yf256 += 1024.0f)
		{
			bool flag = false;
			const uchar4 img4 = vload4(y, raw);
			const char4 thresLow = img4 < (uchar4)(THREDHOLD);
			const char4 thresHigh = img4 >= (uchar4)(256 - THREDHOLD);

			float predYDist = 0;
			if (thresLow.y != thresLow.z)//TF:L+M/H, or FT:M/H+L
			{
				//L+H, L+M, H+L, M+L
				predYDist = thresLow.y ? (thresHigh.z ? -img4.y : 256 - img4.z) : (thresHigh.y ? -img4.z : 256 - img4.y);
				//M/H+L+L, M/H+L+M/H, LH+H, LH+L/M, LM+L, LM+M/H
				dist = thresLow.z ? (thresLow.w ? 512 - img4.z : -img4.w) :
					(thresHigh.z ? (thresHigh.w ? -512 - img4.y : 256 - img4.w) :
						(thresLow.w ? 512 - img4.z : 256 - img4.w));
					
				quickEnd = true;//solved output
			}
			else if (thresHigh.y != thresHigh.z)//TF:H+M, or FT:M+H
			{
				predYDist = thresHigh.y ? -img4.z : -img4.y;
				//MH+H, MH+L/M, HM+L, HM+M/H
				dist = thresHigh.z ? (thresHigh.w ? -512 - img4.y : 256 - img4.w) : (thresLow.w ? 512 - img4.z : - img4.w);
				
				quickEnd = true;//solved output
			}
			else if(thresLow.y == thresHigh.y)//FF->M+M, as edge
			{
				predYDist = 256.0f - (img4.y + img4.z);
				dist = img4.z < 128 ? 512 + (256 - img4.y) : 512 - img4.z;
				
				quickEnd = true;
			}
			else//L+L, or H+H, consider ele.w
			{
				//calculate uppper dist
				int curdistY1 = thresLow.y ?
					thresLow.x ? 512 : 512 - img4.x ://L+L, outside => L+LL vs M/H+LL 
					-(thresHigh.x ? 512 : 256 + img4.x);//H+H, inside => H+HH vs L/M+HH
				if ((dist ^ curdistY1) >= 0)//the same inside/out
					curdistY1 += dist;

				if (thresLow.z == thresLow.w && thresHigh.z == thresHigh.w)//the same situation in the next pix
				{
					dist = curdistY1 + (thresLow.y ? 512 : -512);//dist same sign
					predYDist = dist;
				}
				else
				{
					const int negimg4w = -img4.w;
					const int curdistY2 = thresLow.y ? 512 - img4.w : img4.w - 256;//outside vs inside
					dist = thresLow.y ? negimg4w : 256 + negimg4w;//next: inside vs outside
					predYDist = thresLow.y ? min(curdistY1, curdistY2) : max(curdistY1, curdistY2);//outside(+), find min; inside(-), find max

					quickEnd = true;
				}
			}
			//quick 4-dist(incell) calculation
			const float way4dist2 = incellTriDist2(vload4(y, xdistCol));
			float mindist2 = min(way4dist2, predYDist * predYDist);
			if (quickEnd)
			{
				//const float4 tmp = vload4(y, xdistCol);
				//outCol[outidx] = distRounding2(tmp.y, predYDist);
				outCol[outidx] = distRounding2(mindist2, predYDist);
				quickEnd = false;
			}
			else
			{
				float4 distY = (float4)(1408.0f, 1152.0f, 896.0f, 640.0f);
				float objdy2 = 640.0f * 640.0f;
				float dy2lim = min(mindist2, yf256 * yf256);
				
				local short * restrict minPixPtr = thresLow.y ? minPixLowUp : minPixHighUp;
				for (uint testy = y; testy-- > 0 && objdy2 <= dy2lim; distY += 1024.0f, objdy2 = distY.s3 * distY.s3)
				{
					const float minYDist = distY.s3 + minPixPtr[testy];
					//quick 4-dist calculation
					const float4 testdist4_sqr = vload4(testy, xdistCol);
					const float4 dist_sqr = mad(distY, distY, testdist4_sqr);
					const float min4dist2 = min_hor(dist_sqr);
					mindist2 = min(min(mindist2, minYDist * minYDist), min4dist2);
					dy2lim = min(dy2lim, mindist2);
				}

				distY = (float4)(640.0f, 896.0f, 1152.0f, 1408.0f);
				objdy2 = 640.0f * 640.0f;
				minPixPtr = thresLow.y ? minPixLowDown : minPixHighDown;
				for (uint testy = y + 1; testy < ylim && objdy2 <= dy2lim; distY += 1024.0f, objdy2 = distY.s3 * distY.s3)
				{
					const float minYDist = distY.s3 + minPixPtr[testy];
					//quick 4-dist calculation
					const float4 testdist4_sqr = vload4(testy, xdistCol);
					const float4 dist_sqr = mad(distY, distY, testdist4_sqr);
					const float min4dist2 = min_hor(dist_sqr);
					mindist2 = min(min(mindist2, minYDist * minYDist), min4dist2);
					dy2lim = min(dy2lim, mindist2);
				}
				outCol[outidx] = distRounding2(mindist2, predYDist);

			}
		}
		
	}

}


kernel void graysdf4_2(global const Info * const restrict info, global const uchar * const restrict img, global uchar * const restrict result)
{
	private const uint gid = get_group_id(0);
	private const uint lid = get_local_id(0), lid4 = lid * 4;
	local ushort xdist4[MAX_PIX4 * XD_Stride];
	local short outcache[MAX_PIX4 * XD_Stride];
	private const uint w = info[gid].w, h = info[gid].h, offset = info[gid].offset;
	local uchar imgcache[MAX_PIX4 * XD_Stride];
	
	local ushort distCache[MAX_PIX4 * XD_Stride];
	local ushort * restrict const perRCCache = distCache + lid * XD_Stride;
	local float distCacheF[MAX_PIX4 * XD_Stride4];
	//local float * restrict const pXDist = distCacheF + lid * XD_Stride4;//xdist predicted on each sample point

	for (uint row = 0; row < 4; ++row)//4 rows
	{
		if (lid4 < h)//each row operation
		{
			local uchar * restrict const raw = (local uchar *)(distCacheF) + lid * XD_Stride;
			{
				global const uchar * restrict const rowptr = img + offset + (lid4 + row) * w;
				for (uint x = 0, imgidx = lid4 + row, xlim = w / 4; x < xlim; ++x, imgidx += XD_Stride)//pre-transpose img
				{
					uchar4 img4 = vload4(x, rowptr);
					vstore4(img4, x, raw);
					imgcache[imgidx] = convert_uchar_sat(dot(convert_float4(img4), (float4)(-0.25f, 0.75f, 0.75f, -0.25f)));
				}
			}
			int dist = 64 * 256, adder = 0;
			uchar lastimg = 0;
			bool lastThresNL = false;
			for (uint x = 0; x < w; ++x, dist += adder)
			{
				const uchar objimg = raw[x];
				const bool objThresNL = objimg > THREDHOLD;
				if (lastThresNL == objThresNL)
					perRCCache[x] = dist;
				else
				{
					adder = 256;
					const int dEnter = objimg - 128;
					const int dLeave = (256 + 128) - lastimg;
					const bool isEnter = lastimg < objimg;
					dist = isEnter ? dEnter : dLeave;
					perRCCache[x] = abs(dist);
				}
				lastimg = objimg; lastThresNL = objThresNL;
			}
			dist = perRCCache[w - 1], adder = 0;
			lastimg = 0, lastThresNL = false;
			for (uint x = w; x--; dist += adder)
			{
				const int objPix = perRCCache[x];
				const uchar objimg = raw[x];
				const bool objThresNL = objimg > THREDHOLD;
				if (lastThresNL == objThresNL)
					perRCCache[x] = min(dist, objPix);
				else
				{
					adder = 256;
					const int dEnter = objimg - 128;
					const int dLeave = (256 + 128) - lastimg;
					const bool isEnter = lastimg < objimg;
					dist = isEnter ? dEnter : dLeave;
					perRCCache[x] = min((int)abs(dist), objPix);
				}
				lastimg = objimg; lastThresNL = objThresNL;
			}
			//downsample
			for (uint x = 0, xdidx = lid4 + row, xlim = w / 4; x < xlim; ++x, xdidx += XD_Stride)//pre-transpose xdist
			{
				const float4 dist4 = convert_float4(vload4(x, perRCCache));
				xdist4[xdidx] = convert_ushort_sat(min(dist4.y * 3.0f - dist4.x, dist4.z * 3.0f - dist4.w) * 0.5f);
				//xdist4[xdidx] = clamp(dot(dist4, (float4)(-0.25f, 0.75f, 0.75f, -0.25f)), 0.0f, 65536.0f);
				//xdist4[xdidx] = clamp(dot(dist4, (float4)(0.0f, 0.5f, 0.5f, 0.0f)), 0.0f, 65536.0f);
			}
		}
	}

	//synchronize
	barrier(CLK_LOCAL_MEM_FENCE);

	if (lid4 < w)//has been fit to 32
	{
		local const uchar * restrict const raw = imgcache + lid * XD_Stride;

		local ushort * restrict const xdistCol = xdist4 + lid * XD_Stride;
		local short * restrict const outCol = outcache + lid * XD_Stride;
		
		//why keep xdist:
		//     A------>
		//     B   /
		//     C--------.---->
		//     D
		//updating C means CA gets smaller line than C, C^2 = AC^2 + A^2 
		//when checking D in the rev stage, truth is D^2 = AD^2 + A^2 = (CD+AC)^2 + A^2 = CD^2 + AC^2 + A^2 + 2CD*AC
		//since C has been updated, we will get D^2 = CD^2 + C^2 = CD^2 + AC^2 + A^2, decreased by 2CD*AC
		//so just keep it in the first stage
		
		perRCCache[0] = xdistCol[0];
		uchar lastimg = raw[0];
		bool lastThresNL = false, lastThresNH = true;
		bool quickEnd = false;
		int dist = 64 * 256, adder = 0;
		float yf = 512.0f;
		for (uint y = 1; y < h; y++, dist += adder)
		{
			const int curxdist = xdistCol[y];
			const uchar objimg = raw[y];
			const bool objThresNL = objimg > THREDHOLD, objThresNH = objimg < 255 - THREDHOLD;
			if (!objThresNL)//empty
			{
				if (lastThresNL)//leave edge
				{
					dist = 256 + 128 - lastimg;
					perRCCache[y] = min(curxdist, dist);
					quickEnd = true;
				}
			}
			else if (objThresNH)//not pure white/black -> edge
			{
				dist = objimg - 128;
				perRCCache[y] = abs(dist);
				quickEnd = true;
			}
			else//full, >255-THRES
			{
				if (lastThresNH)//enter edge
				{
					dist = 128 + lastimg;
					perRCCache[y] = min(curxdist, dist);
					quickEnd = true;
				}
			}
			if (quickEnd)
			{
				quickEnd = false;
				adder = 256;
			}
			else
			{	//current pure and last is the same
				float objPix = min(curxdist, dist);
				float curdist2 = objPix * objPix;
				float maxdy2 = min(curdist2, yf * yf), dy2 = 65536.0f, dy256 = 256.0f;
				for (uint testy = y - 1; dy2 < maxdy2; dy2 = dy256 * dy256)
				{
					const uchar oimg = raw[testy];
					const bool oimgThresNL = oimg > THREDHOLD;
					if (objThresNL != oimgThresNL)//edge
					{
						const float negDistOImg = oimgThresNL ? oimg - 128 : 128 - oimg;
						const float oydist = dy256 - negDistOImg;
						objPix = min(oydist, objPix);//objPix has been prove <= curxdist
						break;//further won't be shorter
					}
					const float oxdist = xdistCol[testy];
					const float newdist = oxdist * oxdist + dy2;
					if (newdist < curdist2)
					{
						curdist2 = newdist;
						objPix = sqrt(newdist);
						maxdy2 = min(newdist, maxdy2);
					}
					dy256 += 256.0f; testy--;
				}
				perRCCache[y] = (ushort)objPix;
			}
			lastimg = objimg; lastThresNL = objThresNL; lastThresNH = objThresNH;
			yf += 256.0f;
		}

		//not working since y is always positive --- not continous
		//downsample xdist & conic fit
		//for (uint y = 0, ylim = h / 4; y < ylim; ++y)
		//{
		//	const float4 dist4 = convert_float4(vload4(y, xdistCol));
		//	//assume x-dist fit conic, y=ax^2 + bx + c 
		//	//[Y]   x   y   [Y]   z   w
		//	//[X]  -3  -1    0    1   3
		//	//[Y] = (9(y+z) - (x+w)) / 16
		//	pXDist[y] = dot(dist4, (float4)(-1.0f / 16.0f, 9.0f / 16.0f, 9.0f / 16.0f, -1.0f / 16.0f));
		//}

		//why use perRCCache and rewrite xdistCol:
		//     A------>
		//     B    /
		//     C  /
		//     D--------.----->
		//perRCCache's D means D~A may be smaller than D, which means C may also choose A, where C~A < D~A 
		//when checking C in next stage, C~D became smaller, but still C~D > D~A > C~A, where C~A comes from previos stage
		//xdistCol = D, pXDist = C~D, output =~ C~A

		outCol[h - 1] = perRCCache[h - 1];

		dist = 64 * 256, adder = 0;
		//lastimg, lastThres are all prepared
		quickEnd = false;
		global uchar * restrict output = result + offset + w * (h - 1) + lid;
		yf = 512.0f;
		for (uint y = h - 1; y--; output -= w, dist += adder)
		{
			const uchar objimg = raw[y];
			const bool objThresNL = objimg > THREDHOLD, objThresNH = objimg < 255 - THREDHOLD;
			const int objPix = perRCCache[y];//it should be at most xdist[xdidx]
			if (!objThresNL)//empty
			{
				if (lastThresNL)//leave edge
				{
					dist = 256 + 128 - lastimg;
					//sure: outside, posative
					outCol[y] = min(dist, objPix);
					quickEnd = true;
				}
			}
			else if (objThresNH)//not pure white/black -> edge
			{
				dist = objimg - 128;
				outCol[y] = objimg > 127 ? -(short)objPix : (short)objPix;
				//already changed
				quickEnd = true;
			}
			else//full
			{
				if (lastThresNH)//enter edge
				{
					dist = 128 + lastimg;
					//sure: inside, negative
					outCol[y] = -(short)min(dist, objPix);
					quickEnd = true;
				}
			}
			if (quickEnd)
			{
				quickEnd = false;
				adder = 256;
			}
			else
			{//current pure and last is the same
				float objPixF = (float)min(dist, objPix);
				float curdist2 = objPixF * objPixF;
				float maxdy2 = min(curdist2, yf * yf), dy2 = 65536.0f, dy256 = 256.0f;
				for (uint testy = y + 1; dy2 < maxdy2; dy2 = dy256 * dy256)
				{
					const uchar oimg = raw[testy];
					const bool oimgThresNL = oimg > THREDHOLD;
					if (objThresNL != oimgThresNL)//edge
					{
						const float negDistOImg = oimgThresNL ? oimg - 128 : 128 - oimg;
						const float oydist = dy256 - negDistOImg;
						objPixF = min(oydist, objPixF);
						break;//further won't be shorter
					}
					const float oxdist = perRCCache[testy];//get rid of xdist, it was meant to use local instead of private 
					const float newdist = oxdist * oxdist + dy2;
					if (newdist < curdist2)
					{
						curdist2 = newdist;
						objPixF = sqrt(newdist);
						maxdy2 = min(newdist, maxdy2);
					}
					dy256 += 256.0f; testy++;
				}
				outCol[y] = objimg > 127 ? -(short)objPixF : (short)objPixF;
			}
			lastimg = objimg; lastThresNL = objThresNL; lastThresNH = objThresNH;
			yf += 256.0f;
		}

		
		//for (uint y = 0, retidx = offset / 4 + lid; y < h; ++y, retidx += w / 4)
		//{
		//	//result[retidx] = clamp((float)xdistCol[y] / (raw[y] > 127 ? -16.0f : 32.0f) + 128.0f, 0.0f, 255.0f);
		//	result[retidx] = clamp(outCol[y] / (outCol[y] < 0 ? 16.0f : 32.0f) + 128.0f, 0.0f, 255.0f);
		//	//result[retidx] = raw[y];
		//}
		//downsample
		for (uint y = 0, retidx = offset / 16 + lid, ylim = h / 4; y < ylim; ++y, retidx += w / 4)
		{
			const float4 dist4 = convert_float4(vload4(y, outCol));
			const float4 img4 = convert_float4(vload4(y, raw));
			const float avgDist = dot(dist4, (float4)(-0.25f, 0.75f, 0.75f, -0.25f));
			//const float avgDist = dot(dist4, (float4)(0.0f, 0.5f, 0.5f, 0.0f));
			result[retidx] = convert_uchar_sat(avgDist / (avgDist < 0 ? 10.0f : 16.0f) + 128.0f);
		}
	}
}


#define THREDHOLD 16
#define MAX_PIX 128
#define XD_Stride 129

#if LOC_MEM_SIZE < 50000//small local memory,save raw cache to private
kernel void graysdf(global const Info * const restrict info, global const uchar * const restrict img, global short * const restrict result)
{
	private const uint gid = get_group_id(0);
	private const uint lid = get_local_id(0);
	//local float sq256LUT[MAX_PIX];
	local ushort xdist[MAX_PIX * XD_Stride];
	private const uint w = info[gid].w, h = info[gid].h, offset = info[gid].offset;
	private uchar raw[MAX_PIX];
	//each row operation
	if (lid < h)
	{
		local ushort * restrict perRC2 = xdist + XD_Stride * lid;
		int dist = 64 * 256, adder = 0;
		{
			global const uchar * restrict const rowptr = img + offset + lid * w;
			//prefetch(rowptr, (size_t)w);
			for (uint x = 0, xlim = w / 4; x < xlim; ++x)
				vstore4(vload4(x, rowptr), x, raw);
		}
		uchar lastimg = 0;
		bool lastThresNL = false;
		for (uint x = 0; x < w; ++x, dist += adder)
		{
			const uchar objimg = raw[x];
			const bool objThresNL = objimg > THREDHOLD;
			if (lastThresNL == objThresNL)
				perRC2[x] = dist;
			else
			{
				adder = 256;
				const int dEnter = objimg - 128;
				const int dLeave = (256 + 128) - lastimg;
				const bool isEnter = lastimg < objimg;
				dist = isEnter ? dEnter : dLeave;
				perRC2[x] = abs(dist);
			}
			lastimg = objimg; lastThresNL = objThresNL;
		}
		dist = perRC2[w - 1], adder = 0;
		lastimg = 0, lastThresNL = false;
		for (uint x = w; x--; dist += adder)
		{
			const int objPix = perRC2[x];
			const uchar objimg = raw[x];
			const bool objThresNL = objimg > THREDHOLD;
			if (lastThresNL == objThresNL)
				perRC2[x] = min(dist, objPix);
			else
			{
				adder = 256;
				const int dEnter = objimg - 128;
				const int dLeave = (256 + 128) - lastimg;
				const bool isEnter = lastimg < objimg;
				dist = isEnter ? dEnter : dLeave;
				perRC2[x] = min((int)abs(dist), objPix);
			}
			lastimg = objimg; lastThresNL = objThresNL;
		}
	}
	/*event_t evt = async_work_group_copy(sq256LUT, sq256LUT2, MAX_PIX, 0);
	wait_group_events(1, &evt);*/
	//synchronize
	barrier(CLK_LOCAL_MEM_FENCE);
	if (lid < w)
	{
		//why keep xdist:
		//     A------>
		//     B   /
		//     C--------.---->
		//     D
		//updating C means CA gets smaller line than C, C^2 = AC^2 + A^2 
		//when checking D in the rev stage, truth is D^2 = AD^2 + A^2 = (CD+AC)^2 + A^2 = CD^2 + AC^2 + A^2 + 2CD*AC
		//since C has been updated, we will get D^2 = CD^2 + C^2 = CD^2 + AC^2 + A^2, decreased by 2CD*AC
		//so just keep it in the first stage
		private ushort newCol[MAX_PIX];
		uint xdidx = lid;
		for (uint y = 0, tmpidx = lid + offset; y < h; ++y, tmpidx += w)
			raw[y] = img[tmpidx];
		newCol[0] = xdist[xdidx];
		uchar lastimg = raw[0];
		bool lastThresNL = false, lastThresNH = true;
		bool quickEnd = false;
		int dist = 64 * 256, adder = 0;
		float yf = 512.0f;
		for (uint y = 1; y < h; y++, dist += adder)
		{
			uint testidx = xdidx;
			xdidx += XD_Stride;
			const int curxdist = xdist[xdidx];
			const uchar objimg = raw[y];
			const bool objThresNL = objimg > THREDHOLD, objThresNH = objimg < 255 - THREDHOLD;
			if (!objThresNL)//empty
			{
				if (lastThresNL)//leave edge
				{
					dist = 256 + 128 - lastimg;
					newCol[y] = min(curxdist, dist);
					quickEnd = true;
				}
			}
			else if (objThresNH)//not pure white/black -> edge
			{
				dist = objimg - 128;
				newCol[y] = abs(dist);
				quickEnd = true;
			}
			else//full, >255-THRES
			{
				if (lastThresNH)//enter edge
				{
					dist = 128 + lastimg;
					newCol[y] = min(curxdist, dist);
					quickEnd = true;
				}
			}
			if (quickEnd)
			{
				quickEnd = false;
				adder = 256;
			}
			else
			{	//current pure and last is the same
				float objPix = min(curxdist, dist);
				float curdist2 = objPix * objPix;
				float maxdy2 = min(curdist2, yf * yf), dy2 = 65536.0f, dy256 = 256.0f;
				for (uint testy = y - 1; dy2 < maxdy2; dy2 = dy256 * dy256, testidx -= XD_Stride)
				{
					const uchar oimg = raw[testy];
					const bool oimgThresNL = oimg > THREDHOLD;
					if (objThresNL != oimgThresNL)//edge
					{
						const float negDistOImg = oimgThresNL ? oimg - 128 : 128 - oimg;
						const float oydist = dy256 - negDistOImg;
						objPix = min(oydist, objPix);//objPix has been prove <= curxdist
						break;//further won't be shorter
					}
					const float oxdist = xdist[testidx];
					const float newdist = oxdist * oxdist + dy2;
					if (newdist < curdist2)
					{
						curdist2 = newdist;
						objPix = sqrt(newdist);
						maxdy2 = min(newdist, maxdy2);
					}
					dy256 += 256.0f; testy--;
				}
				newCol[y] = (ushort)objPix;
			}
			lastimg = objimg; lastThresNL = objThresNL; lastThresNH = objThresNH;
			yf += 256.0f;
		}
		//why not update xdist:
		//     A------>
		//     B    /
		//     C  /
		//     D--------.----->
		//updating D means DA gets smaller line than D, which means C may also choose A, where CA < DA 
		//when checking C in next stage, CD became smaller, but still CD > DA > CA, where CA comes from previos stage
		//but no need to update, since objPix self has lower dist, copying cost extra time.

		//for (uint y = 1, xdidx2 = lid + XD_Stride; y < h; y++, xdidx2 += XD_Stride)
		//	xdist[xdidx2] = newCol[y];

		dist = 64 * 256, adder = 0;
		//lastimg, lastThres are all prepared
		quickEnd = false;
		global short * restrict output = result + offset + w * (h - 1) + lid;
		yf = 512.0f;
		for (uint y = h - 1; y--; output -= w, dist += adder)
		{
			uint testidx = xdidx;
			xdidx -= XD_Stride;
			const uchar objimg = raw[y];
			const bool objThresNL = objimg > THREDHOLD, objThresNH = objimg < 255 - THREDHOLD;
			const int objPix = newCol[y];//it should be at most xdist[xdidx]
			if (!objThresNL)//empty
			{
				if (lastThresNL)//leave edge
				{
					dist = 256 + 128 - lastimg;
					//sure: outside, posative
					*output = (short)min(dist, objPix);
					quickEnd = true;
				}
			}
			else if (objThresNH)//not pure white/black -> edge
			{
				dist = objimg - 128;
				*output = objimg > 127 ? -(short)objPix : (short)objPix;
				//already changed
				quickEnd = true;
			}
			else//full
			{
				if (lastThresNH)//enter edge
				{
					dist = 128 + lastimg;
					//sure: inside, negative
					*output = -(short)min(dist, objPix);
					quickEnd = true;
				}
			}
			if (quickEnd)
			{
				quickEnd = false;
				adder = 256;
			}
			else
			{//current pure and last is the same
				float objPixF = (float)min(dist, objPix);
				float curdist2 = objPixF * objPixF;
				float maxdy2 = min(curdist2, yf * yf), dy2 = 65536.0f, dy256 = 256.0f;
				for (uint testy = y + 1; dy2 < maxdy2; dy2 = dy256 * dy256, testidx += XD_Stride)
				{
					const uchar oimg = raw[testy];
					const bool oimgThresNL = oimg > THREDHOLD;
					if (objThresNL != oimgThresNL)//edge
					{
						const float negDistOImg = oimgThresNL ? oimg - 128 : 128 - oimg;
						const float oydist = dy256 - negDistOImg;
						objPixF = min(oydist, objPixF);
						break;//further won't be shorter
					}
					const float oxdist = xdist[testidx];
					const float newdist = oxdist * oxdist + dy2;
					if (newdist < curdist2)
					{
						curdist2 = newdist;
						objPixF = sqrt(newdist);
						maxdy2 = min(newdist, maxdy2);
					}
					dy256 += 256.0f; testy++;
				}
				*output = objimg > 127 ? -(short)objPixF : (short)objPixF;
			}
			lastimg = objimg; lastThresNL = objThresNL; lastThresNH = objThresNH;
			yf += 256.0f;
		}
	}
}

#else //larger local memory, put image cache to local

kernel void graysdf(global const Info * const restrict info, global const uchar * const restrict img, global short * const restrict result)
{
	private const uint gid = get_group_id(0);
	private const uint lid = get_local_id(0);
	local ushort xdist[MAX_PIX * XD_Stride];
	local uchar raws[MAX_PIX * XD_Stride];
	private const uint w = info[gid].w, h = info[gid].h, offset = info[gid].offset;
	//private uchar raw[MAX_PIX];
	//each row operation
	if (lid < h)
	{
		local uchar * restrict const raw = raws + XD_Stride * lid;
		local ushort * restrict const perRC2 = xdist + XD_Stride * lid;
		int dist = 64 * 256, adder = 0;
		{
			global const uchar * restrict const rowptr = img + offset + lid * w;
			for (uint x = 0, xlim = w / 4; x < xlim; ++x)
				vstore4(vload4(x, rowptr), x, raw);
		}
		uchar lastimg = 0;
		bool lastThresNL = false;
		for (uint x = 0; x < w; ++x, dist += adder)
		{
			const uchar objimg = raw[x];
			const bool objThresNL = objimg > THREDHOLD;
			if (lastThresNL == objThresNL)
				perRC2[x] = dist;
			else
			{
				adder = 256;
				const int dEnter = objimg - 128;
				const int dLeave = (256 + 128) - lastimg;
				const bool isEnter = lastimg < objimg;
				dist = isEnter ? dEnter : dLeave;
				perRC2[x] = abs(dist);
			}
			lastimg = objimg; lastThresNL = objThresNL;
		}
		dist = perRC2[w - 1], adder = 0;
		lastimg = 0, lastThresNL = false;
		for (uint x = w; x--; dist += adder)
		{
			const int objPix = perRC2[x];
			const uchar objimg = raw[x];
			const bool objThresNL = objimg > THREDHOLD;
			if (lastThresNL == objThresNL)
				perRC2[x] = min(dist, objPix);
			else
			{
				adder = 256;
				const int dEnter = objimg - 128;
				const int dLeave = (256 + 128) - lastimg;
				const bool isEnter = lastimg < objimg;
				dist = isEnter ? dEnter : dLeave;
				perRC2[x] = min((int)abs(dist), objPix);
			}
			lastimg = objimg; lastThresNL = objThresNL;
		}
	}
	/*event_t evt = async_work_group_copy(sq256LUT, sq256LUT2, MAX_PIX, 0);
	wait_group_events(1, &evt);*/
	//synchronize
	barrier(CLK_LOCAL_MEM_FENCE);
	if (lid < w)
	{
		private ushort newCol[MAX_PIX];
		uint xdidx = lid;
		newCol[0] = xdist[xdidx];
		uchar lastimg = raws[lid];
		bool lastThresNL = false, lastThresNH = true;
		bool quickEnd = false;
		int dist = 64 * 256, adder = 0;
		float yf = 512.0f;
		for (uint y = 1; y < h; y++, dist += adder)
		{
			uint testidx = xdidx;
			xdidx += XD_Stride;
			const int curxdist = xdist[xdidx];
			const uchar objimg = raws[xdidx];
			const bool objThresNL = objimg > THREDHOLD, objThresNH = objimg < 255 - THREDHOLD;
			if (!objThresNL)//empty
			{
				if (lastThresNL)//leave edge
				{
					dist = 256 + 128 - lastimg;
					newCol[y] = min(curxdist, dist);
					quickEnd = true;
				}
			}
			else if (objThresNH)//not pure white/black -> edge
			{
				dist = objimg - 128;
				newCol[y] = abs(dist);
				quickEnd = true;
			}
			else//full, >255-THRES
			{
				if (lastThresNH)//enter edge
				{
					dist = 128 + lastimg;
					newCol[y] = min(curxdist, dist);
					quickEnd = true;
				}
			}
			if (quickEnd)
			{
				quickEnd = false;
				adder = 256;
			}
			else
			{	//current pure and last is the same
				float objPix = min(curxdist, dist);
				float curdist2 = objPix * objPix;
				float maxdy2 = min(curdist2, yf * yf), dy2 = 65536.0f, dy256 = 256.0f;
				for (; dy2 < maxdy2; dy2 = dy256 * dy256, testidx -= XD_Stride)
				{
					const uchar oimg = raws[testidx];
					const bool oimgThresNL = oimg > THREDHOLD;
					if (objThresNL != oimgThresNL)//edge
					{
						const float negDistOImg = oimgThresNL ? oimg - 128 : 128 - oimg;
						const float oydist = dy256 - negDistOImg;
						objPix = min(oydist, objPix);//objPix has been prove <= curxdist
						break;//further won't be shorter
					}
					const float oxdist = xdist[testidx];
					const float newdist = oxdist * oxdist + dy2;
					if (newdist < curdist2)
					{
						curdist2 = newdist;
						objPix = sqrt(newdist);
						maxdy2 = min(newdist, maxdy2);
					}
					dy256 += 256.0f;
				}
				newCol[y] = (ushort)objPix;
			}
			lastimg = objimg; lastThresNL = objThresNL; lastThresNH = objThresNH;
			yf += 256.0f;
		}
		dist = 64 * 256, adder = 0;
		//lastimg, lastThres are all prepared
		quickEnd = false;
		global short * restrict output = result + offset + w * (h - 1) + lid;
		yf = 512.0f;
		for (uint y = h - 1; y--; output -= w, dist += adder)
		{
			uint testidx = xdidx;
			xdidx -= XD_Stride;
			const uchar objimg = raws[xdidx];
			const bool objThresNL = objimg > THREDHOLD, objThresNH = objimg < 255 - THREDHOLD;
			const int objPix = newCol[y];// no need to check xdist[idx], it is checked in previous pass
			if (!objThresNL)//empty
			{
				if (lastThresNL)//leave edge
				{
					dist = 256 + 128 - lastimg;
					//sure: outside, posative
					*output = (short)min(dist, objPix);
					quickEnd = true;
				}
			}
			else if (objThresNH)//not pure white/black -> edge
			{
				dist = objimg - 128;
				*output = objimg > 127 ? -(short)objPix : (short)objPix;
				//already changed
				quickEnd = true;
			}
			else//full
			{
				if (lastThresNH)//enter edge
				{
					dist = 128 + lastimg;
					//sure: inside, negative
					*output = -(short)min(dist, objPix);
					quickEnd = true;
				}
			}
			if (quickEnd)
			{
				quickEnd = false;
				adder = 256;
			}
			else
			{//current pure and last is the same
				float objPixF = (float)min(dist, objPix);
				float curdist2 = objPixF * objPixF;
				float maxdy2 = min(curdist2, yf * yf), dy2 = 65536.0f, dy256 = 256.0f;
				for (; dy2 < maxdy2; dy2 = dy256 * dy256, testidx += XD_Stride)
				{
					const uchar oimg = raws[testidx];
					const bool oimgThresNL = oimg > THREDHOLD;
					if (objThresNL != oimgThresNL)//edge
					{
						const float negDistOImg = oimgThresNL ? oimg - 128 : 128 - oimg;
						const float oydist = dy256 - negDistOImg;
						objPixF = min(oydist, objPixF);
						break;//further won't be shorter
					}
					const float oxdist = xdist[testidx];
					const float newdist = oxdist * oxdist + dy2;
					if (newdist < curdist2)
					{
						curdist2 = newdist;
						objPixF = sqrt(newdist);
						maxdy2 = min(newdist, maxdy2);
					}
					dy256 += 256.0f;
				}
				*output = objimg > 127 ? -(short)objPixF : (short)objPixF;
			}
			lastimg = objimg; lastThresNL = objThresNL; lastThresNH = objThresNH;
			yf += 256.0f;
		}
	}
}

#endif
