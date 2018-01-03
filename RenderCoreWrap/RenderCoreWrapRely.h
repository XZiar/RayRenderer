#pragma once

#pragma unmanaged
#define BAK_M_CEE _M_CEE
#undef _M_CEE

#include "../RenderCore/RenderCore.h"
#include "../common/Exceptions.hpp"
#include "../common/miniLogger/miniLogger.h"

using namespace common;
using std::vector;

#pragma managed
#define _M_CEE BAK_M_CEE


#include <msclr/marshal_cppstd.h>
