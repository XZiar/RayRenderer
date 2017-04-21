// RenderCoreWrap.h
#pragma once

#include "RenderCoreWrapRely.h"
#include "b3d.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading::Tasks;

namespace RayRender
{

ref class BasicTest;

public ref class LightHolder
{
private:
	const vector<common::Wrapper<b3d::Light>>& src;
	List<Basic3D::Light^>^ lights;
	rayr::BasicTest * const core;
internal:
	LightHolder(rayr::BasicTest *core_, const vector<common::Wrapper<b3d::Light>>& lights_) : core(core_), src(lights_) {}
public:
	property Basic3D::Light^ default[int32_t]
	{
		Basic3D::Light^ get(int32_t i)
		{
			if(i < 0 || (uint32_t)i >= src.size())
				throw gcnew System::IndexOutOfRangeException();
			return gcnew Basic3D::Light(&src[i]);
		}
	};
	property int32_t Size
	{
		int32_t get() { return (int32_t)src.size(); }
	}
	void Add(Basic3D::LightType type);
	void Clear();
};

public ref class BasicTest
{
private:
	rayr::BasicTest *core;
	Basic3D::Camera ^cam_;
internal:
public:
	BasicTest();
	~BasicTest() { this->!BasicTest(); }
	!BasicTest();

	property bool mode
	{
		bool get() { return core->mode; }
		void set(bool value) { core->mode = value; }
	}

	property Basic3D::Camera^ cam
	{
	public:
		Basic3D::Camera^ get() { return cam_; }
	}

	property uint16_t objectCount
	{
		uint16_t get() { return core->objectCount(); }
	}

	initonly LightHolder^ Lights;

	void Draw();
	void Resize(const int w, const int h);
	void Moveobj(const uint16_t id, const float x, const float y, const float z);
	void Rotateobj(const uint16_t id, const float x, const float y, const float z);

	Task<Func<bool>^>^ AddModelAsync(String^ name);

	void TryException(int type);
};

}
