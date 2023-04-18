# trader
A multiplatform, performant trading client and GUI with very few dependencies

## What trader can do
---
This project is still in its early stages, so to say. Right now, it supports HTTPS using a blocking- and sockets-based architeture sockets on Windows and has a Direct3D 11 rendering backend.

## Planned features
---
- [ ] OpenGL backend for Linux
- [ ] Linux-based networking backend
- [ ] WebSockets and socket-based connections to APIs
- [ ] Immediate Mode GUI with real-time data visualization and manipulation
- [ ] Multi-threaded architecture using [I/O Completion Ports](https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports) on Windows and [io_uring](https://man.archlinux.org/man/io_uring.7.en) on Linux
- [ ] Embedded Python Interpreter to conduct trades
