-- GPD Pocket 4 snippet for Hyprland 0.55+ (Lua config provider).
-- Drop these blocks into ~/.config/hypr/hyprland.lua, or save this file as
-- ~/.config/hypr/gpd-pocket4.lua and `require("gpd-pocket4")` from hyprland.lua.
--
-- The panel is physically portrait 1600x2560. Laptop mode is transform 3.
-- Touchscreen name from `hyprctl devices`: nvtk0603:00-0603:f001

local monitor = "eDP-1"
local mode = "1600x2560@144"
local scale = 2
local laptop_transform = 3
local touch = "nvtk0603:00-0603:f001"

hl.monitor({
    output = monitor,
    mode = mode,
    position = "0x0",
    scale = scale,
    transform = laptop_transform,
})

hl.config({
    input = {
        touchdevice = {
            transform = laptop_transform,
            output = monitor,
        },
        tablet = {
            transform = laptop_transform,
            output = monitor,
        },
    },
})

hl.device({
    name = touch,
    transform = laptop_transform,
})

-- Auto-rotation daemon. Install GPD4Rotate to /usr/local/bin first.
hl.on("hyprland.start", function()
    hl.exec_cmd("GPD4Rotate")
end)

-- SUPER+R is commonly bound to a launcher on Omarchy; SHIFT avoids the clash.
hl.bind("SUPER + SHIFT + R", hl.dsp.exec_cmd(os.getenv("HOME") .. "/.config/hypr/scripts/toggle-rotation.sh"))
