#pragma once

#include "WindowHostRely.h"
#include "WindowHost.h"

#include "ImageUtil/ImageCore.h"
#include "SystemCommon/MiniLogger.h"
#include "SystemCommon/Exceptions.h"
#include "SystemCommon/LoopBase.h"
#include "SystemCommon/PromiseTask.h"
#include "SystemCommon/SpinLock.h"
#include "common/TrunckedContainer.hpp"
#include <map>
#include <thread>
#include <future>


namespace xziar::gui
{
class WindowRunner;
class WindowHost_;

namespace detail
{
common::mlog::MiniLogger<false>& wdLog();

void RegisterInteraction(std::unique_ptr<IFilePicker> support) noexcept;
void RegisterBackend(std::unique_ptr<WindowBackend> backend) noexcept;
template<typename T>
inline bool RegisterBackend() noexcept
{
    static_assert(std::is_base_of_v<WindowBackend, T>);
    std::string_view name = T::BackendName;
    try
    {
        RegisterBackend(std::make_unique<T>());
    }
    catch (const common::BaseException& be)
    {
        wdLog().Warning(u"Failed to create backend [{}]: {}\n", name, be);
    }
    catch (const std::exception& ex)
    {
        wdLog().Warning(u"Failed to create backend [{}]: {}\n", name, ex.what());
    }
    catch (...)
    {
        wdLog().Warning(u"Failed to create backend [{}]\n", name);
    }
    return true;
}

template<typename T = void>
struct WaitablePayload;
template<>
struct WaitablePayload<void>
{
    WindowHost_* Host;
    std::promise<void> Promise;
    WaitablePayload(WindowHost_* host) : Host(host)
    { }
};
template<typename T>
struct WaitablePayload : WaitablePayload<void>
{
    T ExtraData;
    WaitablePayload(WindowHost_* host, T&& data) : WaitablePayload<void>{ host }, ExtraData(std::forward<T>(data))
    { }
};
using CreatePayload = WaitablePayload<const std::function<std::any(std::string_view)>*>;

template<typename T, typename U>
inline void TryGetT(const T*& dst, const std::any& dat) noexcept
{
    if (!dst)
    {
        const auto ptr = std::any_cast<U>(&dat);
        if (ptr)
            dst = reinterpret_cast<const T*>(ptr);
    }
}
template<typename T, typename... Ts>
inline const T* TryGetFinally(const std::any& dat) noexcept
{
    const T* ret = nullptr;
    if (dat.has_value())
    {
        TryGetT<T, T>(ret, dat);
        (..., void(TryGetT<T, Ts>(ret, dat)));
    }
    return ret;
}
//struct CreatePayload : public WaitablePayload<void>
//{
//    const std::function<const void* (std::string_view)>* Provider;
//    CreatePayload(WindowHost_* host, const std::function<const void* (std::string_view)>& provider) :
//        WaitablePayload{ host }, Provider(provider ? &provider : nullptr)
//    { }
//};

struct OpaqueResource
{
    using TDispose = void(const OpaqueResource&) noexcept;
    std::array<uint64_t, 3> Cookie;
    TDispose* Disposer;
    constexpr OpaqueResource() noexcept : Cookie{ 0u,0u,0u }, Disposer(nullptr) {}
    OpaqueResource(TDispose* disposer, void* ptr) noexcept : Cookie{ reinterpret_cast<uintptr_t>(ptr), 0u, 0u }, Disposer(disposer) {}
    constexpr OpaqueResource(TDispose* disposer, uint64_t val0, uint64_t val1, uint64_t val2) noexcept : Cookie{ val0, val1, val2 }, Disposer(disposer) {}
    constexpr OpaqueResource(TDispose* disposer, uint64_t val0, uint64_t val1) noexcept : OpaqueResource(disposer, val0, val1, 1u) {}
    constexpr OpaqueResource(TDispose* disposer, uint64_t val0) noexcept : OpaqueResource(disposer, val0, 0u, 1u) {}
    COMMON_NO_COPY(OpaqueResource)
    OpaqueResource(OpaqueResource&& rhs) noexcept : Cookie(rhs.Cookie), Disposer(rhs.Disposer)
    {
        rhs.Disposer = nullptr;
    }
    OpaqueResource& operator= (OpaqueResource&& rhs) noexcept
    {
        if (&rhs != this)
        {
            std::swap(Cookie, rhs.Cookie);
            std::swap(Disposer, rhs.Disposer);
            rhs.~OpaqueResource();
        }
        return *this;
    }
    ~OpaqueResource()
    {
        if (Disposer)
        {
            Disposer(*this);
            Disposer = nullptr;
        }
    }
    constexpr operator bool() const noexcept { return Disposer; }
};


struct LockField
{
    std::atomic<uint32_t> Data;
    constexpr LockField() noexcept : Data(0) {}
    
