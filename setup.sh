#!/usr/bin/env bash

# ==========================================================================
# Deployment Tool: AetherRain Terminal Splash Configuration & Installer
# Target: Optimized login / boot splash hook for Niri WM & Display Managers
# ==========================================================================

set -e

SRC_PATH="aetherrain.c"
BIN_DEST="/usr/local/bin/aetherrain"

if [ ! -f "$SRC_PATH" ]; then
    echo "[-] Error: Source asset ($SRC_PATH) not found in current directory."
    exit 1
fi

get_current_val() {
    grep "#define $1" "$SRC_PATH" | awk '{print $3}'
}
get_current_int() {
    grep "int $1 =" "$SRC_PATH" | sed -E 's/.*=\s*([0-9]+);.*/\1/'
}

CURR_BUILD=$(get_current_val "BUILD_DURATION_SEC")
CURR_HOLD=$(get_current_val "HOLD_DURATION_SEC")
CURR_OUTRO=$(get_current_val "OUTRO_DURATION_SEC")
CURR_SPEED=$(get_current_int "SPEED_MODE")
CURR_DENSITY=$(get_current_int "DENSITY_MODE")
CURR_H_EFF=$(get_current_int "HOLD_EFFECT_MODE")
CURR_O_EFF=$(get_current_int "OUTRO_EFFECT_MODE")

echo "=================================================="
echo "          AetherRain Engine Installer             "
echo "=================================================="
if [ -f "$BIN_DEST" ]; then
    echo "[*] Notice: Existing binary detected at $BIN_DEST"
    echo "    Modifying config will patch the source and re-compile."
else
    echo "[*] Notice: Fresh installation initialization."
fi
echo "=================================================="

# Profile Core Selection
echo "Select Configuration Profile:"
echo "1) Default System Profile (3s Build, 2s Combined Hold/Outro)"
echo "2) Custom Manual Parameters"
read -rp "Selection (1-2) [Default: 1]: " prof_choice
prof_choice=${prof_choice:-1}

if [ "$prof_choice" -eq 1 ]; then
    build_t="3.0"
    hold_t="1.2"
    outro_t="0.8"
    speed_s="2"
    density_s="2"
    h_eff_s="1"
    o_eff_s="4"
    color_p="1"
else
    echo "--------------------------------------------------"
    read -rp "Text Generation Build Time (seconds) [Current: ${CURR_BUILD}s]: " build_t
    build_t=${build_t:-$CURR_BUILD}

    read -rp "Text Decrypted Hold Time (seconds) [Current: ${CURR_HOLD}s]: " hold_t
    hold_t=${hold_t:-$CURR_HOLD}

    read -rp "Outro Dynamic Animation Time (seconds) [Current: ${CURR_OUTRO}s]: " outro_t
    outro_t=${outro_t:-$CURR_OUTRO}

    echo "--------------------------------------------------"
    echo "Select Simulation Velocity [Current: $CURR_SPEED]:"
    echo "1) Low Velocity  2) Balanced Stream  3) High Velocity"
    read -rp "Selection (1-3): " speed_s
    speed_s=${speed_s:-$CURR_SPEED}

    echo "--------------------------------------------------"
    echo "Select Screen Rain Density [Current: $CURR_DENSITY]:"
    echo "1) Light Matrix  2) Balanced Layer  3) Extreme Overload"
    read -rp "Selection (1-3): " density_s
    density_s=${density_s:-$CURR_DENSITY}

    echo "--------------------------------------------------"
    echo "Select Theme Preset / Color Configuration:"
    echo "1) Matrix Emerald (Classic Green Layering)"
    echo "2) Phosphor Amber (Retro Warm CRT)"
    echo "3) Cyber Cobalt (Deep Neon Blue Layout)"
    echo "4) Crimson Alert (Security System Red)"
    echo "5) Pure Monochrome (White Rain / Gray Text)"
    echo "6) Custom Matrix (Specify Custom Text & Rain RGB)"
    read -rp "Selection (1-6): " color_p
    color_p=${color_p:-1}

    echo "--------------------------------------------------"
    echo "Select Steady-State Hold Effect [Current: $CURR_H_EFF]:"
    echo "1) Active Glitch  2) Neon Pulse  3) Wave Shimmer  4) Binary Flip  5) Frozen"
    read -rp "Selection (1-5): " h_eff_s
    h_eff_s=${h_eff_s:-$CURR_H_EFF}

    echo "--------------------------------------------------"
    echo "Select Outro Dissolve Effect [Current: $CURR_O_EFF]:"
    echo "1) Hard Cut  2) Dissolve Melt  3) Anti-Grav  4) Kinetic Scatter  5) EMP Burn  6) Compression"
    read -rp "Selection (1-6): " o_eff_s
    o_eff_s=${o_eff_s:-$CURR_O_EFF}
