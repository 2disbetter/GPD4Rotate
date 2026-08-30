#!/bin/bash
# Optional post-rotate hook. Copy to ~/.config/hypr/scripts/gpd4rotate-hook.sh
# and chmod +x it. GPD4Rotate will invoke it with the new Hyprland transform
# (0-3) after every successful orientation change.
#
# Restart lisgd (or any other gesture tool) so swipe directions stay correct.
set -euo pipefail

TRANSFORM="${1:-3}"
HOOK="$HOME/.config/hypr/scripts/touchscreen-gestures.sh"

if [[ -x "$HOOK" ]]; then
    CALLED_FROM_AUTOROTATE=1 "$HOOK" "$TRANSFORM"
fi
