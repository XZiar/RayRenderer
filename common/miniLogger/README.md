# miniLogger

Mini Logger

## Concept

Logger is seperated as frontend and backend. Both sync/async backend are supported, but async backends are prefered.

Log content formation and message dispatcher are handled by frontend, which is almost "stateless" and runs at caller's thread.

Logging operations are handled by backend, which could be on another thread if LoggerQBackend(Queue based async backend) are used.

Backend are binded with logger instance, but they are "shared". Also, logger has a static backend, running on an isolated thread, accepting global callback bindings.

## Support Backend

* **Console Backend** `shared`
  
  Windows only, support colorful output.

* **Debuger Backend** `shared`
  
  Send debug message to debugger, such as VisualStudio's debug window, or stderr in Linux

* **File Backend** `not-shared`
  
  Simply write log to file

* **Global Backend** `shared`
  
  Global hooker. Used to capture logging in other runtime (.Net).

## License

miniLogger (including its component) is licensed under the [MIT license](../../License.txt).
