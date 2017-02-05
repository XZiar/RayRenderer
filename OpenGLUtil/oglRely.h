#pragma once

#define GLEW_STATIC
#include <GL/glew.h>
#pragma comment (lib, "glu32.lib")    /* link OpenGL Utility lib     */
#pragma comment (lib, "opengl32.lib") /* link Microsoft OpenGL lib   */
#pragma comment (lib, "gdi32.lib")    /* link Windows GDI lib        */
#pragma comment (lib, "user32.lib")   /* link Windows user lib       */
#ifndef _DEBUG
#    pragma comment(lib, "glew32s.lib")
#else
#    pragma comment(lib, "glew32sd.lib")
#endif
