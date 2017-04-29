typedef struct Info
{
	int offset;
	uchar w;
	uchar h;
}Info;

kernel void bmpsdf(global const Info *info, global read_only uchar *img, global ushort *result)
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
		uchar dist = 64, adder = 0;
		for (int x = 0; x < w; ++x, ++idx, dist += adder)
		{
			if (img[idx + offset])
				xdist[idx] = dist = 0, adder = 1;
			else
				xdist[idx] = dist;
		}
		dist = xdist[--idx], adder = 0;
		for (int x = w; x--; --idx, dist += adder)
		{
			if (!xdist[idx])
				dist = 0, adder = 1;
			else
				xdist[idx] = min(xdist[idx], dist);
		}
	}
	//synchronize
	barrier(CLK_LOCAL_MEM_FENCE);
	if (lid < w)
	{
		int idx = lid;
		private ushort perCol[160];
		ushort obj = 32768;
		perCol[0] = sqLUT[xdist[idx]];
		for (int y = 1; y < h; perCol[y++] = obj)
		{
			int testidx = idx;
			idx += w;
			int curxdist = xdist[idx];
			if (curxdist == 0)// 0 dist = inside
			{
				obj = 0; continue;
			}
			obj = sqLUT[curxdist];
			ushort maxdy = min(obj, sqLUT[y + 1]);
			for (ushort dy = 1, dy2 = 1; dy2 < maxdy; dy2 = sqLUT[++dy], testidx -= w)
			{
				uchar oxdist = xdist[testidx];
				if (oxdist == 0)
				{
					//dy^2 < obj
					obj = dy2;
					break;//further won't be shorter
				}
				ushort newdist = sqLUT[oxdist] + dy2;
				if (newdist < obj)
				{
					obj = newdist;
					maxdy = min(newdist, maxdy);
				}
			}
		}
		for (int y = h - 1; y--; perCol[y] = obj)
		{
			int testidx = idx;
			idx -= w;
			obj = perCol[y];
			int curxdist = xdist[idx];
			if (curxdist == 0)// 0 dist = inside
				continue;
			ushort maxdy = min(obj, sqLUT[h - y - 1]);
			for (ushort dy = 1, dy2 = 1; dy2 < maxdy; dy2 = sqLUT[++dy], testidx += w)
			{
				uchar oxdist = xdist[testidx];
				if (oxdist == 0)
				{
					//dy^2 < obj
					obj = dy2;
					break;//further won't be shorter
				}
				ushort newdist = sqLUT[oxdist] + dy2;
				if (newdist < obj)
				{
					obj = newdist;
					maxdy = min(newdist, maxdy);
				}
			}
		}
		idx = offset + lid;
		for (int y = 0; y < h; ++y, idx += w)
			result[idx] = perCol[y];
			//result[idx] = sqLUT[xdist[idx - offset]];
	}
}