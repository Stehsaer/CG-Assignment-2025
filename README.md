# CG-Assignment-2025

This repository contains the source code for SYSU Computer Graphics course assignment (Professor Zhou Fan).

## Graphics Features
- glTF parsing and rendering
- glTF animations and skinning support
- Cascaded shadow maps
- Auto exposure algorithm
- Bloom effect
- HBAO with temporal denoising
- AgX tonemapping, providing visuals like Blender's EEVEE
- Frustum culling for camera and shadow views
- BC3, BC5 and BC7 image compression formats
- Screen space ReSTIR GI

## Current Stage
This project is currently under "Render-Tech Dev" stage. No game/interaction logic is implemented yet.

## Prerequisites

- Latest version of [xmake](https://xmake.io/)
- Python 3
- Vulkan SDK: Need a rather new vulkan SDK installation, needed for:
  - Vulkan headers 
  - GLSL/SPIR-V Compiler: `glslang` or `glslangValidator`
  - SPIR-V Optimizer: `spirv-opt`
- Modern C++ compiler with adequate C++23 support:
  - MSVC shipped with at least VS2022
  - GCC 14+ (this project is developed using GCC 15.2)
  > Some C++23 features such as `std::views::chunk` and `std::views::slide` are not supported by clang, thus it's not supported for now.
- Decent GPU system: this projects implements quite a lot of advanced features that consumes much GPU power. Don't expect it to run fast on an outdated or an integrated GPU.

## Building

### Step 1
Execute the following to automatically check the environment:
```bash
xmake check-env
```
This should check all the prerequisites for building.

### Step 2
Build the project using the following command:
```bash
xmake build
```
For first-time builds, xmake may prompt for installing third-party packages. Permit the installation to proceed.

### Step 3
Run:
```bash
xmake run main <gltf-file-path>
```
to run the program and see the visual outputs.