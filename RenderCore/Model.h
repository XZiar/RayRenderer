#pragma once

#include "RenderElement.h"

namespace rayr
{

class alignas(16) Model : public Drawable
{
protected:
	class OBJLoder
	{
		wstring filename;
		FILE *fp;
		char tmpdat[8][256];
	public:
		OBJLoder(const wstring &fname);
		~OBJLoder();
		int8_t read(string data[]);
		int8_t parseFloat(const string &in, float out[]);
		int8_t parseInt(const string &in, int out[]);
	};
public:

};

}