# cpong

A curvy pong game written in C using [Raylib](https://www.raylib.com).

## Getting started

Native:

```bash
mkdir build
cmake -S . -B build
cmake --build build
cd build/cpong
./cpong
```

Web:

```bash
mkdir build_web
emcmake cmake -S . -B build_web -DPLATFORM=Web
emmake cmake --build build_web
cd build_web/cpong
emrun cpong.html
```
