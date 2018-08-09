#pragma once
#include <cstdint>

namespace common
{

template<typename T, size_t N = 32>
class AvgCounter
{
private:
    T Data[N] = {0};
    uint64_t Count = 0;
    T Sum = 0;
public:
    T GetAvg() const { return Sum / (Count > N ? N : Count); }
    T Push(const T dat)
    {
        T& slot = Data[(Count++) % 32];
        Sum -= slot;
        slot = dat;
        Sum += dat;
        return GetAvg();
    }
};


}