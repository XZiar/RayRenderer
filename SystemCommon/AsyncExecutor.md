# AsyncExecutor

Async Task Support.

## [AsyncManager](AsyncManager.h)

An stackful coroutine environment using [boost.context](../3rdParty/boost.context) as backend support.

It provides general async promise-waiting via PromiseTask.

It simply uses a polling scheduler, waiting events are queried every xx ms(default 20ms).

### Concept

Tasks are submitted to the manager from any thread. They are executed in another thread asynchronizely, leaving a promise to wait.

`boost::context` is used to keep stack information and stack-switch.

Tasks are dispatched in the order of submit. Proper promise can be waited inside async-thread via `AsyncAgent`, yielding the context to another task.

Promise waiting are handled by "select-like" polling strategy. Thread will be resumed after promise is finished(success or fail) or explicitly awaiting, but not accurately follow the order of events' finishing time (no native support for various promises' batch polling).

Execution thread will sleep several milliseconds if no task is runnable, and will sleep for the condition_variable if no task in queue. It aims at low resource occupation, not for latency-sensitive situation.

### Await support

Tasks inside AsyncExecutor can wait for normal promise but not stall thread by await provided by [`AsyncAgent`](AsyncAgent.h).

Also, AsyncAgent will register into threadlocal so any functions can call `SafeWait` in case they are in the AsyncExecutor environment. Waiting for a promise of the same thread natively will cause deadlock.

`Yield` and `Sleep` is also natively supported, based on Executor's polling strategy rather than low-level signal or other thread.

## [AsyncProxy](AsyncProxy.h)

A proxy to enable callback based await for all PromiseTask.

PromiseTask is waited actively and will cause host stalled until task finished. To enable async waiting, the host need to expose passively signal recieving and a proxy(`AsyncProxy`) to perform the active task waiting.

`AsyncProxy` runs a loop in a standalone thread, invoke the callback when the task is completed. 

It can be used with [`CLIAsync`](../common/CLIAsync.hpp) to provide .Net's `Task<>`-based async-await experience.

## License

AsyncExecutor (including its component) is licensed under the [MIT license](../../License.txt).
