#include "b3d.h"


namespace Basic3D
{

Camera::Camera() :isRef(false)
{
	cam = new b3d::Camera();
}

Camera::!Camera()
{
	if (!isRef)
		delete cam;
}

void Camera::Move(const float dx, const float dy, const float dz)
{
	cam->move(dx, dy, dz);
}

void Camera::Pitch(const float angx)
{
	cam->pitch(angx);
}

void Camera::Yaw(const float angy)
{
	cam->yaw(angy);
}

void Camera::Roll(const float angz)
{
	cam->roll(angz);
}


Light::!Light()
{
	if (!isRef)
		delete light;
}

}