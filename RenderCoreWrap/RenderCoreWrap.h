// RenderCoreWrap.h
#pragma once

#include "RenderCoreRely.h"
#include "b3d.h"

using namespace System;

namespace RayRender
{

public ref class BasicTest
{
private:
	rayr::BasicTest *core;
	Basic3D::Camera ^cam_;
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

	void draw();
	void resize(const int w, const int h);
	void rfsData();
	void moveobj(const float x, const float y, const float z);
	void rotateobj(const float x, const float y, const float z);
};

}
