# trader
A multiplatform, performant trading client and GUI with very few dependencies.

## What trader can do
This project is still in its early stages, so to say. Right now, it supports HTTPS using a blocking- and sockets-based architecture sockets on Windows and has a Direct3D 11 rendering backend, running at 60fps with ease. It has the infrastructure to do hot-reloads using efficient methods (a.k.a. [I/O Completion Ports](https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports)), instrumented profiling, logging asserts, frame buffer captures, visual debugging, plots, text editing, and more! One might say it's awesome!

## Currently working on 
- Linux build on linux-bootstrap branch
- Reorganizing project for multiplatform builds

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

## How to Compile
This project depends on stb\_image\_write.h, stb\_rect\_pack.h, stb\_sprintf.h, stb\_truetype.h, and OpenSSL. I haven't configured it to be built by another person so mileage will definitely vary. I have still yet to create submodules and a great way for others to build this thing. Luckily, it shouldn't be hard since the stb stuff is easy to take care of and OpenSSL static and dynamic libraries can be [easily obtained](https://wiki.openssl.org/index.php/Binaries). I haven't even changed the build scripts for release binaries!

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
