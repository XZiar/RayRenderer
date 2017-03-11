#include "RenderCoreWrap.h"

namespace RayRender
{

BasicTest::BasicTest()
{
	core = new rayr::BasicTest();
	cam_ = gcnew Basic3D::Camera(&core->cam);
}

BasicTest::!BasicTest()
{
	delete core;
}

void BasicTest::draw()
{
	core->draw();
}

void BasicTest::resize(const int w, const int h)
{
	core->resize(w, h);
}


void BasicTest::rfsData()
{
	core->rfsData();
}

void BasicTest::moveobj(const uint16_t id, const float x, const float y, const float z)
{
	core->moveobj(id, x, y, z);
}

void BasicTest::rotateobj(const uint16_t id, const float x, const float y, const float z)
{
	core->rotateobj(id, x, y, z);
}

uint16_t BasicTest::objectCount()
{
	return core->objectCount();
}

}