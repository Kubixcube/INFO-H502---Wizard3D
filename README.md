# Wizard3D

A compact C++/OpenGL project featuring:
- a controllable **wizard** (first/third-person camera),
- **fireball** projectiles with physics and particle explosions,
- an **ice wall** with **planar reflections** (mirror camera + FBO, Fresnel blend with cubemap),
- Phong lighting, skybox, multiple OBJ models,
- a fixed-pool **particle system** with billboarding.

> This project targets the INFO-H-502 (3D Graphics) course's project.

---

## Demo
- Build and run (see below).

---

## Prerequisites
- **CMake** ≥ 3.20
- **C++17** toolchain
- **OpenGL 3.3 Core**
- Libraries (included/submoduled or fetched by CMake):
  - **GLFW**
  - **GLAD**
  - **GLM**
  - **stb_image**
  - **ReactPhysics3D**

> Assets (models, textures, skybox) are expected under `assets/`. CMake copies runtime assets/shaders next to the executable.

---

## Build & Run

### Windows (Visual Studio)
```powershell
# from repo root
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release -j 12

# Run from the build output so relative asset paths resolve
.\Release\Wizard3D.exe
