# Ray Tracing in One Weekend: Animated Heart Edition 💖🚀

A highly optimized, multi-threaded C++ ray tracer built upon Peter Shirley's famous *Ray Tracing in One Weekend* series. This project extends the standard sphere-tracing engine with custom mathematical implicit surfaces (3D plump hearts), real-time chronological lighting, and OpenMP parallelization for blazing fast render speeds!

## ✨ Features

- **Custom Implicit Geometry (3D Hearts)**: Employs algebraic surfaces and ray marching with binary search refinement to accurately render perfect, artifact-free 3D hearts without polygons.
- **Time-Based Global Illumination**: Derives the scene's base color tint procedurally from your computer's exact local time (Year, Month, Day, Hour, Minute, Second), ensuring a unique visual mood every time you render.
- **OpenMP Multi-Threading**: Renders scanlines concurrently across all available CPU cores, resulting in massive speedups compared to the original single-threaded tracer.
- **Procedural Generation Layout**: Uses hardware-seeded randomness (`std::random_device`) to generate unique, densely packed scenes with random quantities, sizes, and metallic/glass layouts without clipping.
- **Multi-Format Saving Engine**: Built-in support for saving high-quality renders natively via `stb_image_write`.

## 🛠️ Building & Rendering

This project uses CMake for cross-platform building.

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

When you run the executable, the interactive CLI will guide you through:
1. **Selecting the Resolution Quality** (From lightning-fast SD testing up to glorious 4K rendering)
2. **Selecting the Image Format** (.png, .jpg, .bmp)

The engine will then dynamically calculate the scene using all your CPU threads and save the finished image directly to your working directory!

## 📜 Credits
- **Base Arc**: Based on the *Ray Tracing in One Weekend* book by Peter Shirley.
- **Image Saving**: Powered by the fantastic single-header library `stb_image_write.h`.

<img width="2560" height="1440" alt="image" src="https://github.com/user-attachments/assets/7eec57ec-1181-4efd-a22a-f7783599aab5" />