    static forceinline void DoLock(LockField& target, uint8_t index) noexcept
    {
        const auto bit = 1u << (index * 2);
        while (true)
        {
            const auto oldVal = target.Data.fetch_or(bit);
            if (!(oldVal & bit)) break;
            COMMON_PAUSE();
        }
    }
    static forceinline void DoUnlock(LockField& target, uint8_t index) noexcept
    {
        const auto bit = 1u << (index * 2);
        target.Data.fetch_and(~bit);
    }
    static forceinline bool LockChange(LockField& target, uint8_t index) noexcept // lock bit + set change bit
    {
        const auto lockbit = 1u << (index * 2);
        const auto changebit = 10u << (index * 2);
        while (true)
        {
            const auto oldVal = target.Data.fetch_or(lockbit);
            if (!(oldVal & lockbit)) // get lock
            {
                if (!(oldVal & changebit)) // change bit unset
                {
                    target.Data.fetch_or(changebit);
                    return false;
                }
                return true;
            }
            COMMON_PAUSE();
        }
        CM_UNREACHABLE();
    }
    static forceinline bool LockApply(LockField& target, uint8_t index) noexcept // lock bit + unset change bit
    {
        const auto lockbit = 1u << (index * 2);
        const auto changebit = 10u << (index * 2);
        while (true)
        {
            const auto oldVal = target.Data.fetch_or(lockbit);
            if (!(oldVal & lockbit)) // get lock
            {
                if (oldVal & changebit) // change bit set
                {
                    target.Data.fetch_and(~changebit);
                    return true;
                }
                return false;
            }
            COMMON_PAUSE();
        }
        CM_UNREACHABLE();
    }
    template<uint8_t Index>
    struct ConsumerLock
    {
        static_assert(Index < 16);
        LockField& Target;
        const bool Changed;
        ConsumerLock(LockField& target) noexcept : Target(target), Changed(LockApply(Target, Index)) {}
        COMMON_NO_COPY(ConsumerLock)
        COMMON_NO_MOVE(ConsumerLock)
        ~ConsumerLock()
        {
            DoUnlock(Target, Index);
        }
        constexpr operator bool() const noexcept { return Changed; }
    };
    template<uint8_t Index>
    struct ProducerLock
    {
        static_assert(Index < 16);
        LockField& Target;
        const bool AlreadyChanged = false;
        ProducerLock(LockField& target) noexcept : Target(target), AlreadyChanged(LockChange(Target, Index))
        { }
        COMMON_NO_COPY(ProducerLock)
        COMMON_NO_MOVE(ProducerLock)
        ~ProducerLock()
        {
            DoUnlock(Target, Index);
        }
        constexpr operator bool() const noexcept { return AlreadyChanged; }
    };
};

namespace WdAttrIndex
{
inline constexpr uint8_t Title = 0;
inline constexpr uint8_t Icon = 1;
inline constexpr uint8_t Custom = 8;
}


template<typename Ch>
class StableStringPool
{
    common::spinlock::WRSpinLock Lock;
    common::container::TrunckedContainer<Ch> Pool;
public:
    StableStringPool() noexcept : Pool(4096, 8) {}
    COMMON_NO_COPY(StableStringPool)
    COMMON_NO_MOVE(StableStringPool)
    template<typename F>
    std::basic_string_view<Ch> Put(const std::basic_string_view<Ch>& str, F&& func) noexcept
    {
        auto lock = Lock.WriteScope();
        const auto space = Pool.Alloc(str.size());
        memcpy_s(space.data(), space.size_bytes(), str.data(), str.size() * sizeof(Ch));
        std::basic_string_view<Ch> ret{ space.data(), str.size() };
        func(ret);
        return ret;
    }
    std::basic_string_view<Ch> Put(const std::basic_string_view<Ch>& str) noexcept
    {
        common::span<Ch> space;
        {
            auto lock = Lock.WriteScope();
            space = Pool.Alloc(str.size());
        }
        memcpy_s(space.data(), space.size_bytes(), str.data(), str.size() * sizeof(Ch));
        return { space.data(), str.size() };
    }
    auto LockRead() noexcept
    {
        return Lock.ReadScope();
    }
};


template<typename F>
forceinline bool WrapException(F&& func, common::mlog::MiniLogger<false>& logger, std::u16string_view msg) noexcept
{
    try
    {
        func();
        return true;
    }
    catch (::common::BaseException& be)
    {
        logger.Warning(u"Exception during {}: {}\n", msg, be);
    }
    catch (...)
    {
        logger.Warning(u"Exception during {}\n", msg);
    }
    return false;
}

class WindowManager
{
    friend WindowHost_;
    friend WindowRunner;
private:
    struct InvokeNode : public common::NonMovable, public common::container::IntrusiveDoubleLinkListNodeBase<InvokeNode>
    {
        std::function<void(void)> Task;
        InvokeNode(std::function<void(void)>&& task) : Task(std::move(task)) { }
    };

