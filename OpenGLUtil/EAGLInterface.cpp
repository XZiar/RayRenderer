#include "oglPch.h"
#include "oglUtil.h"
#include "oglContext.h"
#include "SystemCommon/ObjCHelper.h"
#include "SystemCommon/NSFoundation.h"

#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFURL.h>
#include <CoreFoundation/CFBundle.h>


extern "C"
{
    void EAGLGetVersion(unsigned int* major, unsigned int* minor);
}

namespace oglu
{
using namespace std::string_view_literals;
using common::objc::Clz;
using common::objc::ClzInit;
using common::objc::Instance;
using common::foundation::NSString;

// struct NSBundle : public Instance
// {
//     static const ClzInit<id> NSBundleClass("NSBundle", false, "bundleWithPath:");
//     NSBundle(std::u16string_view path) : Instance(NSBundleClass.Create(NSString::Create(u"/System/Library/Frameworks/OpenGLES.framework")))
//     {
//     }
// };


class EAGLLoader_ final : public EAGLLoader
{
private:
    struct EAGLHost final : public EAGLLoader::EAGLHost
    {
        friend EAGLLoader_;
        uint16_t Version = 0;
        bool IsOffscreen;
        static inline const ClzInit<NSUInteger, id> EAGLContextClass = ClzInit<NSUInteger, id>("EAGLContext", false, "initWithAPI:sharegroup:");
        static inline const SEL SelSetCurrent = sel_registerName("setCurrentContext:");
        static inline const SEL SelGetCurrent = sel_registerName("currentContext");
        static inline const SEL SelGetSharegroup = sel_registerName("sharegroup");
        EAGLHost(EAGLLoader_& loader, bool isOffscreen) : EAGLLoader::EAGLHost(loader), IsOffscreen(isOffscreen)
        {
            SupportES = true;
            SupportDesktop = false;
            unsigned int major = 0, minor = 0;
            EAGLGetVersion(&major, &minor);
            Version = static_cast<uint16_t>(major * 10 + minor);
        }
        ~EAGLHost() final
        {
        }
        void BindDrawable(oglContext_& ctx_, void* caeaglLayer) final
        {
            static const SEL SelRBOStorage = sel_registerName("renderbufferStorage:fromDrawable:");
            const auto oldCtx = oglContext_::CurrentContext();
            ctx_.UseContext();
            Instance ctx(reinterpret_cast<id>(CtxFunc->Target));
            GLuint rbo = GL_INVALID_INDEX;
            CtxFunc->CreateRenderbuffers(1, &rbo);
            CtxFunc->BindRenderbuffer_(GL_RENDERBUFFER, rbo);
            ctx.Call<BOOL, NSUInteger, id>(SelRBOStorage, rbo, reinterpret_cast<id>(caeaglLayer));
            // TODO: save rbo info into shared base
            oldCtx->UseContext();
        }
        void* CreateContext_(const CreateInfo& cinfo, void* sharedCtx) noexcept final
        {
            NSUInteger ver = cinfo.Version == 0 ? 3 : cinfo.Version / 10;
            if (cinfo.Version % 10 != 0)
                return nullptr;
            id shareGroup = nullptr;
            if (sharedCtx)
                shareGroup = Instance(reinterpret_cast<id>(sharedCtx)).Call<id>(SelGetSharegroup);
            return EAGLContextClass.Create(ver, reinterpret_cast<id>(sharedCtx));
        }
        bool MakeGLContextCurrent_(void* hRC) const final
        {
            return EAGLContextClass.Call<BOOL, id>(SelSetCurrent, reinterpret_cast<id>(hRC)) == YES;
        }
        void DeleteGLContext(void* hRC) const final
        {
            Instance ctx(reinterpret_cast<id>(hRC));
            ctx.Release();
        }
        void SwapBuffer() const final
        {
            static const SEL SelPresent = sel_registerName("presentRenderbuffer:");
            Instance ctx(reinterpret_cast<id>(CtxFunc->Target));
            // TODO: get and bind rbo from shared base
            // CtxFunc->BindRenderbuffer_(GL_RENDERBUFFER, rbo);
            ctx.Call<BOOL, NSUInteger>(SelPresent, GL_RENDERBUFFER);
        }
        void ReportFailure(std::u16string_view action) const final
        {
            oglLog().Error(u"Failed to {}\n"sv, action);
        }
        void TemporalInsideContext(void* hRC, const std::function<void(void* hRC)>& func) const final
        {
            const auto oldHrc = EAGLContextClass.Call<id>(SelGetCurrent);
            if (MakeGLContextCurrent_(hRC))
            {
                func(hRC);
                MakeGLContextCurrent_(oldHrc);
            }
        }
        uint32_t GetVersion() const noexcept final { return Version; }
        void* GetDeviceContext() const noexcept final { return nullptr; }
        void* LoadFunction(std::string_view name) const noexcept final
        {
            const auto nameStr = CFStringCreateWithBytes(nullptr, reinterpret_cast<const UInt8*>(name.data()),
                static_cast<CFIndex>(name.size()), kCFStringEncodingUTF8, FALSE);
            const auto func = CFBundleGetFunctionPointerForName(static_cast<EAGLLoader_&>(Loader).OpenGLESBundle, nameStr);
            CFRelease(nameStr);
            return func;
        }
        void* GetBasicFunctions(size_t, std::string_view) const noexcept final
        {
            return nullptr;
        }
    };
    //Instance OpenGLESBundle;
    static inline const CFStringRef GLESPathStr = CFSTR("/System/Library/Frameworks/OpenGLES.framework");
    static inline const CFURLRef GLESPath = CFURLCreateWithFileSystemPath(nullptr, GLESPathStr, kCFURLPOSIXPathStyle, TRUE);
    CFBundleRef OpenGLESBundle;
public:
    static constexpr std::string_view LoaderName = "EAGL"sv;
    EAGLLoader_() : OpenGLESBundle(CFBundleCreate(nullptr, GLESPath))
    {
    }
    ~EAGLLoader_() final
    {
        CFRelease(OpenGLESBundle);
    }
private:

    std::string_view Name() const noexcept final { return LoaderName; }
    std::u16string Description() const noexcept final
    {
        return u"EAGL";
    }

    /*void Init() override
    { }*/

    std::shared_ptr<EAGLLoader::EAGLHost> CreateHost(bool useOffscreen) final
    {
        return std::make_shared<EAGLHost>(*this, useOffscreen);
    }

    static inline const auto Dummy = detail::RegisterLoader<EAGLLoader_>();
};


}
