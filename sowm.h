#pragma once
#include <X11/Xlib.h>
#include <stdlib.h>

#define MAX_MONITORS 8
#define WS_INIT_CAP  10

#define win (client *t = 0, *c = list; c && t != list->prev; t = c, c = c->next)

#define canvas_to_screen(val, pan) ((val) - (pan))
#define screen_to_canvas(val, pan) ((val) + (pan))

#define ws_save(W) ws_list[W] = list
#define ws_sel(W)  list = ws_list[ws = (W)]

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(v, lo, hi) (MAX((lo), MIN((v), (hi))))

#define win_size(W, gx, gy, gw, gh) \
  XGetGeometry(d, W, &(Window){0}, gx, gy, gw, gh, \
               &(unsigned int){0}, &(unsigned int){0})

#define mod_clean(mask) \
  (mask & ~(numlock | LockMask) & \
   (ShiftMask | ControlMask | Mod1Mask | Mod2Mask | Mod3Mask | Mod4Mask | \
    Mod5Mask))

typedef struct {
  const char **com;
  const int    i;
  const Window w;
} Arg;

struct key {
  unsigned int  mod;
  KeySym        keysym;
  void        (*function)(const Arg arg);
  const Arg     arg;
};

typedef struct client {
  struct client *next, *prev;
  int f;
  int wx, wy;
  unsigned int ww, wh;
  int cx, cy;
  int monitor;
  Window w;
} client;

typedef struct {
  client *clients;
  int     pan_x[MAX_MONITORS];
  int     pan_y[MAX_MONITORS];
} workspace;

void button_press(XEvent *e);
void button_release(XEvent *e);
void configure_request(XEvent *e);
void input_grab(Window root);
void key_press(XEvent *e);
void map_request(XEvent *e);
void mapping_notify(XEvent *e);
void notify_destroy(XEvent *e);
void notify_enter(XEvent *e);
void notify_motion(XEvent *e);
void run(const Arg arg);
void win_add(Window w);
void win_center(const Arg arg);
void win_del(Window w);
void win_fs(const Arg arg);
void win_focus(client *c);
void win_kill(const Arg arg);
void win_prev(const Arg arg);
void win_next(const Arg arg);
void win_round_corners(Window w, int rad);
void win_to_ws(const Arg arg);
void ws_go(const Arg arg);
void ws_switch(int i);
void ws_focusnext(const Arg arg);
void move_nextmon(const Arg arg);

int  ws_create(void);
void ws_kill_empty(void);
void ws_go_pan(const Arg arg);

void canvas_pan(int mon, int dx, int dy);
void canvas_pan_key(const Arg arg);
void canvas_reset(const Arg arg);
void canvas_apply_all(void);
void canvas_focus(client *c);

int  mon_at_ptr(void);
int  mon_at_win(Window w);

static int xerror() { return 0; }
