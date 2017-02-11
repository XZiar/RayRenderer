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

void Camera::move(const float dx, const float dy, const float dz)
{
	cam->move(dx, dy, dz);
}

void Camera::pitch(const float angx)
{
	cam->pitch(angx);
}

void Camera::yaw(const float angy)
{
	cam->yaw(angy);
}

void Camera::roll(const float angz)
{
	cam->roll(angz);
}

}