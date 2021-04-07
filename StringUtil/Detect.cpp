#include "Detect.h"
#include "3rdParty/uchardet/src/nscore.h"
#include "3rdParty/uchardet/src/nsUniversalDetector.h"

namespace common::str
{

namespace detail
{

class Uchardet : public nsUniversalDetector
{
protected:
    std::string Charset;
    virtual void Report(const char* charset) override
    {
        Charset = charset;
    }
public:
    Uchardet() : nsUniversalDetector(NS_FILTER_ALL) { }

    virtual void Reset() override
    {
        nsUniversalDetector::Reset();
        Charset.clear();
    }

    bool IsDone() const
    {
        return mDone == PR_TRUE;
    }

    std::string GetEncoding() const
    {
        return Charset.empty() ? (IsDone() ? mDetectedCharset : "") : Charset;
    }
};

}


CharsetDetector::CharsetDetector() : Tool(std::make_unique<detail::Uchardet>())
{
}

CharsetDetector::~CharsetDetector() {}

bool CharsetDetector::HandleData(const common::span<const std::byte> data) const
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

void CharsetDetector::Reset() const 
{
    Tool->Reset();
}

void CharsetDetector::EndData() const
{
    Tool->DataEnd();
}

std::string CharsetDetector::GetEncoding() const
{
    return Tool->GetEncoding();
}

std::string GetEncoding(const common::span<const std::byte> data)
{
    thread_local CharsetDetector detector;
    detector.Reset();
    if (!detector.HandleData(data))
        return "error";
    detector.EndData();
    return detector.GetEncoding();
}


}