# AetherRain 🌧️✨
A mesmerizing matrix-style splash screen / system welcome animation, built with GTK and Cairo. Optimized for Wayland desktops such as <a href="https://github.com/niri-wm/niri"> Niri </a> and Hyprland.
<div align="center">
  <img src="https://github.com/MR-PR0G/Aetherrain-SplashLoginScreen/blob/main/Demos/DemoAtherrainUi.png" alt="ScreenShot" width="500" style="">
</div>
<a href="https://github.com/MR-PR0G/Aetherrain-SplashLoginScreen/blob/main/Demos/DemoAtherrain.mp4">
You can click for see video
</a>

## 🧠 What is AetherRain?
AetherRain is a full‑screen, interactive splash screen that runs after login – before your desktop environment or application launches.
It creates a “digital rain” effect (Matrix‑style) that gradually reveals a custom text message.
Perfect for system integrators, themers, or anyone wanting a stunning pre‑session animation.
## ✨ Features

  - 🟢 Digital rain with dynamic speed & density

  - 📝 Customizable text (editable from menu)

  - 🎨 Full RGB color picker (presets + manual)

  - ⚡ Visual effects during reveal: Matrix Random, Neon Blink, Horizontal Glitch

  - 🎬 Outro effects for text (fade, gravity fall, flicker) and for rain (fade, chaos burst)

  - ⌨️ Keyboard‑driven TUI menu (no mouse needed)

  - 🖥️ Full‑screen borderless window

  - 🔧 Configurable build / hold timings

  - ❄️ Fast, pure C / GTK4 – lightweight and portable
## 🚀Quick Start & Binary Installation 
If you have downloaded the pre-compiled binary executable you can instantly run the configuration utility and deploy the splash screen into your system without needing any build tools
```bash
# 1. Give executable permissions to the downloaded binary
chmod +x aetherrain
# 2. Launch the interactive TUI configuration panel directly
./aetherrain
```
🎮 How to Use
- Navigation – Arrow keys (← → ↑ ↓)
- Select – Enter
- Quit – Q key
## 📦 System Dependencies
While most modern Linux distributions come pre-equipped with GTK environments, you will need the development headers to compile the binary from source.
For Arch Linux:
```bash
sudo pacman -S gtk4 cairo gcc pkg-config base-devel
```
For Ubuntu:
```bash
sudo apt install libgtk-4-dev libcairo2-dev gcc pkg-config build-essential
```
For Fedora:
```bash
sudo dnf install -y gtk4-devel gtk4-layer-shell cairo-devel gcc pkgconf-pkg-config make automake autoconf gcc-c++
```
## 🛠️ Compilation & Optimization Manual
If you are a developer or want to build the binary manually from the source code, use the following compilation paradigms:
### 1. Developer Control Dashboard Mode
This mode keeps the interactive TUI configuration panel active at the bottom, allowing you to dynamically adjust rain scales, text variables, and colors before deploying:
```bash
gcc -O3 aetherrain.c -o aetherrain $(pkg-config --cflags --libs gtk4) -lm
```
### 2. Standalone System Splash Mode (Production)
This mode compiles the code with the strict internal environment flag (-DPRODUCTION_MODE). It strips away all setup UI interfaces, delivering a pure, ultra-fast, and performance-optimized system splash screen that immediately triggers the matrix animation and exits cleanly:
```bash
gcc -O3 aetherrain.c -o aetherrain-splash $(pkg-config --cflags --libs gtk4) -lm -DPRODUCTION_MODE
```
## 🗃️ Architecture and Configuration Storage
The engine writes configuration profiles persistently in clean unified standard layouts inside the user configuration matrix:
- Storage Location: ~/.config/aetherrain/config.conf
- Target Build Location: ~/.local/bin/aetherrain-splash
- Autostart Configuration: ~/.config/autostart/aetherrain.desktop
