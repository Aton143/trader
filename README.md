()trader
A multiplatform, performant trading client and GUI with very few dependencies. Handmade?

## What trader can do
This project is still in its early stages, so to say. Right now, it supports HTTPS using a blocking- and sockets-based architecture sockets on Windows and has a Direct3D 11 rendering backend, running at 60fps with ease. We're (I am ) using OpenGL for the rendering backend on Linux (work can be found on the linux-bootstrap branch). It has the infrastructure to do hot-reloads using efficient methods (a.k.a. [I/O Completion Ports](https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports)), instrumented profiling, logging asserts, frame buffer captures, visual debugging, plots, text editing, and more! One might say it's awesome!

## Currently working on 
- Linux build on linux-bootstrap branch
- OpenGL renderer build-up and general rendering system cleanup
- Getting user input on Linux
- General feature parity with Windows

## Planned features
- [ ] Linux! Oh, no..., but going
- [ ] OpenGL backend for Linux
- [ ] Multi-threaded architecture

## To-Do
- [ ] User-created UI "workspace"
- [ ] JSON and HTTP response parsing

## Difficult-to-implement-but-useful features which may require a library for the time being
- [ ] HLSL &rarr; GLSL parser
- [ ] InputLayout/VAO generator from shader

## How to Compile
This project depends on stb\_image\_write.h, stb\_rect\_pack.h, stb\_sprintf.h, stb\_truetype.h, and OpenSSL. I haven't configured it to be built by another person so mileage will definitely vary. I have still yet to create submodules and a great way for others to build this thing. Luckily, it shouldn't be hard since the stb stuff is easy to take care of and OpenSSL static and dynamic libraries can be [easily obtained](https://wiki.openssl.org/index.php/Binaries). I haven't even changed the build scripts for release binaries!

## Other uses for this project
If you check out the linux-ubuntu-opengl-triangle branch, you can find how to initialize OpenGL on X11 without external libraries (that means you, [GLUT](https://freeglut.sourceforge.net/), [GLFW](https://github.com/glfw/glfw), [SDL](https://www.libsdl.org/), [ANGLE](https://github.com/google/angle), [Qt](https://doc.qt.io/qt-5/qtopengl-index.html),  etc.) in src/platform_linux/linux_trader. If you want the same for Direct3D 11, you should check out [Kevin Moran's tutorial](https://github.com/kevinmoran/BeginnerDirect3D11).

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
- [Eskil Steenberg](http://quelsolaar.com/about)
- [Ignacio Castaño](http://www.ludicon.com/castano/blog/)
- [Mike Acton](https://www.youtube.com/watch?v=4B00hV3wmMY)
