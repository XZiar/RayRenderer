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
		FILE *fp = nullptr;
	public:
		char curline[256];
		const char* param[5];
		struct TextLine
		{
			uint64_t type;
			uint8_t pcount;
			operator const bool() const
			{
				return type != "EOF"_hash;
			}
		};
		OBJLoder(const wstring &fname);
		~OBJLoder();
		TextLine readLine();
		int8_t parseFloat(const uint8_t idx, float *output);
		int8_t parseInt(const uint8_t idx, int32_t *output);
	};
	vector<Point> pts;
	vector<uint32_t> indexs;
	oglu::oglBuffer vbo, ebo;
	void loadOBJ(OBJLoder& ldr);
public:
	Model(const wstring& fname);
	virtual void prepareGL(const GLint(&attrLoc)[3]) override;
};

}