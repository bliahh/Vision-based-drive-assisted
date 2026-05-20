# Vision Drive Assist — OpenGL HUD

Receives TCP JSON from the Python pipeline, renders a real-time ADAS HUD.

## Dependencies

```bash
# Ubuntu / Debian
sudo apt install libglew-dev libglfw3-dev nlohmann-json3-dev

# Arch
sudo pacman -S glew glfw-x11 nlohmann-json
```

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Run

```bash
# Start Python pipeline first (it sends TCP on port 5005)
python3 run/main.py --video my_drive.mp4

# In another terminal:
./build/adas_hud --port 5005
```

## Keys

| Key | Action          |
|-----|-----------------|
| `1` | Toggle DA area  |
| `2` | Toggle lane lines |
| `3` | Toggle cars     |
| `4` | Toggle signs    |
| `D` | Debug grid      |
| `S` | Screenshot (.ppm) |
| `Q` / `Esc` | Quit |

## Integration with your existing code

### tcp_client.cpp
Add your existing `tcp_client.cpp` to `CMakeLists.txt` under `add_executable`.

### Box coordinate space
`hud_renderer.cpp → drawCars()` currently normalizes boxes assuming a 1920×1080 source frame:
```cpp
float x1 = carBoxX(car.x1 / 1920.f);
```
If your frame is a different resolution, change those divisors, or normalize in `parseObjects()`.

### Lane coordinate space
Lane lines from HybridNets are in [0..1]×[0..1] within the lower 58% of the frame.
`laneToScreenY()` maps ny=0 → 42% of screen height, ny=1 → bottom edge.
Adjust `roadTop` if your pipeline uses a different crop.

## Extending the template

### Add text rendering (FreeType)
```cpp
// In hud_renderer.h
#include <ft2build.h>
#include FT_FREETYPE_H
// Add TextRenderer member, call drawText("38 mph", x, y, size) from drawHUD()
```

### Add ego car motion blur
```cpp
// In drawEgoCar(): store last N positions, draw fading quads
```

### Add distance estimation
```cpp
// In drawCars(): estimate distance from box height
// dist_m ≈ (real_car_height_m * focal_length) / box_height_px
```
