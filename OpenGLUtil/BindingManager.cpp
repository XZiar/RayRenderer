#include "oglPch.h"
#include "BindingManager.h"


namespace oglu::detail
{


template<>
GLuint CachedResManager<oglTexBase_>::GetID(const oglTexBase_* tex) noexcept
{
    return tex->TextureID;
}
template<>
void CachedResManager<oglTexBase_>::BindRess(const oglTexBase_* tex, const uint16_t slot) noexcept
{
    tex->bind(slot);
}
template<>
void CachedResManager<oglTexBase_>::BindToProg(const GLuint progId, const GLuint loc, const uint16_t slot) noexcept
{
    CtxFunc->ogluProgramUniform1i(progId, loc, slot);
}


template<>
GLuint CachedResManager<oglImgBase_>::GetID(const oglImgBase_* img) noexcept
{
    return img->GetTextureID();
}
template<>
void CachedResManager<oglImgBase_>::BindRess(const oglImgBase_* img, const uint16_t slot) noexcept
{
    img->bind(slot);
}
template<>
void CachedResManager<oglImgBase_>::BindToProg(const GLuint progId, const GLuint loc, const uint16_t slot) noexcept
{
    CtxFunc->ogluProgramUniform1i(progId, loc, slot);
}


template<>
GLuint CachedResManager<oglUniformBuffer_>::GetID(const oglUniformBuffer_* ubo) noexcept
{
    return ubo->BufferID;
}
template<>
void CachedResManager<oglUniformBuffer_>::BindRess(const oglUniformBuffer_* ubo, const uint16_t slot) noexcept
{
    ubo->bind(slot);
}
template<>
void CachedResManager<oglUniformBuffer_>::BindToProg(const GLuint progId, const GLuint loc, const uint16_t slot) noexcept
{
    CtxFunc->ogluUniformBlockBinding(progId, loc, slot);
}


}