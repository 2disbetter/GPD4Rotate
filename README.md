# GPD4Rotate
Rotation application for the GPD Pocket 4 and Hyprland via Omarchy Linux, but should be usable on other distros using Hyprland that don't already have plumbing in place for rotation. This uses a toggle-rotation file in your hyprland configuration folder. (~/.config/hypr/) 

The GPD4Rotate file should be moved to /usr/local/bin/ and given permissions to execute. 

This applicaiton is dependent on inotify-tools and iio-sensor-proxy (pacman -S inotify-tools iio-sensor-proxy). The point of this application is to be light weight and allow rotation to just work. 

I currently use a script that is called by my hyprland key combo that sets a toggle-rotation file as either enabled or disabled. 
This is an example of how that script could look: 
```
#!/bin/bash
TOGGLE_FILE="$HOME/.config/hypr/rotation-toggle"

# Toggle between 0 (off) and 1 (on)
if [ "$(cat "$TOGGLE_FILE")" -eq 1 ]; then
    echo "0" > "$TOGGLE_FILE"
    notify-send "Auto-rotation disabled"
else
    echo "1" > "$TOGGLE_FILE"
    notify-send "Auto-rotation enabled"
fi
```

You will need to add a an exec-once command and a key combo line to your hyprland.conf file like this: 

```
bind = SUPER, R, exec, ~/.config/hypr/toggle-rotation.sh
exec-once = GPD4Rotate
```
Restart your ocmputer or relaunch Hyprland. This will allow you to use your key combo to enable and disable rotation. 

