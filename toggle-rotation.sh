#!/bin/bash
# Toggle GPD4Rotate auto-rotation. Bind this from hyprland.lua:
#   hl.bind("SUPER + SHIFT + R", hl.dsp.exec_cmd(os.getenv("HOME") .. "/.config/hypr/scripts/toggle-rotation.sh"))
set -euo pipefail

TOGGLE_FILE="${HOME}/.config/hypr/rotation-toggle"
mkdir -p "$(dirname "$TOGGLE_FILE")"
[[ -f "$TOGGLE_FILE" ]] || echo "1" > "$TOGGLE_FILE"

if [[ "$(tr -d '[:space:]' < "$TOGGLE_FILE")" == "1" ]]; then
    echo "0" > "$TOGGLE_FILE"
    notify-send "GPD Pocket 4" "Auto-rotation disabled" 2>/dev/null || true
else
    echo "1" > "$TOGGLE_FILE"
    notify-send "GPD Pocket 4" "Auto-rotation enabled" 2>/dev/null || true
fi