    std::vector<std::pair<uintptr_t, WindowHost>> WindowList;
    common::container::IntrusiveDoubleLinkList<InvokeNode, common::spinlock::WRSpinLock> InvokeList;
    static LockField& GetResourceLock(WindowHost_* host) noexcept;
    template<uint8_t Index>
    class ResSetLock : private LockField::ProducerLock<Index>
    {
    public:
        ResSetLock(WindowHost_* host) noexcept : LockField::ProducerLock<Index>(GetResourceLock(host)) {}
        using LockField::ProducerLock<Index>::operator bool;
    };
protected:
    struct ReSizingLock
    {
        const WindowHost_& Host;
        ReSizingLock(const WindowHost_& host);
        ~ReSizingLock();
    };
    [[nodiscard]] static bool TryLockWindow(WindowHost_& host, uint8_t idx) noexcept;
    static void LockWindow(WindowHost_& host, uint8_t idx) noexcept;
    static bool UnlockWindow(WindowHost_& host, uint8_t idx) noexcept;
    template<uint8_t Idx, typename F>
    static decltype(auto) LockWindowFor(WindowHost_& host, F&& task) noexcept
    {
        static_assert(Idx < 16);
        LockWindow(host, Idx);
        decltype(auto) ret = task();
        const auto suc = UnlockWindow(host, Idx);
        Ensures(suc);
        return ret;
    }
    [[nodiscard]] static OpaqueResource* GetWindowResource(WindowHost_* host, uint8_t resIdx) noexcept; // only get ptr, should lock before access content when needed
    template<uint8_t Index>
    class ResApplyLock : private LockField::ConsumerLock<Index>
    {
    public:
        ResApplyLock(WindowHost_* host) noexcept : LockField::ConsumerLock<Index>(GetResourceLock(host)) {}
        using LockField::ConsumerLock<Index>::operator bool;
    };
    using TitleLock = ResApplyLock<WdAttrIndex::Title>;
    using IconLock = ResApplyLock<WdAttrIndex::Icon>;
    template<uint8_t Idx>
    using CustomLock = ResApplyLock<WdAttrIndex::Custom + Idx>;
    template<uint8_t Idx>
    using CustomSetLock = ResSetLock<WdAttrIndex::Custom + Idx>;
    template<typename T>
    struct RectBase
    {
        T Width, Height;
        constexpr RectBase() noexcept : Width(0), Height(0) {}
        constexpr RectBase(T w, T h) noexcept : Width(w), Height(h) {}
        constexpr RectBase(const WindowHost_& wd) noexcept : Width(static_cast<T>(wd.Width)), Height(static_cast<T>(wd.Height)) {}
        void FormatWith(common::str::FormatterExecutor& executor, common::str::FormatterExecutor::Context& context, const common::str::FormatSpec* spec) const
        {
            using U = std::make_unsigned_t<T>;
            executor.PutInteger(context, static_cast<U>(Width), std::is_signed_v<T>, spec);
            executor.PutString(context, "x", nullptr);
            executor.PutInteger(context, static_cast<U>(Height), std::is_signed_v<T>, spec);
        }
        constexpr bool operator==(const RectBase<T>& rhs) const noexcept { return Width == rhs.Width && Height == rhs.Height; }
        constexpr bool operator!=(const RectBase<T>& rhs) const noexcept { return Width != rhs.Width || Height != rhs.Height; }
        constexpr std::pair<T, T> ResizeWithin(uint32_t w, uint32_t h) const noexcept
        {
            const auto dw = this->Width, dh = this->Height;
            const auto wAlignH = uint64_t(dw) * h / w, hAlignW = uint64_t(dh) * w / h;
            Ensures((wAlignH <= (uint32_t)dh) || (hAlignW <= (uint32_t)dw));
            T tw = 0, th = 0;
            if (wAlignH <= (uint32_t)dh) // W-align
                tw = dw, th = static_cast<T>(wAlignH);
            else // H-align
                tw = static_cast<T>(hAlignW), th = dh;
            return { tw, th };
        }
    };
    template<typename T>
    struct CacheRect : public RectBase<T>
    {
        // return { sizeChanged, needInitialize }
        std::pair<bool, bool> Update(const WindowHost_& wd) noexcept
        {
            ReSizingLock lock(wd);
            RectBase<T> newSize(wd);
            if (*this != newSize)
            {
                const bool initilized = this->Width > 0 && this->Height > 0;
                static_cast<RectBase<T>&>(*this) = newSize;
                return { true, !initilized };
            }
            return { false, false };
        }
    };

