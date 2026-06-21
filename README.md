# sowm (*~~Simple~~ Shitty Opinionated Window Manager*)

- Floating only.
- Fullscreen toggle (fixed).
- Window centering (fixed).
- Mix of mouse and keyboard workflow.
- Focus with cursor.
- Rounded corners (added)
- change display with key-bind

- Alt-Tab window focusing.
- All windows die on exit.
- No window borders.
- No EWMH.

## Default Keybindings

**Window Management**

| combo                      | action                 |
| -------------------------- | -----------------------|
| `Mouse`                    | focus under cursor     |
| `MOD4` + `Left Mouse`      | move window            |
| `MOD4` + `Right Mouse`     | resize window          |
| `MOD4` + `f`               | maximize toggle        |
| `MOD4` + `c`               | center window          |
| `MOD4` + `Shift` + `c`     | kill window            |
| `MOD4` + `1-6`             | desktop swap           |
| `MOD4` + `Shift` +`1-6`    | send window to desktop |
| `MOD1` + `TAB` (*alt-tab*) | focus cycle            |
| `MOD4` + `Period`          | Go to next monitor     |

**Programs**

| combo                    | action           | program        |
| ------------------------ | ---------------- | -------------- |
| `MOD4` + `Return`        | terminal         | `wezterm`      |
| `MOD4` + `p`             | dmenu            | `dmenu_run`    |
| `MOD4` + `Shift` + `s`   | scrot            | `scr`          |
| `XF86_AudioLowerVolume`  | volume down      | `amixer`       |
| `XF86_AudioRaiseVolume`  | volume up        | `amixer`       |
| `XF86_AudioMute`         | volume toggle    | `amixer`       |
| `XF86_MonBrightnessUp`   | brightness up    | `bri`          |
| `XF86_MonBrightnessDown` | brightness down  | `bri`          |


# DWM (*~~Simple~~ Shitty Opinionated Window Manager*)

- Floating & tiling.
- Fullscreen toggle.
- Mix of mouse and keyboard workflow.
- Focus with cursor.
- All windows die on exit.
- No window borders.
- No EWMH.

## Dependencies

- `xlib` (*usually `libX11`*).
