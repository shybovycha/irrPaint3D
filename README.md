# Realtime model painting

## Overview

This is a simplistic implementation (at least a try to implement) LSCM algorithm as it is described in ALICE.

## Compiling and running

1. Install [vcpkg](https://github.com/microsoft/vcpkg/):
2. Use CMake in conjunction with vcpkg to install the dependencies and build the project:

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:\src\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build
```

Then just run `irr-paint-3d` binary from the `out` directory, passing the path to a 3d model
as the first command line argument and the output filename (unwrapped texture) as the second one:

```bash
.\out\build\x64-Debug\irr-paint-3d.exe out\build\x64-Debug\dwarf.x .\output.jpg
```

## License

### Code

See [LICENSE](LICENSE). The code is thought to be not only significantly different than the example(s) but also not covered by the engine license since examples are trivial and not part of the engine.

### Media
(dwarf model and texture from Irrlicht Example 8 SpecialFX)

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
