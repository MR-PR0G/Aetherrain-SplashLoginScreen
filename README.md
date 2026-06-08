# AetherRain (arain)

A lightweight, high-performance, and ultra-precise Matrix-style digital rain splash screen engine written in C using `ncurses`. Specifically designed as a lightning-fast, full-screen boot/login visual hook for modern Linux Desktop Environments and Window Managers like **Niri WM** or Display Managers (**DMS**).

---

## 🌌 The Philosophy Behind the Name

* **Full Name:** AetherRain
* **CLI Command / Short Name:** `arain`

### Why "AetherRain"?
In ancient science and 19th-century physics, **Aether** (Ether) was the invisible, weightless, and omnipresent substance believed to permeate all space, acting as the medium through which light and universal energies propagate. 

Combined with **Rain**, the name signifies a "stream of pure, cosmic digital energy." In this project, the text matrix is completely hidden in the void; it does not simply appear—it physically materializes only when the invisible *Aether streams* pass over it.

---

## ⚡ Features

* **Frame-Accurate Text Synchronization:** Characters are decrypted precisely when a Layer-0 (foreground) rain head intersects the character coordinates.
* **Highly Configurable Engine:** Control generation speed, rain density, and transition modes.
* **Custom Dynamic States:** Features 5 distinct text-holding modes (e.g., Active Glitch, Neon Pulse) and 6 dynamic exit/outro systems (e.g., Kinetic Scatter, EMP Burnout).
* **Automated Patch-Installer:** The interactive script updates configurations directly inside the C source and re-compiles instantly without bloat.

---

## 🛠️ Installation & Configuration

### Prerequisites
Ensure you have `gcc` and the `ncurses` development libraries installed on your system.

```bash
# Ubuntu/Debian
sudo apt install build-essential libncurses5-dev libncursesw5-dev

# Arch Linux
sudo pacman -S base-devel ncurses

# Fedora
sudo dnf groupinstall "Development Tools"
sudo dnf install ncurses-devel

```
Setup Guide

  Clone the repository and navigate into the directory:
  
    git clone [https://github.com/yourusername/aetherrain.git](https://github.com/yourusername/aetherrain.git)
    cd aetherrain
    
  Make the installer executable and run it:
  
    chmod +x setup.sh
    ./setup.sh

Profile Options
 Default Profile: Instantly configures the splash to run optimally within your custom time constraints (3s  text build up, 2s combined hold and exit animation).

 Manual Profile: Allows you to custom-define text generation timings, stream velocities, custom text/rain RGB values, and distinct animation modules.
