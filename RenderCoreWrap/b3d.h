#pragma once

#include "RenderCoreRely.h"

using namespace System;

namespace Basic3D
{

public ref class Camera
{
private:
	b3d::Camera *cam;
	bool isRef = false;
internal:
	Camera(b3d::Camera * obj) :cam(obj), isRef(true) { }
public:
	property int Width
	{
		int get() { return cam->width; }
		void set(int value) { cam->width = value; }
	}
	property int Height
	{
		int get() { return cam->height; }
		void set(int value) { cam->height = value; }
	}
	Camera();
	~Camera() { this->!Camera(); };
	!Camera();

	void move(const float dx, const float dy, const float dz);
	//rotate along x-axis
	void pitch(const float angx);
	//rotate along y-axis
	void yaw(const float angy);
	//rotate along z-axis
	void roll(const float angz);
};

}