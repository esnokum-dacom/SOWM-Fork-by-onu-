#ifndef CONFIG_H
#define CONFIG_H

#include "sowm.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#define MOD Mod4Mask
#define PAN_STEP 120
#define ROUND_CORNERS 0
#define ZOOM_STEP     1.1f
#define ZOOM_STEP_OUT 0.9f
#define ZOOM_MIN      0.125f
#define ZOOM_MAX      4.0f

const char *menu[] = {"dmenu_run", 0};
const char *mterm[] = {"tabbed", "-r", "2", "st", "-w", "[]", 0};
const char *term[] = {"st", 0};
const char *xmenu[] = {"XMenu", 0};
const char *scrot[] = {"sh", "-c", "scrot -s - | xclip -selection clipboard -t image/png", 0};
const char *briup[] = {"bri", "10", "+", 0};
const char *bridown[] = {"bri", "10", "-", 0};
const char *voldown[] = {"amixer", "sset", "Master", "5%-", 0};
const char *volup[] = {"amixer", "sset", "Master", "5%+", 0};
const char *volmute[] = {"amixer", "sset", "Master", "toggle", 0};
const char *colors[] = {"bud", "/home/onu/Wallpapers/", 0};

static struct key keys[] = {
    {MOD | ShiftMask, XK_c, win_kill, {0}},
    {MOD, XK_c, win_center, {0}},
    {MOD, XK_f, win_fs, {0}},

    {MOD, XK_period, ws_focusnext, {0}},
    {MOD | ShiftMask, XK_period, move_nextmon, {0}},

    {Mod1Mask, XK_Tab, win_next, {0}},
    {Mod1Mask | ShiftMask, XK_Tab, win_prev, {0}},

    {MOD, XK_p, run, {.com = menu}},
    {MOD, XK_m, run, {.com = xmenu}},
    {MOD | ShiftMask, XK_s, run, {.com = scrot}},
    // {MOD | ShiftMask, XK_Return, run, {.com = term}},
    {MOD | ShiftMask, XK_Return, run, {.com = mterm}},

    {0, XF86XK_AudioLowerVolume, run, {.com = voldown}},
    {0, XF86XK_AudioRaiseVolume, run, {.com = volup}},
    {0, XF86XK_AudioMute, run, {.com = volmute}},
    {0, XF86XK_MonBrightnessUp, run, {.com = briup}},
    {0, XF86XK_MonBrightnessDown, run, {.com = bridown}},

    {MOD | ShiftMask, XK_Left, canvas_pan_key, {.i = 0}},
    {MOD | ShiftMask, XK_Right, canvas_pan_key, {.i = 1}},
    {MOD | ShiftMask, XK_Up, canvas_pan_key, {.i = 2}},
    {MOD | ShiftMask, XK_Down, canvas_pan_key, {.i = 3}},

    // Reset canvas pan and zoom to origin
    {MOD | ShiftMask, XK_Home, canvas_reset, {0}},

    // {MOD, XK_1, ws_go, {.i = 1}},
    // {MOD | ShiftMask, XK_1, win_to_ws, {.i = 1}},
    // {MOD, XK_2, ws_go, {.i = 2}},
    // {MOD | ShiftMask, XK_2, win_to_ws, {.i = 2}},
    // {MOD, XK_3, ws_go, {.i = 3}},
    // {MOD | ShiftMask, XK_3, win_to_ws, {.i = 3}},
    // {MOD, XK_4, ws_go, {.i = 4}},
    // {MOD | ShiftMask, XK_4, win_to_ws, {.i = 4}},
    // {MOD, XK_5, ws_go, {.i = 5}},
    // {MOD | ShiftMask, XK_5, win_to_ws, {.i = 5}},
    // {MOD, XK_6, ws_go, {.i = 6}},
    // {MOD | ShiftMask, XK_6, win_to_ws, {.i = 6}},

    {MOD, XK_1, ws_go_pan, {.i = 1}},
    {MOD, XK_2, ws_go_pan, {.i = 2}},
    {MOD, XK_3, ws_go_pan, {.i = 3}},
    {MOD, XK_4, ws_go_pan, {.i = 4}},
    {MOD, XK_5, ws_go_pan, {.i = 5}},
    {MOD, XK_6, ws_go_pan, {.i = 6}},
    {MOD, XK_7, ws_go_pan, {.i = 7}},
    {MOD, XK_8, ws_go_pan, {.i = 8}},
    {MOD, XK_9, ws_go_pan, {.i = 9}},
};

#endif