fi

# Apply Micro-adjustments and Durations to Source
sed -i -E "s/(#define BUILD_DURATION_SEC )[0-9.]+(\s*)/\1$build_t\2/" "$SRC_PATH"
sed -i -E "s/(#define HOLD_DURATION_SEC )[0-9.]+(\s*)/\1$hold_t\2/" "$SRC_PATH"
sed -i -E "s/(#define OUTRO_DURATION_SEC )[0-9.]+(\s*)/\1$outro_t\2/" "$SRC_PATH"

sed -i -E "s/(int SPEED_MODE\s*=\s*)[0-9]+;/\1$speed_s;/" "$SRC_PATH"
sed -i -E "s/(int DENSITY_MODE\s*=\s*)[0-9]+;/\1$density_s;/" "$SRC_PATH"
sed -i -E "s/(int HOLD_EFFECT_MODE\s*=\s*)[0-9]+;/\1$h_eff_s;/" "$SRC_PATH"
sed -i -E "s/(int OUTRO_EFFECT_MODE\s*=\s*)[0-9]+;/\1$o_eff_s;/" "$SRC_PATH"

# Color Dynamic Injections
apply_color_vectors() {
    sed -i -E "s/(short RGB_TEXT\[3\]\s*=\s*\{)[0-9 ,]+(\};)/\1$1\2/" "$SRC_PATH"
    sed -i -E "s/(short RGB_SHADOW\[3\]\s*=\s*\{)[0-9 ,]+(\};)/\1$2\2/" "$SRC_PATH"
    sed -i -E "s/(short RGB_Z0\[3\]\s*=\s*\{)[0-9 ,]+(\};)/\1$3\2/" "$SRC_PATH"
    sed -i -E "s/(short RGB_Z1\[3\]\s*=\s*\{)[0-9 ,]+(\};)/\1$4\2/" "$SRC_PATH"
    sed -i -E "s/(short RGB_Z2\[3\]\s*=\s*\{)[0-9 ,]+(\};)/\1$5\2/" "$SRC_PATH"
    sed -i -E "s/(short RGB_Z3\[3\]\s*=\s*\{)[0-9 ,]+(\};)/\1$6\2/" "$SRC_PATH"
}

case "$color_p" in
    1) # Emerald Green (All layers themed matching environment)
        apply_color_vectors "300, 1000, 300" "0, 150, 0" "700, 1000, 700" "0, 600, 0" "0, 350, 0" "0, 150, 0" ;;
    2) # Phosphor Amber
        apply_color_vectors "1000, 750, 200" "200, 100, 0" "1000, 650, 0" "750, 450, 0" "500, 250, 0" "250, 100, 0" ;;
    3) # Cyber Cobalt Blue
        apply_color_vectors "200, 600, 1000" "0, 100, 300" "500, 800, 1000" "0, 450, 800" "0, 300, 550" "0, 150, 300" ;;
    4) # Crimson Alert
        apply_color_vectors "1000, 200, 200" "200, 0, 0" "1000, 500, 500" "750, 0, 0" "450, 0, 0" "200, 0, 0" ;;
    5) # Monochrome
        apply_color_vectors "1000, 1000, 1000" "200, 200, 200" "900, 900, 900" "600, 600, 600" "400, 400, 400" "200, 200, 200" ;;
    6) # Fully Custom Parameters (Prompt inputs from user)
        echo "Enter RGB Values scaled between 0 and 1000:"
        read -rp "Text String RGB (e.g., 1000, 1000, 0 for Yellow): " cust_txt
        read -rp "Rain Layer 0 Bright Accent RGB: " cust_z0
        read -rp "Rain Layer 1 Standard Fluid RGB: " cust_z1
        read -rp "Rain Layer 2 Mid Depth Dim RGB: " cust_z2
        read -rp "Rain Layer 3 Deep Background RGB: " cust_z3
        apply_color_vectors "$cust_txt" "$cust_z3" "$cust_z0" "$cust_z1" "$cust_z2" "$cust_z3" ;;
esac

echo "[*] Committing compilation pipeline..."
gcc -O3 "$SRC_PATH" -o aetherrain -lncurses

echo "[*] Syncing binary artifact with destination path..."
if [ -w "/usr/local/bin" ]; then
    mv aetherrain "$BIN_DEST"
else
    sudo mv aetherrain "$BIN_DEST"
fi

echo "=================================================="
echo "[+] Configuration Successfully Synchronized: $BIN_DEST"
echo "=================================================="
