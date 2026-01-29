## CPU .obj file renderer

This project contains a CPU-based .obj file renderer. The executable looks for a file named model.obj in its directory, and renders it in a window. As this project was written to develop a better understanding of 3D graphics pipelines, no external 3D graphics APIs were used. This project does not currently support textures.

## Screenshot

![alt text](media/sampleImage.png "Sample Model")

## Instructions

- Put a .obj file in the same directory as the executable, named model.obj.
- The obj file must have UV coordinates disabled for the file to load properly, as textures are not currently supported

## Controls

| Input        | Action              |
|-------------|---------------------|
| W / A / S / D | Move horizontally   |
| Space        | Move up             |
| Shift        | Move down           |
| Mouse        | Look around         |
| Esc          | Toggle cursor       |

## Build instructions
This project uses the Windows API, and will only work on windows.
```bash
mkdir build
cd build
cmake ..
cmake --build
```
The executable will be output to:
```bash
build/bin/renderer
```