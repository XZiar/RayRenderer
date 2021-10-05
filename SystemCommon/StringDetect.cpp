#include "SystemCommonPch.h"
#include "StringDetect.h"
#include "uchardet/src/nscore.h"
#include "uchardet/src/nsUniversalDetector.h"

namespace common::str
{

namespace detail
{

class Uchardet : public nsUniversalDetector
{
protected:
    std::string Encoding;
    virtual void Report(const char* charset) override
    {
        Encoding = charset;
    }
public:
    Uchardet() : nsUniversalDetector(NS_FILTER_ALL) { }

    virtual void Reset() override
    {
        nsUniversalDetector::Reset();
        Encoding.clear();
    }

    bool IsDone() const
    {
        return mDone == PR_TRUE;
    }

    std::string GetEncoding() const
    {
        return Encoding.empty() ? (IsDone() ? mDetectedCharset : "") : Encoding;
    }
};

}


EncodingDetector::EncodingDetector() : Tool(std::make_unique<detail::Uchardet>())
{
}

EncodingDetector::~EncodingDetector() {}

bool EncodingDetector::HandleData(const common::span<const std::byte> data) const
{
    const size_t len = data.size();
    for (size_t offset = 0; offset < len && !Tool->IsDone();)
    {
        const uint32_t batchLen = static_cast<uint32_t>(std::min(len - offset, (size_t)/*UINT32_MAX*/4096));
        if (Tool->HandleData(reinterpret_cast<const char*>(data.data()) + offset, batchLen) != NS_OK)
            return false;
        offset += batchLen;
    }
    return true;
}

void EncodingDetector::Reset() const 
{
    Tool->Reset();
}

void EncodingDetector::EndData() const
{
    Tool->DataEnd();
}

std::string EncodingDetector::GetEncoding() const
{
    return Tool->GetEncoding();
}

std::string GetEncoding(const common::span<const std::byte> data)
{
    thread_local EncodingDetector detector;
    detector.Reset();
    if (!detector.HandleData(data))
        return "error";
    detector.EndData();
    return detector.GetEncoding();
}


}