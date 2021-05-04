# Realtime model painting

## Overview

This application aims at imlpementing painting 3D models in realtime.

Currently application only picks the UV coordinates of a selected point on a 3D model and shows it in a texture preview window.
If your model has multiple materials with textures, the ones without textures are not displayed - there is simply no reason for that.

![Screenshot](https://raw.githubusercontent.com/shybovycha/irrPaint3D/master/media/screenshot2.png)

## TODO

### Required

Features still not implemented but required:

- [ ] Actually drawing on the model
- [ ] Saving the modified texture

### Optional

Things that are not really required but which would greatly improve user experience:

- [ ] Translating / rotating / scaling the model instead of the camera
- [ ] Choosing the brushes - color, size, opacity
- [ ] Preventing screen from flickering with black when hovering over model
- [ ] Pointing to the selected point on a model with 3D arrow instead of highlighting the whole triangle
- [ ] Updating model texture on the fly

## Compiling and running

1. Install [vcpkg](https://github.com/microsoft/vcpkg/):
2. Use CMake in conjunction with vcpkg to install the dependencies and build the project:

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:\src\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build
```

Then just run `irr-paint-3d` binary from the `out` directory.

```bash
.\out\build\x64-Debug\irr-paint-3d.exe
```

Use UI to open the 3D model. Use `RMB` (Right Mouse Button) to move camera around and `LMB` (Left Mouse Button) to rotate camera.
To zoom in and out use `RMB + LMB`.

## License

### Code

See [LICENSE](LICENSE). The code is thought to be not only significantly different than the example(s) but also not covered by the engine license since examples are trivial and not part of the engine.

### Media

* dwarf model and texture from [Irrlicht Example 8 SpecialFX](http://irrlicht.sourceforge.net/docu/example008.html)

```
  The Irrlicht Engine License
  ===========================

  Copyright (C) 2002-2015 Nikolaus Gebhardt

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgement in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be clearly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
```
