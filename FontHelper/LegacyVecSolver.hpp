#pragma once

#include "FontRely.h"


namespace oglu
{

	int solveCubic(float a, float b, float c, float* r)
	{
		float p = b - a * a / 3;
		float q = a * (2 * a*a - 9 * b) / 27 + c;
		float p3 = p * p*p;
		float d = q * q + 4 * p3 / 27;
		float offset = -a / 3;
		if (d >= 0)
		{ // Single solution
			float z = sqrtf(d);
			float u = (-q + z) / 2;
			float v = (-q - z) / 2;
			u = cbrt(u);
			v = cbrt(v);
			r[0] = offset + u + v;
			return 1;
		}
		float u = sqrtf(-p / 3);
		float v = acos(-sqrtf(-27 / p3) * q / 2) / 3;
		float m = cos(v), n = sin(v)*1.732050808f;
		r[0] = offset + u * (m + m);
		r[1] = offset - u * (n + m);
		r[2] = offset + u * (n - m);
		return 3;
	}


	inline float dot(float x1, float y1, float x2, float y2)
	{
		return x1 * x2 + y1 * y2;
	}

	float SignedDistanceSquared(float x, float y, ft::FreeTyper::QBLine s, float minDis = 1e20f)
	{
		float res[3];

		{
			float d0x = x - s.p0x, d0y = y - s.p0y;
			float d2x = x - s.p2x, d2y = y - s.p2y;
			float d0 = d0x * d0x + d0y * d0y;
			float d2 = d2x * d2x + d2y * d2y;
			minDis = min(minDis, min(d0, d2));
		}

		float Mx = s.p0x - x, My = s.p0y = y;
		float Ax = s.p1x - s.p0x, Ay = s.p1y - s.p0y;
		float Bx = s.p2x - s.p1x - Ax, By = s.p2y - s.p1y - Ay;

		float a = 1.0f / (Bx*Bx + By * By);// 1/dot(B,B);
		float b = 3 * (Ax * Bx + Ay * By);// 3dot(A,B);
		float c = 2 * (Ax * Ax + Ay * Ay) + Mx * Bx + My * By;// 2dot(A,A)+dot(M,B)
		float d = Mx * Ax + My * Ay;// dot(M,A)

		int n = solveCubic(b*a, c*a, d*a, res);

		for (int j = 0; j < n; j++)
		{
			float t = res[j];
			if (t >= 0 && t <= 1)
			{
				float dx = (1 - t)*(1 - t)*s.p0x + 2 * t*(1 - t)*s.p1x + t * t*s.p2x - x;
				float dy = (1 - t)*(1 - t)*s.p0y + 2 * t*(1 - t)*s.p1y + t * t*s.p2y - y;
				minDis = min(minDis, dx*dx + dy * dy);
			}
		}

		return minDis;
	}

	float LineDistanceSquared(float x, float y, ft::FreeTyper::SLine l)
	{
		float cross = (l.p1x - l.p0x) * (x - l.p0x) + (l.p1y - l.p0y) * (y - l.p0y);
		if (cross <= 0)
			return (x - l.p0x) * (x - l.p0x) + (y - l.p0y) * (y - l.p0y);

		float d2 = (l.p1x - l.p0x) * (l.p1x - l.p0x) + (l.p1y - l.p0y) * (l.p1y - l.p0y);
		if (cross >= d2)
			return (x - l.p1x) * (x - l.p1x) + (y - l.p1y) * (y - l.p1y);

		float r = cross / d2;
		float px = l.p0x + (l.p1x - l.p0x) * r;
		float py = l.p0y + (l.p1y - l.p0y) * r;
		return (x - px) * (x - px) + (py - l.p0y) * (py - l.p0y);
	}

	void stroke(ft::FreeTyper& ft2, oglTexture& testTex)
	{
		auto ret = ft2.TryStroke();
		auto w = ret.second.first, h = ret.second.second;
		w = ((w + 3) / 4) * 4;
		vector<uint8_t> data(w*h);
		auto qlines = ret.first.first;
		auto slines = ret.first.second;
		//qlines = { ft::FreeTyper::QBLine{0.f,0.f,(float)w,0.f,0.5f*w,.5f*h} };
		//slines.clear();
		qlines.clear();
		slines = { ft::FreeTyper::SLine{ 0.f,0.f,(float)w,(float)h } };
		for (uint32_t a = 0; a < h; a++)
			for (uint32_t b = 0; b < w; b++)
			{
				float minDist = 1e20f;
				for (auto& qline : qlines)
				{
					minDist = SignedDistanceSquared(b, a, qline, minDist);
				}
				for (auto& sline : slines)
				{
					auto newDist = LineDistanceSquared(b, a, sline);
					minDist = min(minDist, newDist);
				}
				//data[a*w + b] = minDist < 1.0f ? 0 : 255;
				auto dist = sqrt(minDist);
				//data[a*w + b] = dist < 1.6f ? 0 : 255;
				data[a*w + b] = (uint8_t)std::clamp(dist * 4, 0.f, 255.f);
			}
		testTex->setData(TextureInnerFormat::R8, TextureDataFormat::R8, w, h, data);
	}


}