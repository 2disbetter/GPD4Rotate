# GPD4Rotate

Rotation daemon for the **GPD Pocket 4** on **Hyprland 0.55+** (Lua config provider).

This tree is an update of [2disbetter/GPD4Rotate](https://github.com/2disbetter/GPD4Rotate). The original binary talked to Hyprland with `hyprctl keyword`, which **does not apply when the config provider is Lua**. Runtime changes now go through `hyprctl eval` / `hyprctl --batch` and the Lua API (`hl.monitor`, `hl.config`, `hl.device`).

> Modified from the upstream GPD4Rotate sources for Hyprland 0.55+ Lua. Original license terms are unchanged; see `LICENSE`.

## Why this update exists

Hyprland 0.55 replaced `hyprland.conf` (hyprlang) with `~/.config/hypr/hyprland.lua`. Debian's `hyprctl(1)` is explicit:

> `keyword` — Set a config keyword dynamically. **This will not work if your config provider is lua (refer to eval).**

So the upstream `setOrientation()` calls:

```text
hyprctl keyword monitor "eDP-1,1600x2560@144,0x0,2,transform,3"
hyprctl keyword input:touchdevice:transform 3
hyprctl keyword input:tablet:transform 3
```

are ignored on a Lua session. This build issues the equivalent Lua instead:

```text
hyprctl --batch "eval hl.monitor({ output = 'eDP-1', mode = '1600x2560@144', position = '0x0', scale = 2.0, transform = 3 }) ; eval hl.config({ input = { touchdevice = { transform = 3, output = 'eDP-1' }, tablet = { transform = 3, output = 'eDP-1' } } }) ; eval hl.device({ name = 'nvtk0603:00-0603:f001', transform = 3 })"
```

## GPD Pocket 4 notes

| Piece | Value |
| --- | --- |
| Panel | Physically portrait **1600×2560** |
| Typical output | `eDP-1` |
| Laptop / keyboard-toward-user | Hyprland **transform 3** |
| Touchscreen | `nvtk0603:00-0603:f001` (`NVTK0603:00 0603:F001`) |
| Accelerometer | MXC6655 via `iio-sensor-proxy` |

Sensor → transform mapping (unchanged from upstream, Pocket 4 specific):

| `monitor-sensor` orientation | Transform |
| --- | --- |
| `normal` | 3 (laptop) |
| `right-up` | 2 |
| `left-up` | 0 |
| `bottom-up` | 1 |

On start the daemon now applies an orientation immediately (sensor reading, or transform 3 if the sensor is still `undefined`). You no longer have to tilt the device once to get a usable landscape desktop.

## Dependencies

```bash
# Arch / Omarchy
sudo pacman -S cmake gcc dbus iio-sensor-proxy
```

Enable the sensor proxy if it is not already running:

```bash
sudo systemctl enable --now iio-sensor-proxy.service
```

If auto-rotation is inverted, the Pocket 4 accel mount matrix sometimes needs a hwdb override (same approach as Pocket 3):

```text
# /etc/udev/hwdb.d/61-sensor-local.hwdb
sensor:modalias:acpi:MXC6655*:dmi:*:svnGPD:pnG1628-04:*
ACCEL_MOUNT_MATRIX=-1, 0, 0; 0, 1, 0; 0, 0, 1
```

```bash
sudo systemd-hwdb update
sudo udevadm trigger -v -p DEVNAME=/dev/iio:device0
sudo systemctl restart iio-sensor-proxy
```

Confirm `monitor-sensor` prints `orientation changed: …` when you tilt the lid.

## Build and install

```bash
cmake -S . -B build
cmake --build build
sudo install -Dm755 build/GPD4Rotate /usr/local/bin/GPD4Rotate
```

Install the toggle script (path the Lua bind below expects):

```bash
install -Dm755 scripts/toggle-rotation.sh ~/.config/hypr/scripts/toggle-rotation.sh
```

## Hyprland Lua config

Do **not** add `bind =` / `exec-once =` lines to `hyprland.conf`. Those are hyprlang. Put the following in `~/.config/hypr/hyprland.lua`, or `require()` the ready-made snippet:

```lua
require("gpd-pocket4")  -- if you copied examples/hyprland-gpd-pocket4.lua next to hyprland.lua
```

Minimum inline version:

```lua
-- Static laptop orientation so the first frame is already landscape.
hl.monitor({
    output = "eDP-1",
    mode = "1600x2560@144",
    position = "0x0",
    scale = 2,
    transform = 3,
})

hl.config({
    input = {
        touchdevice = { transform = 3, output = "eDP-1" },
        tablet      = { transform = 3, output = "eDP-1" },
    },
})

hl.device({
    name = "nvtk0603:00-0603:f001",
    transform = 3,
})

hl.on("hyprland.start", function()
    hl.exec_cmd("GPD4Rotate")
end)

-- SUPER+R is often the launcher on Omarchy; SHIFT avoids the clash.
hl.bind("SUPER + SHIFT + R", hl.dsp.exec_cmd(os.getenv("HOME") .. "/.config/hypr/scripts/toggle-rotation.sh"))
```

Confirm the touchscreen name with `hyprctl devices` if yours differs. Refresh rate is `1600x2560@144` on the 144 Hz panel; drop it to `@60` if `hyprctl monitors` reports 60 Hz.

A full annotated copy lives in `examples/hyprland-gpd-pocket4.lua`.

## Runtime files

Created automatically under `~/.config/hypr/` on first run:

| File | Purpose |
| --- | --- |
| `rotation-toggle` | `1` = follow the accelerometer, `0` = leave the current transform alone |
| `scale` | Monitor scale used on every rotate. Default `2.0`. Write a float (`1.5`, `2.0`, …) and restart the daemon to pick it up |

Optional hook, if present and executable:

```text
~/.config/hypr/scripts/gpd4rotate-hook.sh <transform>
```

See `scripts/gpd4rotate-hook.example.sh`. Use it to restart `lisgd` (or anything else whose axes depend on rotation). An example gesture script is in `examples/touchscreen-gestures.sh`.

## What changed versus upstream

- `setOrientation()` uses `hyprctl --batch` + `hl.monitor` / `hl.config` / `hl.device` instead of `hyprctl keyword`.
- Per-device transform is applied to `nvtk0603:00-0603:f001`, matching the ArchWiki Pocket 4 Hyprland note.
- Initial orientation is applied at process start.
- Scale is read from `~/.config/hypr/scale` (same idea as FW12Rotate).
- README and examples target `hyprland.lua`, not `hyprland.conf`.
- Optional post-rotate hook for gesture tools.

The toggle-file + `inotify` + `monitor-sensor` loop is the same design as upstream.

## Troubleshooting

1. `hyprctl version` should be **0.55 or newer**.
2. `hyprctl status` / the rolling log should show the **lua** config provider. If you still have only `hyprland.conf`, this binary is the wrong tool — use upstream, or migrate the config.
3. Test a single rotate by hand:

   ```bash
   hyprctl eval 'hl.monitor({ output = "eDP-1", mode = "1600x2560@144", position = "0x0", scale = 2, transform = 3 })'
   ```

   If that fails, the daemon cannot work either.
4. `hyprctl devices` must list the touchscreen. If the name is not `nvtk0603:00-0603:f001`, change `TOUCH_DEVICE` in `gpd4rotate.cpp` and rebuild.
5. If the image rotates but the finger map does not, the per-device `hl.device` name is wrong or the global `input.touchdevice.transform` did not stick — check the `hyprctl eval` output.
6. Loud / early fan spin is a firmware curve, not this daemon. Fn + the fan key on the Pocket 4 toggles a quieter BIOS profile.
