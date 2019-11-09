#include "oglPch.h"
#include "BindingManager.h"


namespace oglu::detail
{



GLuint TextureManager::getID(const oglTexBase& obj) const
{
    return obj->textureID;
}

void TextureManager::innerBind(const oglTexBase& obj, const uint16_t slot) const
{
    obj->bind(slot);
}

void TextureManager::outterBind(const GLuint prog, const GLuint loc, const uint16_t slot) const
{
    DSA->ogluProgramUniform1i(prog, loc, slot);
}


GLuint TexImgManager::getID(const oglImgBase& obj) const
{
    return obj->GetTextureID();
}

void TexImgManager::innerBind(const oglImgBase& obj, const uint16_t slot) const
{
    obj->bind(slot);
}

void TexImgManager::outterBind(const GLuint prog, const GLuint loc, const uint16_t slot) const
{
    DSA->ogluProgramUniform1i(prog, loc, slot);
}


GLuint UBOManager::getID(const oglUBO& obj) const
{
    return obj->BufferID;
}

void UBOManager::innerBind(const oglUBO& obj, const uint16_t slot) const
{
    obj->bind(slot);
}

void UBOManager::outterBind(const GLuint prog, const GLuint loc, const uint16_t slot) const
{
    DSA->ogluUniformBlockBinding(prog, loc, slot);
}

}