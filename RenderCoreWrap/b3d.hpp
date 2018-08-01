#pragma once

#include "RenderCoreWrapRely.h"

using namespace System;

namespace Basic3D
{
using namespace common;

public value struct Vec3F
{
    float x, y, z;
    Vec3F(float x_, float y_, float z_) : x(x_), y(y_), z(z_) { }
    virtual String^ ToString() override
    {
        return String::Format("{0:F3},{1:F3},{2:F3}", x, y, z);
    }
internal:
    Vec3F(const b3d::Vec3& vec) : x(vec.x), y(vec.y), z(vec.z) { }
    Vec3F(const b3d::Vec4& vec) : x(vec.x), y(vec.y), z(vec.z) { }
    void Store(b3d::Vec3& vec) { vec.x = x; vec.y; vec.z = z; }
    void Store(b3d::Vec4& vec) { vec.x = x; vec.y; vec.z = z; }
};

public ref class Camera : BaseViewModel
{
internal:
    b3d::Camera *cam;
    bool isRef = false;
internal:
    Camera(b3d::Camera *obj) :cam(obj), isRef(true) { }
public:
    ~Camera() { this->!Camera(); }
    !Camera()
    {
        if (!isRef)
            delete cam;
    }

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
    property Vec3F Position
    {
        Vec3F get() { return Vec3F(cam->position); }
        void set(Vec3F value) { value.Store(cam->position); OnPropertyChanged("Position"); }
    }
    property Vec3F Direction
    {
        Vec3F get() { return Vec3F(cam->rotation); }
        void set(Vec3F value) { value.Store(cam->rotation); OnPropertyChanged("Direction"); }
    }

    void Move(const float dx, const float dy, const float dz)
    {
        cam->Move(dx, dy, dz);
        OnPropertyChanged("Position");
    }
    //rotate along x-axis, radius
    void Pitch(const float radx)
    {
        cam->pitch(radx);
        OnPropertyChanged("Direction");
    }
    //rotate along y-axis, radius
    void Yaw(const float rady)
    {
        cam->yaw(rady);
        OnPropertyChanged("Direction");
    }
    //rotate along z-axis, radius
    void Roll(const float radz)
    {
        cam->roll(radz);
        OnPropertyChanged("Direction");
    }
    void Rotate(const float dx, const float dy, const float dz)
    {
        cam->Rotate(dx, dy, dz);
        OnPropertyChanged("Direction");
    }
};

}