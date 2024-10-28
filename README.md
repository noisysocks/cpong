# cpong

## Getting started

Native:

```bash
mkdir build
cmake -S . -B build
cmake --build build
./build/cpong/cpong
```

Web:

```bash
mkdir build_web
emcmake cmake -S . -B build_web -DPLATFORM=Web
emmake cmake --build build_web
emrun build_web/cpong/cpong.html
```
