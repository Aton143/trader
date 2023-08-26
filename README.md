# trader
An in-development game engine with very few dependencies for a small puzzle game. Formerly, a multiplatform trading client and GUI. Do you understand why it's called "trader"?

## What trader can do
This project is still in its early stages, so to say. Right now, it supports HTTPS using a blocking- and sockets-based architecture sockets on Windows and has a Direct3D 11 rendering backend, running at 60fps with ease. On Linux, development is underway for X11-based system using OpenGL as the rendering backend (work can be found on the linux-bootstrap branch). It has the infrastructure to do shader hot-reloads, instrumented profiling, logging asserts, frame buffer captures, visual debugging, plots, text editing, and more! One might say it's awesome!

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
- [ ] HLSL &rarr; GLSL converter (a.k.a. [hlslparser](https://github.com/Thekla/hlslparser))
- [ ] InputLayout/VAO generator from shader

## How to build
I haven't configured it to be built by another person so mileage will definitely vary. Make sure you have the requisite libraries on Linux. You could probably use the .github/workflows yaml as a source for a "vanilla build." Keep in mind that there aren't any release build scripts at the moment.

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
