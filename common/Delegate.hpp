#include "CommonRely.hpp"
#include "SpinLock.hpp"
#include <map>
#include <functional>

namespace common
{


template<typename... Args>
class Delegate : public NonCopyable
{
private:
    std::atomic_uint32_t Indexer;
    std::map<uint32_t, std::function<void(const Args&...)>> Callbacks;
    RWSpinLock Lock;
public:
    void operator()(const Args&... args)
    {
        const auto lock = Lock.ReadScope();
        for (const auto pair : Callbacks)
            pair.second(args...);
    }
    template<typename T>
    uint32_t operator+=(T&& callback)
    {
        const auto lock = Lock.WriteScope();
        const auto idx = Indexer++;
        Callbacks.emplace(idx, std::forward<T>(callback));
        return idx;
    }
    bool operator-=(uint32_t idx)
    {
        const auto lock = Lock.WriteScope();
        Callbacks.erase(idx);
        return true;
    }
};


}
