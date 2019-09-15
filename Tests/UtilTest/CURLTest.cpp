#include "TestRely.h"
#include "MiniLogger/QueuedBackend.h"
#include "common/SpinLock.hpp"
#include "common/TimeUtil.hpp"
#include "StringCharset/Convert.h"
#include "fmt/format.h"
#define CURL_STATICLIB
#include "curl/include/curl/curl.h"
#include <vector>

#if defined(_WIN32)
#   pragma comment(lib, "Ws2_32.lib")
#endif

using std::string;
using std::vector;
using namespace common;
using namespace common::mlog;

static MiniLogger<false>& log()
{
    static MiniLogger<false> logger(u"CURLTest", { GetConsoleBackend() });
    return logger;
}

class CURLRequest
{
private:
    struct CURLInit
    {
        CURLInit()
        {
            curl_global_init(CURL_GLOBAL_ALL);
        }
        ~CURLInit()
        {
            curl_global_cleanup();
        }
    };
    CURL* Handle = nullptr;
    std::vector<std::byte> Data;

    size_t OnRecieve(void* ptr, size_t size)
    {
        auto pdata = static_cast<const std::byte*>(ptr);
        Data.insert(Data.end(), pdata, pdata + size);
        return size;
    }
    static size_t WriteFunc(void *ptr, size_t size, size_t nmemb, void *userp)
    {
        return static_cast<CURLRequest*>(userp)->OnRecieve(ptr, nmemb);
    }
public:
    CURLRequest(const std::string& url)
    {
        static CURLInit INIT;
        Handle = curl_easy_init();
        curl_easy_setopt(Handle, CURLOPT_URL, url.data());
        curl_easy_setopt(Handle, CURLOPT_WRITEFUNCTION, &WriteFunc);
        curl_easy_setopt(Handle, CURLOPT_WRITEDATA, this);
    }
    ~CURLRequest()
    {
        curl_easy_cleanup(Handle);
    }
    void Perform()
    {
        curl_easy_perform(Handle);
    }
    const std::vector<std::byte>& GetData() const { return Data; }
};

static void CURLTest()
{
    const auto url = "http://www.baidu.com";
    CURLRequest request(url);
    log().info(u"Access {}\n", url);
    request.Perform();
    const auto& data = request.GetData();
    const auto content = common::strchset::to_u16string(data.data(), data.size(), common::str::Charset::UTF8);
    log().info(u"Response:\n{}\n\n\n", content);
    getchar();
}


const static uint32_t ID = RegistTest("CURLTest", &CURLTest);
