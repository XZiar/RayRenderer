float getNoise(int x, int y)
{
	const uint n = mad24(y, 58, x) + mad24(x, 4093, y);
	return mad24(n, mad24(n, n * 15731u, 789221u), 1376312589u) / 4294967296.0f;
	//return (((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 2147483648.0f);
}


kernel void genColorful(global write_only float4 * dat)
{
	const int idx = get_global_id(0),
		idy = get_global_id(1),
		sizex = get_global_size(0),
		sizey = get_global_size(1);
	const int id = idy * sizex + idx;

	float pstep = (idx + idy) * 1.0f / (sizex + sizey);
	float4 tryc = (float4)(idx * 1.0f / sizex, idy * 1.0f / sizey, pstep, 1.0f);

	dat[id] = tryc;
}


float InterCosine(const float x0, const float x1, const float w)
{
	const float f = 0.5f - cospi(w) * 0.5f;
	return mix(x0, x1, f);
}


kernel void genStepNoise(int level, global write_only float4 * dst)
{
	const int idx = get_global_id(0),
		idy = get_global_id(1),
		w = get_global_size(0),
		h = get_global_size(1);
	const int id = mad24(idy, w, idx);
	
	const float stp = pown(0.5f, level);
	const float rx = idx * stp, ry = idy * stp;
	const int x0 = floor(rx), y0 = floor(ry),
		x1 = ceil(rx), y1 = ceil(ry);
	const float w00 = getNoise(x0, y0),
		w10 = getNoise(x1, y0),
		w01 = getNoise(x0, y1),
		w11 = getNoise(x1, y1);
	const float wx = mad(cospi(rx - x0), -0.5f, 0.5f),
		wy = mad(cospi(ry - y0), -0.5f, 0.5f);
	const float w0 = mix(w00, w10, wx),
		w1 = mix(w01, w11, wx);

	const float val = mix(w0, w1, wy);
	dst[id] = (float4)(val, val, val, 1.0f);
}


kernel void genMultiNoise(int level, global write_only float4 * dst)
{
	const int idx = get_global_id(0),
		idy = get_global_id(1),
		w = get_global_size(0),
		h = get_global_size(1);
	const int id = mad24(idy, w, idx);

	float val = 0.0f;
	float stp = 1.0f;
	float amp = 1 / pown(2.0f, level);
	for (int a = level; a-- > 0; amp *= 2, stp *= 0.5f)
	{
		float rx = idx * stp, ry = idy * stp;
		const int x0 = floor(rx), y0 = floor(ry),
			x1 = ceil(rx), y1 = ceil(ry);

		const float w00 = getNoise(x0, y0),
			w10 = getNoise(x1, y0),
			w01 = getNoise(x0, y1),
			w11 = getNoise(x1, y1);
		rx -= x0, ry -= y0;
		const float w0 = InterCosine(w00, w10, rx),
			w1 = InterCosine(w01, w11, rx);
		val += InterCosine(w0, w1, ry) * amp;
	}
	dst[id] = (float4)(val, val, val, 1.0f);
}

kernel void genNoiseBase(global write_only float * src)
{
	const int idx = get_global_id(0),
		idy = get_global_id(1),
		w = get_global_size(0),
		h = get_global_size(1);
	const int id = mad24(idy, w, idx);
	src[id] = getNoise(idx, idy);
}


kernel void genNoiseMulti(int level, global read_only float * src, global write_only float4 * dst)
{
	const int idx = get_global_id(0),
		idy = get_global_id(1),
		w = get_global_size(0),
		h = get_global_size(1);
	const int id = mad24(idy, w, idx);

	float val = 0.0f;
	float stp = 1.0f;
	float amp = 1 / pown(2.0f, level);
	for (int a = level; a-- > 0; amp *= 2, stp *= 0.5f)
	{
		const float rx = idx * stp, ry = idy * stp;
		const int x0 = floor(rx), y0 = floor(ry),
			x1 = ceil(rx), y1 = ceil(ry);
		const int base = mad24(y0, w, x0);
		const float w00 = src[base],
			w10 = src[base + 1],
			w01 = src[base + w],
			w11 = src[base + w + 1];
		const float wx = mad(cospi(rx - x0), -0.5f, 0.5f),
			wy = mad(cospi(ry - y0), -0.5f, 0.5f);
		const float w0 = mix(w00, w10, wx),
			w1 = mix(w01, w11, wx);
		val += mix(w0, w1, wy) * amp;
	}
	dst[id] = (float4)(val, val, val, 1.0f);
}