#include "RenderCoreWrap.h"

namespace RayRender
{

BasicTest::BasicTest()
{
	core = new rayr::BasicTest();
}

BasicTest::!BasicTest()
{
	delete core;
}

bool BasicTest::mode::get()
{
	return core->mode;
}

void BasicTest::mode::set(bool value)
{
	core->mode = value;
}

void BasicTest::draw()
{
	core->draw();
}

void BasicTest::resize(const int w, const int h)
{
	core->resize(w, h);
}


}