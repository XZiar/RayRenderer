// RenderCoreWrap.h
#pragma once

#include "RenderCoreRely.h"
#include "b3d.h"
#include "logger.h"

using namespace System;

namespace RayRender
{

public ref class BasicTest
{
private:
	rayr::BasicTest *core;
	Basic3D::Camera ^cam_;
public:
	static BasicTest()
	{
		common::mlog::logger::setGlobalCallBack(onLog);
	}
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

	void draw();
	void resize(const int w, const int h);
	void rfsData();
	void moveobj(const uint16_t id, const float x, const float y, const float z);
	void rotateobj(const uint16_t id, const float x, const float y, const float z);
	uint16_t objectCount();
};

}
