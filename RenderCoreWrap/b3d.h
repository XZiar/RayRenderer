#pragma once

#include "RenderCoreWrapRely.h"

using namespace System;

namespace Basic3D
{

using namespace common;

public ref class Camera
{
private:
	b3d::Camera *cam;
	bool isRef = false;
internal:
	Camera(b3d::Camera *obj) :cam(obj), isRef(true) { }
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
	~Camera() { this->!Camera(); }
	!Camera();

	void Move(const float dx, const float dy, const float dz);
	//rotate along x-axis
	void Pitch(const float angx);
	//rotate along y-axis
	void Yaw(const float angy);
	//rotate along z-axis
	void Roll(const float angz);
};

public enum class LightType : int32_t { Parallel, Point, Spot };

public ref class Light
{
private:
	Wrapper<b3d::Light> *light;
	bool isRef = false;
internal:
	Light(const Wrapper<b3d::Light> *obj) : light(new Wrapper<b3d::Light>(*obj)), isRef(true) { }
public:
	~Light() { this->!Light(); }
	!Light();
	String^ name() { return gcnew String((*light)->name.c_str()); }
};

}