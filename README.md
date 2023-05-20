# trader
A multiplatform, performant trading client and GUI with very few dependencies.

## What trader can do
This project is still in its early stages, so to say. Right now, it supports HTTPS using a blocking- and sockets-based architecture sockets on Windows and has a Direct3D 11 rendering backend. It has the infrastructure to do hot-reloads using efficient methods (a.k.a. [I/O Completion Ports](https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports)), instrumented profiling, and logging asserts.
The Immediate Mode GUI library works to write text with support for left-, center-, and right-alignment.

## Planned features
- [ ] OpenGL backend for Linux
- [ ] Linux-based networking backend
- [ ] WebSockets and socket-based connections to APIs
- [ ] Immediate Mode GUI with real-time data visualization and manipulation
- [ ] Multi-threaded architecture
- [ ] Embedded Python Interpreter to conduct trades
