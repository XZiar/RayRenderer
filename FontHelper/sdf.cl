typedef struct Info
{
	int offset;
	uchar w;
	uchar h;
}Info;

kernel void bmpsdf(global const Info* restrict info, global read_only uchar* restrict img, global ushort* restrict result)
{
	private const int gid = get_group_id(0);
	private const int lid = get_local_id(0);
	local ushort sqLUT[160];
	local uchar xdist[160 * 160];
	const int w = info[gid].w, h = info[gid].h, offset = info[gid].offset;
	//setup square-LUT
	sqLUT[lid] = lid*lid;
	//each row operation
	if (lid < h)
	{
		int idx = lid * w;
		private uchar rowRaw[160];
		uchar dist = 64, adder = 0, curimg = 0;
		for (int x = 0; x < w; ++x, ++idx, dist += adder)
		{
			uchar objimg = rowRaw[x] = img[idx + offset];
			if (objimg != curimg)
				xdist[idx] = dist = 0, adder = 1, curimg = objimg;
			else
				xdist[idx] = dist;
		}
		dist = xdist[--idx], adder = 0, curimg = 0;
		for (int x = w; x--; --idx, dist += adder)
		{
			uchar objimg = rowRaw[x];
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
		int idx = lid;
		private uchar colRaw[160];
		private ushort perCol[160];
		ushort obj = 32768;
		for (int y = 0, tmpidx = lid + offset; y < h; ++y, tmpidx += w)
			colRaw[y] = img[tmpidx];
		perCol[0] = sqLUT[xdist[idx]];
		for (int y = 1; y < h; perCol[y++] = obj)
		{
			int testidx = idx;
			idx += w;
			uchar curimg = colRaw[y], curxdist = xdist[idx];
			if (curxdist == 0)// 0 dist = edge
			{
				obj = 0; continue;
			}
			obj = sqLUT[curxdist];
			ushort maxdy2 = min(obj, sqLUT[y + 1]);
			for (ushort dy = 1, dy2 = 1; dy2 < maxdy2; dy2 = sqLUT[++dy], testidx -= w)
			{
				uchar oimg = colRaw[y - dy], oxdist = xdist[testidx];
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
		for (int y = h - 1; y--; perCol[y] = obj)
		{
			int testidx = idx;
			idx -= w;
			uchar curimg = colRaw[y], curxdist = xdist[idx];
			if (curxdist == 0)// 0 dist = edge
				continue;
			obj = perCol[y];
			ushort maxdy2 = min(obj, sqLUT[h - y - 1]);
			for (ushort dy = 1, dy2 = 1; dy2 < maxdy2; dy2 = sqLUT[++dy], testidx += w)
			{
				uchar oimg = colRaw[y + dy], oxdist = xdist[testidx];
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
		for (int y = 0; y < h; ++y, idx += w)
			//result[idx] = perCol[y];
			result[idx] = sqLUT[xdist[idx - offset]];
	}
}


kernel void greysdf(global const Info *info, global read_only uchar *img, global ushort *result)
{
	private const int gid = get_group_id(0);
	private const int lid = get_local_id(0);
	local ushort sqLUT[160];
	local ushort xdist[160 * 160];
	const int w = info[gid].w, h = info[gid].h, offset = info[gid].offset;
	//setup square-LUT
	sqLUT[lid] = lid*lid;
	//each row operation
	if (lid < h)
	{
		int idx = lid * w;
		private uchar rowRaw[160];
		ushort dist = 64 * 256, adder = 0;
		uchar curimg = 0;
		for (int x = 0; x < w; ++x, ++idx, dist += adder)
		{
			uchar objimg = rowRaw[x] = img[idx + offset];
			if (curimg == 0)
			{
				if (objimg != 0)//enter edge
				{
					dist = objimg - 128, adder = 256;
					xdist[idx] = abs(128 - objimg);
				}
				else
					xdist[idx] = dist;
			}
			else
			{
				if (objimg == 0)//leave edge
					xdist[idx] = dist = (256 + 128) - curimg, adder = 256;
				else
					xdist[idx] = dist;
			}
			curimg = objimg;
		}
		dist = xdist[--idx], adder = 0, curimg = 0;
		for (int x = w; x--; --idx, dist += adder)
		{
			uchar objimg = rowRaw[x];
			if (curimg == 0)
			{
				if (objimg != 0)//enter edge
				{
					dist = objimg - 128, adder = 256;
					xdist[idx] = min((ushort)abs(128 - objimg), xdist[idx]);
				}
				else
					xdist[idx] = min((ushort)dist, xdist[idx]);
			}
			else
			{
				if (objimg == 0)//leave edge
				{
					dist = (256 + 128) - curimg, adder = 256;
					xdist[idx] = min((ushort)dist, xdist[idx]);
				}
				else
					xdist[idx] = min((ushort)dist, xdist[idx]);
			}
			curimg = objimg;
		}
	}
	//synchronize
	barrier(CLK_LOCAL_MEM_FENCE);
	if (lid < w)
	{
		int idx = lid;
		for (int y = 0; y < h; ++y, idx += w)
			result[idx + offset] = xdist[idx];
		return;
		private uchar colRaw[160];
		private ushort perCol[160];
		ushort obj = 32768;
		for (int y = 0, tmpidx = lid + offset; y < h; ++y, tmpidx += w)
			colRaw[y] = img[tmpidx];
		perCol[0] = sqLUT[xdist[idx]];
		for (int y = 1; y < h; perCol[y++] = obj)
		{
			int testidx = idx;
			idx += w;
			uchar curimg = colRaw[y], curxdist = xdist[idx];
			if (curxdist == 0)// 0 dist = edge
			{
				obj = 0; continue;
			}
			obj = sqLUT[curxdist];
			ushort maxdy2 = min(obj, sqLUT[y + 1]);
			for (ushort dy = 1, dy2 = 1; dy2 < maxdy2; dy2 = sqLUT[++dy], testidx -= w)
			{
				uchar oimg = colRaw[y - dy], oxdist = xdist[testidx];
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
		for (int y = h - 1; y--; perCol[y] = obj)
		{
			int testidx = idx;
			idx -= w;
			uchar curimg = colRaw[y], curxdist = xdist[idx];
			if (curxdist == 0)// 0 dist = edge
				continue;
			obj = perCol[y];
			ushort maxdy2 = min(obj, sqLUT[h - y - 1]);
			for (ushort dy = 1, dy2 = 1; dy2 < maxdy2; dy2 = sqLUT[++dy], testidx += w)
			{
				uchar oimg = colRaw[y + dy], oxdist = xdist[testidx];
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
		for (int y = 0; y < h; ++y, idx += w)
			result[idx] = perCol[y];
		//result[idx] = sqLUT[xdist[idx - offset]];
	}
}
