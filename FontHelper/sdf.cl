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
#define XD_Stride 129
#if LOC_MEM_SIZE < 50000//small local memory,save raw cache to private
kernel void graysdf(global const float * const restrict sq256LUT2, global const Info * const restrict info, global const uchar * const restrict img, global short * const restrict result)
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

kernel void graysdf(global const float * const restrict sq256LUT2, global const Info * const restrict info, global const uchar * const restrict img, global short * const restrict result)
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

/*
 * old one for intel, make private array perRC in local(pre-transposed) and xdist^2 pre-calculated

kernel void graysdf(global float * restrict sq256LUT2, global const Info * restrict info, global const uchar * restrict img, global short * restrict result)
{
	private const uint gid = get_group_id(0);
	private const uint lid = get_local_id(0);
	private const uint w = info[gid].w, h = info[gid].h, offset = info[gid].offset;
	//local float sq256LUT[MAX_PIX];
	local ushort tmp[MAX_PIX * XD_Stride];
	local uchar imgcache[MAX_PIX * XD_Stride];
	local uchar * const raw = imgcache + lid * XD_Stride;
	//each row operation
	if (lid < h)
	{
		local ushort * const rPerRC = tmp + lid;
		int dist = 64 * 256, adder = 0;
		{
			global const uchar * const rowptr = &img[offset + w * lid];
			for (uint x = 0, xlim = w / 4; x < xlim; ++x)
				vstore4(vload4(x, rowptr), x, raw);
		}
		uint idx = 0;
		uchar lastimg = 0;
		bool lastThresNL = false;
		for (uint x = 0; x < w; ++x, dist += adder, idx += XD_Stride)
		{
			const uchar objimg = raw[x];
			const bool objThresNL = objimg > THREDHOLD;
			if (lastThresNL == objThresNL)
				rPerRC[idx] = dist;
			else
			{
				adder = 256;
				const int dEnter = objimg - 128;
				const int dLeave = (256 + 128) - lastimg;
				const bool isEnter = lastimg < objimg;
				dist = isEnter ? dEnter : dLeave;
				rPerRC[idx] = abs(dist);
			}
			lastimg = objimg; lastThresNL = objThresNL;
		}
		idx -= XD_Stride;
		dist = rPerRC[idx], adder = 0;
		lastimg = 0, lastThresNL = false;
		for (uint x = w; x--; dist += adder, idx -= XD_Stride)
		{
			const int objPix = rPerRC[idx];
			const uchar objimg = raw[x];
			const bool objThresNL = objimg > THREDHOLD;
			if (lastThresNL == objThresNL)
				rPerRC[idx] = min(dist, objPix);
			else
			{
				adder = 256;
				const int dEnter = objimg - 128;
				const int dLeave = (256 + 128) - lastimg;
				const bool isEnter = lastimg < objimg;
				dist = isEnter ? dEnter : dLeave;
				rPerRC[idx] = min((int)abs(dist), objPix);
			}
			lastimg = objimg; lastThresNL = objThresNL;
		}
	}
	//event_t evt = async_work_group_copy(sq256LUT, sq256LUT2, MAX_PIX, 0);
	//wait_group_events(1, &evt);
	//synchronize
	barrier(CLK_LOCAL_MEM_FENCE);
	if (lid < w)
	{
		local ushort * const perRC = &tmp[lid * XD_Stride];//has been prepared.
		private float xdist2[MAX_PIX];
		uint idx = lid;
		for (uint y = 0, imgidx = lid + offset; y < h; ++y, imgidx += w)
			raw[y] = img[imgidx];
		for (uint i = 0, ilim = h / 4; i < ilim; ++i)
		{
			const float4 di = convert_float4(vload4(i, perRC));
			vstore4(di*di, i, xdist2);
		}
		uchar lastimg = raw[0];
		bool lastThresNL = false, lastThresNH = true;
		bool quickEnd = false;
		int dist = 64 * 256, adder = 0;
		float yf = 512.0f;
		for (uint y = 1; y < h; y++, dist += adder)
		{
			const int curxdist = perRC[y];
			const uchar objimg = raw[y];
			const bool objThresNL = objimg > THREDHOLD, objThresNH = objimg < 255 - THREDHOLD;
			if (!objThresNL)//empty
			{
				if (lastThresNL)//leave edge
				{
					dist = 256 + 128 - lastimg;
					perRC[y] = min(curxdist, dist);
					quickEnd = true;
				}
			}
			else if (objThresNH)//not pure white/black -> edge
			{
				dist = objimg - 128;
				perRC[y] = abs(dist);
				quickEnd = true;
			}
			else//full, >255-THRES
			{
				if (lastThresNH)//enter edge
				{
					dist = 128 + lastimg;
					perRC[y] = min(curxdist, dist);
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
				float objPix = min(curxdist, dist);
				float curdist2 = objPix * objPix;
				float maxdy2 = min(curdist2, yf * yf), dy2 = 65536.0f, dy256 = 256.0f;
				for (uint dy = 1, testy = y; dy2 < maxdy2; dy2 = dy256 * dy256, testy--)
				{
					const uchar oimg = raw[y - dy];
					const bool oimgThresNL = oimg > THREDHOLD;
					if (objThresNL != oimgThresNL)//edge
					{
						const float negDistOImg = oimgThresNL ? oimg - 128 : 128 - oimg;
						const float oydist = dy256 - negDistOImg;
						objPix = min(oydist, objPix);//objPix has been prove <= curxdist
						break;//further won't be shorter
					}
					const float newdist = xdist2[testy] + dy2;
					if (newdist < curdist2)
					{
						curdist2 = newdist;
						objPix = sqrt(newdist);
						maxdy2 = min(newdist, maxdy2);
					}
					dy256 += 256.0f; dy++;
				}
				perRC[y] = (ushort)objPix;
			}
			lastimg = objimg; lastThresNL = objThresNL; lastThresNH = objThresNH;
			yf += 256.0f;
		}
		dist = 64 * 256, adder = 0;
		//lastimg, lastThres are all prepared
		quickEnd = false;
		global short * restrict output = result + offset + w * (h - 1) + lid;
		yf = 256.0f;
		for (uint y = h - 1; y--; output -= w, dist += adder)
		{
			const uchar objimg = raw[y];
			const bool objThresNL = objimg > THREDHOLD, objThresNH = objimg < 255 - THREDHOLD;
			const int objPix = perRC[y];// no need to check xdist[idx], it is checked in previous pass
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
				for (uint dy = 1, testy = y; dy2 < maxdy2; dy2 = dy256 * dy256, testy++)
				{
					const uchar oimg = raw[y + dy];
					const bool oimgThresNL = oimg > THREDHOLD;
					if (objThresNL != oimgThresNL)//edge
					{
						const float negDistOImg = oimgThresNL ? oimg - 128 : 128 - oimg;
						const float oydist = dy256 - negDistOImg;
						objPixF = min(oydist, objPixF);
						break;//further won't be shorter
					}
					const float newdist = xdist2[testy] + dy2;
					if (newdist < curdist2)
					{
						curdist2 = newdist;
						objPixF = sqrt(newdist);
						maxdy2 = min(newdist, maxdy2);
					}
					dy256 += 256.0f; dy++;
				}
				*output = objimg > 127 ? -(short)objPixF : (short)objPixF;
			}
			lastimg = objimg; lastThresNL = objThresNL; lastThresNH = objThresNH;
			yf += 256.0f;
		}
	}
}
*/