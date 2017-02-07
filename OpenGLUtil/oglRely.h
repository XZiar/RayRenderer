#pragma once

#ifdef OGLU_EXPORT
#   define OGLUAPI _declspec(dllexport)
#   define OGLUTPL _declspec(dllexport) 
#   define COMMON_EXPORT
#else
#   define OGLUAPI _declspec(dllimport)
#   define OGLUTPL
#endif

#include "../3dBasic/3dElement.hpp"
#include "../3DBasic/CommonBase.hpp"

#include <cstdio>
#include <string>

namespace oclu
{
class _oclMem;
}


namespace oglu
{

using std::string;
using namespace common;

template<class T = char>
class OGLUTPL OPResult
{
private:
	bool isSuc;
public:
	string msg;
	T data;
	OPResult(const bool isSuc_, const string& msg_ = "") :isSuc(isSuc_), msg(msg_) { }
	OPResult(const bool isSuc_, const T& dat_, const string& msg_ = "") :isSuc(isSuc_), msg(msg_), data(dat_) { }
	operator bool() { return isSuc; }
};

}