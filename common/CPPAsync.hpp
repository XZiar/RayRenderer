#pragma once

#include "CommonRely.hpp"
#include "Exceptions.hpp"
#include "PromiseTask.hpp"
#include <functional>
#include <memory>
#include <atomic>

namespace common
{



//void RunAsync(const OnFinish& onFinish, const OnException& onException)
//{
//    if (HasExecuted.test_and_set())
//    {
//        std::thread([mainJob = std::move(MainJob), postJob = std::move(PostJob)]()
//        {
//            try
//            {
//                auto midArg = mainJob();
//                onFinish([midArg = std::move(midArg), postJob = std::move(postJob)]() { return postJob(midArg); });
//            }
//            catch (BaseException& e)
//            {
//                onException(e);
//            }
//        }).detach();
//    }
//}

}