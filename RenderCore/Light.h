#pragma once

#include "RenderCoreRely.h"

namespace b3d
{


enum class LightType : int32_t { Parallel = 0, Point = 1, Spot = 2 };

struct alignas(Vec4) LightData : public common::AlignBase<LightData>
{
	Vec3 position = Vec3::zero();
	Vec3 direction = Vec3::zero();
	Vec4 color = Vec4::one();
	Vec4 attenuation = Vec4::one();
	float coang, exponent;
	const LightType type;
	bool isOn = true;
protected:
	LightData(const LightType type_) : type(type_) {}
};

class alignas(LightData) Light : public LightData
{
protected:
	Light(const LightType type_, const std::wstring& name_);
public:
	std::wstring name;
};

class alignas(16) ParallelLight : public Light
{
public:
	ParallelLight() : Light(LightType::Parallel, L"ParallelLight") {}
};

class alignas(16) PointLight : public Light
{
public:
	PointLight() : Light(LightType::Point, L"PointLight") {}
};

class alignas(16) SpotLight : public Light
{
public:
	SpotLight() : Light(LightType::Spot, L"SpotLight") {}
};

}
