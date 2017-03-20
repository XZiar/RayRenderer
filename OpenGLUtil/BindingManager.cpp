#include "oglRely.h"
#include "BindingManager.h"


namespace oglu::inner
{



GLuint TextureManager::getID(const oglTexture& obj) const
{
	return obj->textureID;
}

void TextureManager::innerBind(const oglTexture& obj, const uint8_t pos) const
{
	obj->bind(pos);
}

void TextureManager::outterBind(const GLuint pid, const GLuint pos, const uint8_t val) const
{
	glProgramUniform1i(pid, pos, val);
}

GLuint UBOManager::getID(const oglUBO& obj) const
{
	return obj->bufferID;
}

void UBOManager::innerBind(const oglUBO& obj, const uint8_t pos) const
{
	obj->bind(pos);
}

void UBOManager::outterBind(const GLuint pid, const GLuint pos, const uint8_t val) const
{
	glUniformBlockBinding(pid, pos, val);
}

}