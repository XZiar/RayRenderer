#include "uchardetlib.h"
#include "nscore.h"
#include "nsUniversalDetector.h"

namespace uchdet
{
using namespace common::str;


class Uchardet : public nsUniversalDetector
{
protected:
    std::string Charset;
    virtual void Report(const char* charset)
    {
        Charset = charset;
    }
public:
    Uchardet() : nsUniversalDetector(NS_FILTER_ALL) { }

    virtual void Reset()
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


CharsetDetector::CharsetDetector()
{
    Pointer = reinterpret_cast<intptr_t>(new Uchardet);
}

CharsetDetector::~CharsetDetector()
{
    auto tool = reinterpret_cast<Uchardet*>(Pointer);
    if (tool)
        delete(tool);
}

bool CharsetDetector::HandleData(const void *data, const size_t len) const
{
    auto tool = reinterpret_cast<Uchardet*>(Pointer);
    for (size_t offset = 0; offset < len && !tool->IsDone();)
    {
        const uint32_t batchLen = static_cast<uint32_t>(std::min(len, (size_t)/*UINT32_MAX*/1024));
        if (tool->HandleData(reinterpret_cast<const char*>(data) + offset, batchLen) != NS_OK)
            return false;
        offset += batchLen;
    }
    return true;
}

void CharsetDetector::Reset() const 
{
    auto tool = reinterpret_cast<Uchardet*>(Pointer);
    tool->Reset();
}

void CharsetDetector::EndData() const
{
    auto tool = reinterpret_cast<Uchardet*>(Pointer);
    tool->DataEnd();
}

std::string CharsetDetector::GetEncoding() const
{
    auto tool = reinterpret_cast<Uchardet*>(Pointer);
    return tool->GetEncoding();
}

std::string getEncoding(const void *data, const size_t len)
{
    thread_local CharsetDetector tool;
    tool.Reset();
    if (!tool.HandleData(data, len))
        return "error";
    tool.EndData();
    return tool.GetEncoding();
}


}