#!/bin/bash
# GPD Pocket 4 touchscreen gestures via lisgd (AUR). Optional companion to
# GPD4Rotate — point the post-rotate hook at this script.
set -euo pipefail

if [[ -z "${CALLED_FROM_AUTOROTATE:-}" ]]; then
    pkill -f "lisgd" 2>/dev/null || true
    sleep 0.3
fi

# Find the NVTK0603 event node if possible; fall back to event10.
DEVICE=""
if command -v libinput >/dev/null 2>&1; then
    DEVICE="$(libinput list-devices 2>/dev/null | awk '
        BEGIN { RS = ""; FS = "\n" }
        /NVTK0603/ {
            for (i = 1; i <= NF; i++) {
                if ($i ~ /Kernel:/) {
                    split($i, a, ":")
                    gsub(/^[ \t]+|[ \t]+$/, "", a[2])
                    print a[2]
                    exit
                }
            }
        }')"
fi
DEVICE="${DEVICE:-/dev/input/event10}"

TRANSFORM="${1:-3}"

WS_PREV="hyprctl dispatch 'hl.dsp.focus({ workspace = \"e-1\" })'"
WS_NEXT="hyprctl dispatch 'hl.dsp.focus({ workspace = \"e+1\" })'"
# Swap this for your overview plugin if you use one (hyprexpo, scrolloverview, …).
OVERVIEW="hyprctl dispatch 'hl.dsp.focus({ workspace = \"overview\" })'"

# Map Hyprland transform -> lisgd -o orientation.
case "$TRANSFORM" in
    3) LISGD_ORIENT=0 ;;  # laptop
    0) LISGD_ORIENT=1 ;;  # portrait left
    1) LISGD_ORIENT=2 ;;  # laptop upside down
    2) LISGD_ORIENT=3 ;;  # portrait right
    *) LISGD_ORIENT=0 ;;
esac

if command -v lisgd >/dev/null 2>&1 && [[ -e "$DEVICE" ]]; then
    pkill -f "lisgd" 2>/dev/null || true
    lisgd -d "$DEVICE" -o "$LISGD_ORIENT" \
        -g "3,LR,*,0.08,$WS_PREV" \
        -g "3,RL,*,0.08,$WS_NEXT" \
        -g "3,UD,*,0.08,$OVERVIEW" \
        -g "3,DU,*,0.08,$OVERVIEW" \
        -t 150 -r 25 &
fi
