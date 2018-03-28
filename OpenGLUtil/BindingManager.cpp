#include "oglRely.h"
#include "BindingManager.h"


namespace oglu::detail
{



GLuint TextureManager::getID(const oglTexture& obj) const
{
	return obj->textureID;
}

void TextureManager::innerBind(const oglTexture& obj, const uint16_t slot) const
{
	obj->bind(slot);
}

void TextureManager::outterBind(const GLuint prog, const GLuint loc, const uint16_t slot) const
{
	glProgramUniform1i(prog, loc, slot);
}


GLuint UBOManager::getID(const oglUBO& obj) const
{
	return obj->bufferID;
}

void UBOManager::innerBind(const oglUBO& obj, const uint16_t slot) const
{
	obj->bind(slot);
}

void UBOManager::outterBind(const GLuint prog, const GLuint loc, const uint16_t slot) const
{
	glUniformBlockBinding(prog, loc, slot);
}

}