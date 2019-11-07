#pragma once
#include "oglRely.h"
#include "DSAWrapper.h"
#include "StringCharset/Convert.h"
#include "SystemCommon/FileEx.h"
#include "SystemCommon/ThreadEx.h"
#include "MiniLogger/MiniLogger.h"
#include "ImageUtil/ImageUtil.h"
#include "ImageUtil/TexFormat.h"
#include "common/Linq2.hpp"
#include "common/StringEx.hpp"
#include "common/StringLinq.hpp"
#include "common/CopyEx.hpp"
#include "common/ContainerEx.hpp"

#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#include <algorithm>


namespace oglu
{
common::mlog::MiniLogger<false>& oglLog();
}
