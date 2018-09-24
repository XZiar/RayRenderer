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
    Vec3F(const miniBLAS::Vec3& vec) : x(vec.x), y(vec.y), z(vec.z) { }
    Vec3F(const miniBLAS::Vec4& vec) : x(vec.x), y(vec.y), z(vec.z) { }
    void Load(const miniBLAS::Vec3& vec) { x = vec.x; y = vec.y; z = vec.z; }
    void Load(const miniBLAS::Vec4& vec) { x = vec.x; y = vec.y; z = vec.z; }
    void Store(miniBLAS::Vec3& vec) { vec.x = x; vec.y = y; vec.z = z; }
    void Store(miniBLAS::Vec4& vec) { vec.x = x; vec.y = y; vec.z = z; }
};

public value struct Vec4F
{
    float x, y, z, w;
    Vec4F(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) { }
    virtual String^ ToString() override
    {
        return String::Format("{0:F3},{1:F3},{2:F3},{3:F3}", x, y, z, w);
    }
internal:
    Vec4F(const miniBLAS::Vec4& vec) : x(vec.x), y(vec.y), z(vec.z), w(vec.w) { }
    void Load(const miniBLAS::Vec4& vec) { x = vec.x; y = vec.y; z = vec.z; w = vec.w; }
    void Store(miniBLAS::Vec4& vec) { vec.x = x; vec.y = y; vec.z = z; vec.w = w; }
};

public ref class Camera : BaseViewModel
{
internal:
    rayr::Camera *cam;
    bool isRef = false;
internal:
    Camera(rayr::Camera *obj) :cam(obj), isRef(true) { }
public:
    ~Camera() { this->!Camera(); }
    !Camera()
    {
        if (!isRef)
            delete cam;
    }

    property int Width
    {
        int get() { return cam->Width; }
        void set(int value) { cam->Width = value; }
    }
    property int Height
    {
        int get() { return cam->Height; }
        void set(int value) { cam->Height = value; }
    }
    property Vec3F Position
    {
        Vec3F get() { return Vec3F(cam->Position); }
        void set(Vec3F value) { value.Store(cam->Position); OnPropertyChanged("Position"); }
    }
    property Vec3F Direction
    {
        Vec3F get() { return Vec3F(cam->Rotation); }
        void set(Vec3F value) { value.Store(cam->Rotation); OnPropertyChanged("Direction"); }
    }

    void Move(const float dx, const float dy, const float dz)
    {
        cam->Move(dx, dy, dz);
        OnPropertyChanged("Position");
    }
    //rotate along x-axis, radius
    void Pitch(const float radx)
    {
        cam->Pitch(radx);
        OnPropertyChanged("Direction");
    }
    //rotate along y-axis, radius
    void Yaw(const float rady)
    {
        cam->Yaw(rady);
        OnPropertyChanged("Direction");
    }
    //rotate along z-axis, radius
    void Roll(const float radz)
    {
        cam->Roll(radz);
        OnPropertyChanged("Direction");
    }
    void Rotate(const float dx, const float dy, const float dz)
    {
        cam->Rotate(dx, dy, dz);
        OnPropertyChanged("Direction");
    }
};

}