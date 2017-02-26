#include "BindingManager.h"


namespace oglu
{



GLuint TextureManager::getID(const oglTexture& obj) const
{
	return obj->textureID;
}

void TextureManager::innerBind(const oglTexture& obj, const uint8_t pos) const
{
	obj->bind(pos);
}

GLuint UBOManager::getID(const oglBuffer& obj) const
{
	return obj->bufferID;
}

void UBOManager::innerBind(const oglBuffer& obj, const uint8_t pos) const
{
	glBindBufferBase(GL_UNIFORM_BUFFER, pos, obj->bufferID);
}

}