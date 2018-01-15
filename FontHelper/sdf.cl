typedef struct Info
{
	uint offset;
	uchar w;
	uchar h;
}Info;

kernel void bmpsdf(global const Info* restrict info, global read_only uchar* restrict img, global ushort* restrict result)
{
	private const uint gid = get_group_id(0);
	private const uint lid = get_local_id(0);
	local ushort sqLUT[160];
	local uchar xdist[160 * 160];
	const uint w = info[gid].w, h = info[gid].h, offset = info[gid].offset;
	private uchar raw[160];
	//setup square-LUT
	sqLUT[lid] = lid*lid;
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

#if defined(NVIDIA)//nvidia, don't use local memory optimization

kernel void graysdf(constant float *sq256LUT, global const Info *info, global uchar *img, global short *result)
{
	private const uint gid = get_group_id(0);
	private const uint lid = get_local_id(0);
	local ushort xdist[144 * 144];
	private const uint w = info[gid].w, h = info[gid].h, offset = info[gid].offset;
	private uchar raw[144];
	private ushort perRC[144];
	//each row operation
	if (lid < h)
	{
		uint idx = lid * w;
		ushort dist = 64 * 256, adder = 0;
		uchar curimg = 0;
		{
			global const uchar *rowptr = &img[idx + offset];
			for (uint x = 0, xlim = w / 4; x < xlim; ++x)
				vstore4(vload4(x, rowptr), x, raw);
		}
		for (uint x = 0; x < w; ++x, dist += adder)
		{
			const uchar objimg = raw[x];
			if (curimg <= THREDHOLD)
			{
				if (objimg > THREDHOLD)//enter edge
				{
					dist = objimg - 128, adder = 256;
					perRC[x] = abs(128 - objimg);
				}
				else
					perRC[x] = dist;
			}
			else
			{
				if (objimg <= THREDHOLD)//leave edge
					perRC[x] = dist = (256 + 128) - curimg, adder = 256;
				else
					perRC[x] = dist;
			}
			curimg = objimg;
		}
		dist = perRC[w - 1], adder = 0, curimg = 0;
		idx += w - 1;
		for (uint x = w; x--; --idx, dist += adder)
		{
			const uchar objimg = raw[x];
			const ushort objPix = perRC[x];
			if (curimg <= THREDHOLD)
			{
				if (objimg > THREDHOLD)//enter edge
				{
					dist = objimg - 128, adder = 256;
					xdist[idx] = min((ushort)abs(128 - objimg), objPix);
				}
				else
					xdist[idx] = min(dist, objPix);
			}
			else
			{
				if (objimg <= THREDHOLD)//leave edge
				{
					dist = (256 + 128) - curimg, adder = 256;
					xdist[idx] = min(dist, objPix);
				}
				else
					xdist[idx] = min(dist, objPix);
			}
			curimg = objimg;
		}
	}
	//synchronize
	barrier(CLK_LOCAL_MEM_FENCE);
	if (lid < w)
	{
		uint idx = lid;
		for (uint y = 0, tmpidx = lid + offset; y < h; ++y, tmpidx += w)
			raw[y] = img[tmpidx];
		perRC[0] = xdist[idx];
		uchar curimg = raw[0];
		ushort dist = 64 * 256, adder = 0;
		bool isLastPure = true;
		for (uint y = 1; y < h; curimg = raw[y++], dist += adder)
		{
			uint testidx = idx;
			idx += w;
			const uchar objimg = raw[y];
			const ushort curxdist = xdist[idx];
			if (objimg <= THREDHOLD)//empty
			{
				if (curimg > THREDHOLD)//leave edge
				{
					dist = 256 + 128 - curimg, adder = 256;
					perRC[y] = min(curxdist, dist);
					continue;
				}
			}
			else if (objimg < 255 - THREDHOLD)//not pure white/black -> edge
			{
				dist = objimg - 128, adder = 256;
				perRC[y] = abs(objimg - 128);
				continue;
			}
			else//full
			{
				if (curimg < 255 - THREDHOLD)//enter edge
				{
					dist = 128 + curimg, adder = 256;
					perRC[y] = min(curxdist, dist);
					continue;
				}
			}
			//current pure and last is the same
			ushort objPix = min(curxdist, dist);
			float curdist2 = (float)objPix * (float)objPix;
			float maxdy2 = min(curdist2, sq256LUT[y + 1]), dy2 = 65536.0f;
			for (uint dy = 1; dy2 < maxdy2; dy2 = sq256LUT[dy], testidx -= w)
			{
				const uchar oimg = raw[y - dy];
				if (objimg <= THREDHOLD)//empty
				{
					if (oimg > THREDHOLD)//edge
					{
						const ushort oydist = dy * 256 + 128 - oimg;
						objPix = min(oydist, objPix);//objPix has been prove <= curxdist
						break;//further won't be shorter
					}
				}
				else//full
				{
					if (oimg <= THREDHOLD)//edge
					{
						const ushort oydist = dy * 256 - 128 + oimg;
						objPix = min(oydist, objPix);//objPix has been prove <= curxdist
						break;//further won't be shorter
					}
				}
				const ushort oxdist = xdist[testidx];
				const float newdist = oxdist * oxdist + dy2; //sqLUT[oxdist] + dy2;
				if (newdist < curdist2)
				{
					curdist2 = newdist;
					objPix = (ushort)sqrt(newdist);
					maxdy2 = min(newdist, maxdy2);
				}
				dy++;
			}
			perRC[y] = objPix;
		}
		dist = 64 * 256, adder = 0;
		curimg = raw[h - 1];
		for (uint y = h - 1; y--; curimg = raw[y], dist += adder)
		{
			uint testidx = idx;
			idx -= w;
			const uchar objimg = raw[y];
			ushort objPix = perRC[y];// no need to check xdist[idx], it is checked in previous pass
			if (objimg <= THREDHOLD)//empty
			{
				if (curimg > THREDHOLD)//leave edge
				{
					dist = 256 + 128 - curimg, adder = 256;
					perRC[y] = min(dist, objPix);
					continue;
				}
			}
			else if (objimg < 255 - THREDHOLD)//not pure white/black -> edge
			{
				dist = objimg - 128, adder = 256;
				//already changed
				continue;
			}
			else//full
			{
				if (curimg < 255 - THREDHOLD)//enter edge
				{
					dist = 128 + curimg, adder = 256;
					perRC[y] = min(dist, objPix);
					continue;
				}
			}
			//current pure and last is the same
			float curdist2 = (float)objPix * (float)objPix;
			float maxdy2 = min(curdist2, sq256LUT[h - y - 1]), dy2 = 65536.0f;
			for (uint dy = 1; dy2 < maxdy2; dy2 = sq256LUT[dy], testidx += w)
			{
				const uchar oimg = raw[y + dy];
				if (objimg <= THREDHOLD)//empty
				{
					if (oimg > THREDHOLD)//edge
					{
						const ushort oydist = dy * 256 + 128 - oimg;
						objPix = min(oydist, objPix);
						break;//further won't be shorter
					}
				}
				else//full
				{
					if (oimg <= THREDHOLD)//edge
					{
						const ushort oydist = dy * 256 - 128 + oimg;
						objPix = min(oydist, objPix);
						break;//further won't be shorter
					}
				}
				const ushort oxdist = xdist[testidx];
				const float newdist = oxdist * oxdist + dy2; //sqLUT[oxdist] + dy2;
				if (newdist < curdist2)
				{
					curdist2 = newdist;
					objPix = (ushort)sqrt(newdist);
					maxdy2 = min(newdist, maxdy2);
				}
				dy++;
			}
			perRC[y] = objPix;
		}
		idx = offset + lid;
		for (uint y = 0; y < h; ++y, idx += w)
			result[idx] = raw[y] > 127 ? -(short)perRC[y] : (short)perRC[y];
	}
}

#elif LOC_MEM_SIZE >= 65536 //64k local memory, put image cache to local

kernel void graysdf(constant float *sq256LUT, global const Info *info, global const uchar *img, global short *result)
{
	private const uint gid = get_group_id(0);
	private const uint lid = get_local_id(0);
	private const uint w = info[gid].w, h = info[gid].h, offset = info[gid].offset;
	local ushort tmp[144 * 144];
	local uchar imgcache[144 * 144];
	local uchar * const raw = imgcache + lid * 144;
	//global ushort *xdist = distcache + 144 * 144 * (gid % 24);
	//each row operation
	if (lid < h)
	{
		local ushort * const rPerRC = tmp + lid;
		ushort dist = 64 * 256, adder = 0;
		uchar curimg = 0;
		{
			global const uchar * const rowptr = &img[offset + w * lid];
			for (uint x = 0, xlim = w / 4; x < xlim; ++x)
				vstore4(vload4(x, rowptr), x, raw);
		}
		uint idx = 0;
		for (uint x = 0, rprc = 0; x < w; ++x, dist += adder, idx += 144)
		{
			const uchar objimg = raw[x];
			if (curimg <= THREDHOLD)
			{
				if (objimg > THREDHOLD)//enter edge
				{
					dist = objimg - 128, adder = 256;
					rPerRC[idx] = abs(128 - objimg);
				}
				else
					rPerRC[idx] = dist;
			}
			else
			{
				if (objimg <= THREDHOLD)//leave edge
					rPerRC[idx] = dist = (256 + 128) - curimg, adder = 256;
				else
					rPerRC[idx] = dist;
			}
			curimg = objimg;
		}
		idx -= 144;
		dist = rPerRC[idx], adder = 0, curimg = 0;
		for (uint x = w; x--; dist += adder, idx -= 144)
		{
			const uchar objimg = raw[x];
			const ushort objPix = rPerRC[idx];
			if (curimg <= THREDHOLD)
			{
				if (objimg > THREDHOLD)//enter edge
				{
					dist = objimg - 128, adder = 256;
					rPerRC[idx] = min((ushort)abs(128 - objimg), objPix);
				}
				else
					rPerRC[idx] = min(dist, objPix);
			}
			else
			{
				if (objimg <= THREDHOLD)//leave edge
				{
					dist = (256 + 128) - curimg, adder = 256;
					rPerRC[idx] = min(dist, objPix);
				}
				else
					rPerRC[idx] = min(dist, objPix);
			}
			curimg = objimg;
		}
	}
	//synchronize
	barrier(CLK_LOCAL_MEM_FENCE);
	if (lid < w)
	{
		local ushort * const perRC = &tmp[lid * 144];//has been prepared.
		private float xdist2[144];
		uint idx = lid;
		for (uint y = 0, imgidx = lid + offset; y < h; ++y, imgidx += w)
			raw[y] = img[imgidx];
		for (uint i = 0, ilim = h / 4; i < ilim; ++i)
		{
			const float4 di = convert_float4(vload4(i, perRC));
			vstore4(di*di, i, xdist2);
		}
		uchar curimg = raw[0];
		ushort dist = 64 * 256, adder = 0;
		bool isLastPure = true;
		for (uint y = 1; y < h; curimg = raw[y++], dist += adder)
		{
			const uchar objimg = raw[y];
			const ushort curxdist = perRC[y];
			if (objimg <= THREDHOLD)//empty
			{
				if (curimg > THREDHOLD)//leave edge
				{
					dist = 256 + 128 - curimg, adder = 256;
					perRC[y] = min(curxdist, dist);
					continue;
				}
			}
			else if (objimg < 255 - THREDHOLD)//not pure white/black -> edge
			{
				dist = objimg - 128, adder = 256;
				perRC[y] = abs(objimg - 128);
				continue;
			}
			else//full
			{
				if (curimg < 255 - THREDHOLD)//enter edge
				{
					dist = 128 + curimg, adder = 256;
					perRC[y] = min(curxdist, dist);
					continue;
				}
			}
			//current pure and last is the same
			ushort objPix = min(curxdist, dist);
			float curdist2 = (float)objPix * (float)objPix;
			float maxdy2 = min(curdist2, sq256LUT[y + 1]), dy2 = 65536.0f;
			for (uint dy = 1, cury = y; dy2 < maxdy2; dy2 = sq256LUT[++dy], cury--)
			{
				const uchar oimg = raw[y - dy];
				if (objimg <= THREDHOLD)//empty
				{
					if (oimg > THREDHOLD)//edge
					{
						const ushort oydist = dy * 256 + 128 - oimg;
						objPix = min(oydist, objPix);//objPix has been prove <= curxdist
						break;//further won't be shorter
					}
				}
				else//full
				{
					if (oimg <= THREDHOLD)//edge
					{
						const ushort oydist = dy * 256 - 128 + oimg;
						objPix = min(oydist, objPix);//objPix has been prove <= curxdist
						break;//further won't be shorter
					}
				}
				const float newdist = xdist2[cury] + dy2;
				if (newdist < curdist2)
				{
					curdist2 = newdist;
					objPix = (ushort)sqrt(newdist);
					maxdy2 = min(newdist, maxdy2);
				}
			}
			perRC[y] = objPix;
		}
		dist = 64 * 256, adder = 0;
		curimg = raw[h - 1];
		for (uint y = h - 1; y--; curimg = raw[y], dist += adder)
		{
			const uchar objimg = raw[y];
			ushort objPix = perRC[y];// no need to check xdist[idx], it is checked in previous pass
			if (objimg <= THREDHOLD)//empty
			{
				if (curimg > THREDHOLD)//leave edge
				{
					dist = 256 + 128 - curimg, adder = 256;
					perRC[y] = min(dist, objPix);
					continue;
				}
			}
			else if (objimg < 255 - THREDHOLD)//not pure white/black -> edge
			{
				dist = objimg - 128, adder = 256;
				//already changed
				continue;
			}
			else//full
			{
				if (curimg < 255 - THREDHOLD)//enter edge
				{
					dist = 128 + curimg, adder = 256;
					perRC[y] = min(dist, objPix);
					continue;
				}
			}
			//current pure and last is the same
			float curdist2 = (float)objPix * (float)objPix;
			float maxdy2 = min(curdist2, sq256LUT[h - y - 1]), dy2 = 65536.0f;
			for (uint dy = 1, cury = y; dy2 < maxdy2; dy2 = sq256LUT[++dy], cury++)
			{
				const uchar oimg = raw[y + dy];
				if (objimg <= THREDHOLD)//empty
				{
					if (oimg > THREDHOLD)//edge
					{
						const ushort oydist = dy * 256 + 128 - oimg;
						objPix = min(oydist, objPix);
						break;//further won't be shorter
					}
				}
				else//full
				{
					if (oimg <= THREDHOLD)//edge
					{
						const ushort oydist = dy * 256 - 128 + oimg;
						objPix = min(oydist, objPix);
						break;//further won't be shorter
					}
				}
				const float newdist = xdist2[cury] + dy2;
				if (newdist < curdist2)
				{
					curdist2 = newdist;
					objPix = (ushort)sqrt(newdist);
					maxdy2 = min(newdist, maxdy2);
				}
			}
			perRC[y] = objPix;
		}
		idx = offset + lid;
		for (uint y = 0; y < h; ++y, idx += w)
			result[idx] = raw[y] > 127 ? -(short)perRC[y] : (short)perRC[y];
	}
}

#else//less than 64k local memory, image stays in global

kernel void graysdf(constant float *sq256LUT, global const Info *info, global const uchar *img, global short *result)
{
	private const uint gid = get_group_id(0);
	private const uint lid = get_local_id(0);
	private const uint w = info[gid].w, h = info[gid].h, offset = info[gid].offset;
	local ushort tmp[144 * 144];
	//each row operation
	if (lid < h)
	{
		local ushort * const rPerRC = tmp + lid;
		ushort dist = 64 * 256, adder = 0;
		uchar curimg = 0;
		global const uchar * const raw = img + offset + lid * w;
		uint idx = 0;
		for (uint x = 0, rprc = 0; x < w; ++x, dist += adder, idx += 144)
		{
			const uchar objimg = raw[x];
			if (curimg <= THREDHOLD)
			{
				if (objimg > THREDHOLD)//enter edge
				{
					dist = objimg - 128, adder = 256;
					rPerRC[idx] = abs(128 - objimg);
				}
				else
					rPerRC[idx] = dist;
			}
			else
			{
				if (objimg <= THREDHOLD)//leave edge
					rPerRC[idx] = dist = (256 + 128) - curimg, adder = 256;
				else
					rPerRC[idx] = dist;
			}
			curimg = objimg;
		}
		idx -= 144;
		dist = rPerRC[idx], adder = 0, curimg = 0;
		for (uint x = w; x--; dist += adder, idx -= 144)
		{
			const uchar objimg = raw[x];
			const ushort objPix = rPerRC[idx];
			if (curimg <= THREDHOLD)
			{
				if (objimg > THREDHOLD)//enter edge
				{
					dist = objimg - 128, adder = 256;
					rPerRC[idx] = min((ushort)abs(128 - objimg), objPix);
				}
				else
					rPerRC[idx] = min(dist, objPix);
			}
			else
			{
				if (objimg <= THREDHOLD)//leave edge
				{
					dist = (256 + 128) - curimg, adder = 256;
					rPerRC[idx] = min(dist, objPix);
				}
				else
					rPerRC[idx] = min(dist, objPix);
			}
			curimg = objimg;
		}
	}
	//synchronize
	barrier(CLK_LOCAL_MEM_FENCE);
	if (lid < w)
	{
		local ushort * const perRC = &tmp[lid * 144];//has been prepared.
		private float xdist2[144];
		global const uchar * const raw = img + offset + lid;
		for (uint i = 0, ilim = h / 4; i < ilim; ++i)
		{
			const float4 di = convert_float4(vload4(i, perRC));
			vstore4(di*di, i, xdist2);
		}
		uchar curimg = raw[0];
		ushort dist = 64 * 256, adder = 0;
		bool isLastPure = true;
		for (uint y = 1; y < h; curimg = raw[w * y++], dist += adder)
		{
			const uchar objimg = raw[w * y];
			const ushort curxdist = perRC[y];
			if (objimg <= THREDHOLD)//empty
			{
				if (curimg > THREDHOLD)//leave edge
				{
					dist = 256 + 128 - curimg, adder = 256;
					perRC[y] = min(curxdist, dist);
					continue;
				}
			}
			else if (objimg < 255 - THREDHOLD)//not pure white/black -> edge
			{
				dist = objimg - 128, adder = 256;
				perRC[y] = abs(objimg - 128);
				continue;
			}
			else//full
			{
				if (curimg < 255 - THREDHOLD)//enter edge
				{
					dist = 128 + curimg, adder = 256;
					perRC[y] = min(curxdist, dist);
					continue;
				}
			}
			//current pure and last is the same
			ushort objPix = min(curxdist, dist);
			float curdist2 = (float)objPix * (float)objPix;
			float maxdy2 = min(curdist2, sq256LUT[y + 1]), dy2 = 65536.0f;
			for (uint dy = 1, cury = y; dy2 < maxdy2; dy2 = sq256LUT[++dy], cury--)
			{
				const uchar oimg = raw[w * (y - dy)];
				if (objimg <= THREDHOLD)//empty
				{
					if (oimg > THREDHOLD)//edge
					{
						const ushort oydist = dy * 256 + 128 - oimg;
						objPix = min(oydist, objPix);//objPix has been prove <= curxdist
						break;//further won't be shorter
					}
				}
				else//full
				{
					if (oimg <= THREDHOLD)//edge
					{
						const ushort oydist = dy * 256 - 128 + oimg;
						objPix = min(oydist, objPix);//objPix has been prove <= curxdist
						break;//further won't be shorter
					}
				}
				const float newdist = xdist2[cury] + dy2;
				if (newdist < curdist2)
				{
					curdist2 = newdist;
					objPix = (ushort)sqrt(newdist);
					maxdy2 = min(newdist, maxdy2);
				}
			}
			perRC[y] = objPix;
		}
		dist = 64 * 256, adder = 0;
		curimg = raw[w * (h - 1)];
		for (uint y = h - 1; y--; curimg = raw[w * y], dist += adder)
		{
			const uchar objimg = raw[w * y];
			ushort objPix = perRC[y];// no need to check xdist[idx], it is checked in previous pass
			if (objimg <= THREDHOLD)//empty
			{
				if (curimg > THREDHOLD)//leave edge
				{
					dist = 256 + 128 - curimg, adder = 256;
					perRC[y] = min(dist, objPix);
					continue;
				}
			}
			else if (objimg < 255 - THREDHOLD)//not pure white/black -> edge
			{
				dist = objimg - 128, adder = 256;
				//already changed
				continue;
			}
			else//full
			{
				if (curimg < 255 - THREDHOLD)//enter edge
				{
					dist = 128 + curimg, adder = 256;
					perRC[y] = min(dist, objPix);
					continue;
				}
			}
			//current pure and last is the same
			float curdist2 = (float)objPix * (float)objPix;
			float maxdy2 = min(curdist2, sq256LUT[h - y - 1]), dy2 = 65536.0f;
			for (uint dy = 1, cury = y; dy2 < maxdy2; dy2 = sq256LUT[++dy], cury++)
			{
				const uchar oimg = raw[w * (y + dy)];
				if (objimg <= THREDHOLD)//empty
				{
					if (oimg > THREDHOLD)//edge
					{
						const ushort oydist = dy * 256 + 128 - oimg;
						objPix = min(oydist, objPix);
						break;//further won't be shorter
					}
				}
				else//full
				{
					if (oimg <= THREDHOLD)//edge
					{
						const ushort oydist = dy * 256 - 128 + oimg;
						objPix = min(oydist, objPix);
						break;//further won't be shorter
					}
				}
				const float newdist = xdist2[cury] + dy2;
				if (newdist < curdist2)
				{
					curdist2 = newdist;
					objPix = (ushort)sqrt(newdist);
					maxdy2 = min(newdist, maxdy2);
				}
			}
			perRC[y] = objPix;
		}
		global short * const output = result + offset + lid;
		for (uint y = 0, idx = 0; y < h; ++y, idx += w)
			output[idx] = raw[idx] > 127 ? -(short)perRC[y] : (short)perRC[y];
	}
}

#endif