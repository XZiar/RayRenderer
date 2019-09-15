# MiniLogger

Mini Logger

A simple logger that provide thread-safe(maybe) logging and global logging management.

It uses [fmt](../3rdParty/fmt) as format support.

## Concept

Logger is separated as frontend and backend. Both sync/async backend are supported, but async backends are preferred.

Log content formation and message dispatcher are handled by frontend, which is almost "stateless" and runs at caller's thread.

Logging operations are handled by backend, which could be on another thread if LoggerQBackend(Queue based async backend) are used.

Backend are bound with logger instance, but they are "shared". Also, logger has a static backend, running on an isolated thread, accepting global callback bindings.

## Support Backend

* **Console Backend** `shared`
  
  Windows only, support colorful output.

* **Debugger Backend** `shared`
  
  Send debug message to debugger, such as VisualStudio's debug window, or stderr in Linux

* **File Backend** `not-shared`
  
  Simply write log to file

* **Global Backend** `shared`
  
  Global hook. Expose ability to capture logs in other runtime (.Net).

## License

MiniLogger (including its component) is licensed under the [MIT license](../../License.txt).