    WindowManager();

    template<typename F>
    forceinline bool WrapException(F&& func, std::u16string_view msg) const noexcept
    {
        return ::xziar::gui::detail::WrapException(std::forward<F>(func), Logger, msg);
    }
    template<typename T>
    void RegisterHost(T handle, WindowHost_* host)
    {
        WindowList.emplace_back((uintptr_t)handle, host->shared_from_this());
    }
    template<typename T>
    [[nodiscard]] WindowHost_* GetWindow(T handle) const noexcept
    {
        const auto window = (uintptr_t)handle;
        for (const auto& pair : WindowList)
        {
            if (pair.first == window)
            {
                return pair.second.get();
            }
        }
        return nullptr;
    }
    [[nodiscard]] xziar::img::Image TryReadImage(common::span<const std::byte> data, std::u16string_view ext, xziar::img::ImgDType dtype, std::u16string_view msg = u"parse image") const noexcept;
    [[nodiscard]] std::optional<bool> InvokeClipboard(const std::function<bool(ClipBoardTypes, const std::any&)>& handler, ClipBoardTypes type, const std::any& dat) const noexcept;
    [[nodiscard]] forceinline bool InvokeClipboard(const std::function<bool(ClipBoardTypes, const std::any&)>& handler, ClipBoardTypes type, const std::any& dat, bool stopAtFault) const noexcept
    {
        return InvokeClipboard(handler, type, dat).value_or(stopAtFault);
    }
    bool UnregisterHost(WindowHost_* host);
    void HandleTask();

    virtual void NotifyTask() noexcept = 0;
public:
    mutable common::mlog::MiniLogger<false> Logger;

    virtual ~WindowManager();
    virtual bool CheckCapsLock() const noexcept = 0;
    virtual void CreateNewWindow(CreatePayload& payload) = 0;
    virtual void BeforeWindowOpen(WindowHost_*) const {}
    virtual void AfterWindowOpen(WindowHost_*) const {}
    virtual void UpdateTitle(WindowHost_* host) const = 0;
    virtual void UpdateIcon(WindowHost_*) const {}
    virtual void CloseWindow(WindowHost_* host) const = 0;
    virtual void ReleaseWindow(WindowHost_* host) = 0;
    virtual const void* GetWindowData(const WindowHost_* host, std::string_view name) const noexcept;
    virtual OpaqueResource PrepareIcon(WindowHost_&, xziar::img::ImageView) const noexcept { return {}; }

    void AddInvoke(std::function<void(void)>&& task);

    // void Invoke(std::function<void(WindowHost_&)> task);
};


void FillBufferColorXXXA(uint32_t* ptr, uint32_t w, uint32_t h, uint32_t rowStride, uint32_t idx = 0) noexcept;
#if COMMON_OS_UNIX
xziar::gui::event::CombinedKey ProcessXKBKey(void* state, uint8_t keycode) noexcept;
FileList UriStringToFiles(std::string_view str) noexcept;
#endif


}

}
