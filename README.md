# trader
A multiplatform, performant trading client and GUI with very few dependencies.

## What trader can do
This project is still in its early stages, so to say. Right now, it supports HTTPS using a blocking- and sockets-based architecture sockets on Windows and has a Direct3D 11 rendering backend. It has the infrastructure to do hot-reloads using efficient methods (a.k.a. [I/O Completion Ports](https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports)), instrumented profiling, logging asserts, frame buffer captures, and visual debugging.  

## Currently working on 
- WebSockets implementation from soup to nuts
- Immediate Mode GUI
    - Real-time data chart creation and manipulation
    - Text Editing
    - Paneling

## Planned features
- [ ] Linux! Oh, no...
- [ ] OpenGL backend for Linux
- [ ] Linux-based networking backend
- [ ] WebSockets and socket-based connections to APIs
- [ ] Immediate Mode GUI with real-time data visualization and manipulation
- [ ] Multi-threaded architecture
- [ ] Embedded Python Interpreter to conduct trades

## To-Do
- [ ] User-created UI "workspace"
- [ ] JSON and HTTP response parsing

## Special Thanks
- [Casey Muratori](https://mollyrocket.com)
- [Mārtiņš Možeiko](https://github.com/mmozeiko)
- [d7samurai](https://github.com/d7samurai)
- [Ryan Fleury](https://www.rfleury.com/)
- [nothings](http://nothings.org/)
- [Tom Forsyth](https://tomforsyth1000.github.io/blog.wiki.html)
- [Omar Cornut](https://github.com/ocornut)
- [Jonathan Blow](http://number-none.com/blow/)
- [Kevin Moran](https://github.com/kevinmoran)
- [Allen Webster](https://mr4th.com/)
