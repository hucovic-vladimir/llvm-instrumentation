# Prerequisites

## Profiler
- LLVM 17.0.0 or higher
- CMake
- C++20 compiler

## Visualizations
- Python 3.9 or higher
- Graphviz

# How to build

Run these commands in the root directory. Replace `path/to/llvm/cmake` with the path to the LLVM cmake directory.
If you built LLVM from source, this directory is located in the `build/lib/cmake/llvm` in the location where
you built LLVM.

```bash
mkdir build
cd build
cmake ..
make
```

The `libInstructionCount.so` library will be built in the build/src directory.


# How to use

## Profiling
- Add `--fpass-plugin=/path/to/libInstructionCount.so` to `CFLAGS`.
- Add `--fuse-ld=/path/to/link.sh` to `LDFLAGS`.
- Compile from the root of your project. The `.basicblocks` and `.patterns` directories will be created.
- Run the executable to generate a profile.


## Visualization
- Run `python3 viz/generate_html.py --basicblocks path/to/.basicblocks --patterns path/to/.patterns \
 --project-root path/to/your/project/root --out-dir path/to/output/directory --profile path/to/profile.json`
