# AsyncExecutor

Async Task Executor, based on boost::context.

## Concept

Tasks are submitted to the manager from any thread. They are executed in another thread asynchronizely, leaving a promise to wait.

`boost::context` is used to keep stack information and stack-switch.

Tasks are dispatched in the order of submition. Proper promise can be waited inside async-thread, yielding the context to another task.

Promise waiting are handled by "select-like" polling strategy. Thread will be resumed after promise is finished(success or fail), but not accurately follow the order of events' finishing time (no native support for various promises' batch polling).

Execution thread will sleep several milliseconds if no task is avaliable, and will sleep for the condition_variable if no task in queue. It aims at low resource occupation, not for latency-sesative situation.

## Await support

Tasks can wait for normal promise but not stall thread by await provided by AsyncAgent.

Yield and Sleep is also natively supported.

## License

AsyncExecutor (including its component) is licensed under the [MIT license](../../License.txt).
