// RenderCoreWrap.h
#pragma once

#pragma unmanaged
#include "../RenderCore/RenderCore.h"
#pragma managed

using namespace System;

namespace RayRender
{

public ref class BasicTest
{
private:
	rayr::BasicTest *core;
public:
	BasicTest();
	~BasicTest() { this->!BasicTest(); }
	!BasicTest();

	property bool mode
	{
		bool get();
		void set(bool value);
	}

	void draw();
	void resize(const int w, const int h);
};

}
