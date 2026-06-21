#include <X11/XF86keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/shape.h>
#include <X11/keysym.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sowm.h"

static client    *list      = NULL;
static client    *cur       = NULL;
static client   **ws_list   = NULL;
static workspace *workspaces= NULL;
static int        ws_cap    = 0;
static int        ws_count  = 0;
static int        ws        = 0;

static int sw, sh;
static int wx, wy;
static unsigned int ww, wh;

static int pan_active   = 0;
static int pan_start_x  = 0;
static int pan_start_y  = 0;
static int pan_origin_x = 0;
static int pan_origin_y = 0;
static int pan_mon      = 0;

static int numlock = 0;

static Display     *d;
static XButtonEvent mouse;
static Window       root;

static void (*events[LASTEvent])(XEvent *e);

#include "config.h"

static void (*events[LASTEvent])(XEvent *e) = {
    [ButtonPress]      = button_press,
    [ButtonRelease]    = button_release,
    [ConfigureRequest] = configure_request,
    [KeyPress]         = key_press,
    [MapRequest]       = map_request,
    [MappingNotify]    = mapping_notify,
    [DestroyNotify]    = notify_destroy,
    [EnterNotify]      = notify_enter,
    [MotionNotify]     = notify_motion,
};

int mon_at_ptr(void) {
    int px, py, di;
    unsigned int du;
    Window dw;
    XQueryPointer(d, root, &dw, &dw, &px, &py, &di, &di, &du);

    int n;
    XineramaScreenInfo *info = XineramaQueryScreens(d, &n);
    if (!info) return 0;

    int mon = 0;
    for (int i = 0; i < n && i < MAX_MONITORS; i++) {
        if (px >= info[i].x_org && px < info[i].x_org + info[i].width &&
            py >= info[i].y_org && py < info[i].y_org + info[i].height) {
            mon = i;
            break;
        }
    }
    XFree(info);
    return mon;
}

int mon_at_win(Window w) {
    int wx2, wy2;
    unsigned int ww2, wh2;
    win_size(w, &wx2, &wy2, &ww2, &wh2);

    int cx = wx2 + (int)ww2 / 2;
    int cy = wy2 + (int)wh2 / 2;

    int n;
    XineramaScreenInfo *info = XineramaQueryScreens(d, &n);
    if (!info) return 0;

    int mon = 0;
    for (int i = 0; i < n && i < MAX_MONITORS; i++) {
        if (cx >= info[i].x_org && cx < info[i].x_org + info[i].width &&
            cy >= info[i].y_org && cy < info[i].y_org + info[i].height) {
            mon = i;
            break;
        }
    }
    XFree(info);
    return mon;
}

static void ws_alloc_arrays(void) {
    ws_cap     = WS_INIT_CAP;
    ws_list    = (client **)  calloc(ws_cap, sizeof(client *));
    workspaces = (workspace *) calloc(ws_cap, sizeof(workspace));
    if (!ws_list || !workspaces) exit(1);
    ws_count = 2;
}

static void ws_grow(void) {
    int new_cap = ws_cap * 2;
    client    **nwl = (client **)   realloc(ws_list,    new_cap * sizeof(client *));
    workspace  *nwp = (workspace *) realloc(workspaces, new_cap * sizeof(workspace));
    if (!nwl || !nwp) return;
    ws_list    = nwl;
    workspaces = nwp;
    memset(ws_list    + ws_cap, 0, (new_cap - ws_cap) * sizeof(client *));
    memset(workspaces + ws_cap, 0, (new_cap - ws_cap) * sizeof(workspace));
    ws_cap = new_cap;
}

int ws_create(void) {
    if (ws_count >= ws_cap) ws_grow();
    int idx = ws_count++;
    ws_list[idx]            = NULL;
    workspaces[idx].clients = NULL;
    memset(workspaces[idx].pan_x, 0, sizeof(workspaces[idx].pan_x));
    memset(workspaces[idx].pan_y, 0, sizeof(workspaces[idx].pan_y));
    long n = (long)ws_count;
    XChangeProperty(d, root,
                    XInternAtom(d, "_NET_NUMBER_OF_DESKTOPS", False),
                    XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&n, 1);
    return idx;
}

void ws_kill_empty(void) {
    if (ws_count <= 1 || list != NULL || ws != ws_count - 1) return;
    ws_count--;
    int prev = ws_count - 1;
    ws_save(ws);
    ws_sel(prev);
    ws_switch(prev);
    if (list) win_focus(list);
}

void ws_go_pan(const Arg arg) {
    int target = arg.i;
    if (target < 0) return;
    if (target == ws_count) ws_create();
    else if (target >= ws_count) return;
    ws_go((Arg){.i = target});
}

void canvas_apply_all(void) {
    for win {
        int m  = c->monitor;
        int px = workspaces[ws].pan_x[m];
        int py = workspaces[ws].pan_y[m];
        XMoveWindow(d, c->w,
                    canvas_to_screen(c->cx, px),
                    canvas_to_screen(c->cy, py));
    }
    XFlush(d);
}

void canvas_pan(int mon, int dx, int dy) {
    if (mon < 0 || mon >= MAX_MONITORS) return;
    workspaces[ws].pan_x[mon] += dx;
    workspaces[ws].pan_y[mon] += dy;
    canvas_apply_all();
}

void canvas_pan_key(const Arg arg) {
    int mon  = mon_at_ptr();
    int step = PAN_STEP;
    switch (arg.i) {
        case 0: canvas_pan(mon, -step, 0);  break;
        case 1: canvas_pan(mon,  step, 0);  break;
        case 2: canvas_pan(mon, 0, -step);  break;
        case 3: canvas_pan(mon, 0,  step);  break;
    }
}

void canvas_reset(const Arg arg) {
    int mon = mon_at_ptr();
    workspaces[ws].pan_x[mon] = 0;
    workspaces[ws].pan_y[mon] = 0;
    canvas_apply_all();
}

void win_focus(client *c) {
    cur = c;
    XSetInputFocus(d, cur->w, RevertToParent, CurrentTime);
    XChangeProperty(d, root,
                    XInternAtom(d, "_NET_ACTIVE_WINDOW", False),
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)&cur->w, 1);
}

void notify_destroy(XEvent *e) {
    win_del(e->xdestroywindow.window);
    if (list)
        win_focus(list->prev);
    else {
        cur = NULL;
        XSetInputFocus(d, root, RevertToPointerRoot, CurrentTime);
    }
}

void notify_enter(XEvent *e) {
    while (XCheckTypedEvent(d, EnterNotify, e))
        ;
    for win
        if (c->w == e->xcrossing.window)
            win_focus(c);
}

void notify_motion(XEvent *e) {
    while (XCheckTypedEvent(d, MotionNotify, e))
        ;

    if (pan_active) {
        int dx = e->xbutton.x_root - pan_start_x;
        int dy = e->xbutton.y_root - pan_start_y;
        workspaces[ws].pan_x[pan_mon] = pan_origin_x - dx;
        workspaces[ws].pan_y[pan_mon] = pan_origin_y - dy;
        canvas_apply_all();
        return;
    }

    if (!mouse.subwindow || (cur && cur->f))
        return;

    int xd = e->xbutton.x_root - mouse.x_root;
    int yd = e->xbutton.y_root - mouse.y_root;

    if (mouse.button == 1) {
        int new_sx = wx + xd;
        int new_sy = wy + yd;
        XMoveWindow(d, mouse.subwindow, new_sx, new_sy);
        if (cur) {
            cur->monitor = mon_at_win(cur->w);
            int m = cur->monitor;
            cur->cx = screen_to_canvas(new_sx, workspaces[ws].pan_x[m]);
            cur->cy = screen_to_canvas(new_sy, workspaces[ws].pan_y[m]);
        }
    } else if (mouse.button == 3) {
        XResizeWindow(d, mouse.subwindow,
                      MAX(1, ww + xd),
                      MAX(1, wh + yd));
        win_round_corners(mouse.subwindow, ROUND_CORNERS);
    }
}

void key_press(XEvent *e) {
    KeySym keysym = XkbKeycodeToKeysym(d, e->xkey.keycode, 0, 0);
    for (unsigned int i = 0; i < sizeof(keys) / sizeof(*keys); ++i)
        if (keys[i].keysym == keysym &&
            mod_clean(keys[i].mod) == mod_clean(e->xkey.state))
            keys[i].function(keys[i].arg);
}

void button_press(XEvent *e) {
    if (e->xbutton.button == 2) {
        pan_active   = 1;
        pan_mon      = mon_at_ptr();
        pan_start_x  = e->xbutton.x_root;
        pan_start_y  = e->xbutton.y_root;
        pan_origin_x = workspaces[ws].pan_x[pan_mon];
        pan_origin_y = workspaces[ws].pan_y[pan_mon];
        return;
    }
    if (!e->xbutton.subwindow) return;
    win_size(e->xbutton.subwindow, &wx, &wy, &ww, &wh);
    XRaiseWindow(d, e->xbutton.subwindow);
    mouse = e->xbutton;
}

void button_release(XEvent *e) {
    if (e->xbutton.button == 2) { pan_active = 0; return; }
    mouse.subwindow = 0;
}

void win_add(Window w) {
    client *c;
    if (!(c = (client *)calloc(1, sizeof(client)))) exit(1);

    c->w       = w;
    c->monitor = mon_at_win(w);

    int sx = 0, sy = 0;
    unsigned int dw, dh;
    win_size(w, &sx, &sy, &dw, &dh);
    int m  = c->monitor;
    c->cx  = screen_to_canvas(sx, workspaces[ws].pan_x[m]);
    c->cy  = screen_to_canvas(sy, workspaces[ws].pan_y[m]);

    if (list) {
        list->prev->next = c;
        c->prev          = list->prev;
        list->prev       = c;
        c->next          = list;
    } else {
        list = c;
        list->prev = list->next = list;
    }

    ws_save(ws);
    workspaces[ws].clients = list;
}

void win_del(Window w) {
    client *x = NULL;
    for win if (c->w == w) x = c;
    if (!list || !x) return;
    if (x->prev == x) list = NULL;
    if (list == x)    list = x->next;
    if (x->next) x->next->prev = x->prev;
    if (x->prev) x->prev->next = x->next;
    free(x);
    ws_save(ws);
    workspaces[ws].clients = list;
}

void win_kill(const Arg arg) {
    if (!cur) return;
    Atom *protocols;
    int   n;
    Atom  wm_delete = XInternAtom(d, "WM_DELETE_WINDOW", False);
    Atom  wm_protos = XInternAtom(d, "WM_PROTOCOLS",     False);
    if (XGetWMProtocols(d, cur->w, &protocols, &n)) {
        int ok = 0;
        for (int i = 0; i < n; i++) if (protocols[i] == wm_delete) { ok = 1; break; }
        XFree(protocols);
        if (ok) {
            XEvent ev = {.xclient = {
                .type = ClientMessage, .window = cur->w,
                .message_type = wm_protos, .format = 32,
                .data.l[0] = wm_delete, .data.l[1] = CurrentTime,
            }};
            XSendEvent(d, cur->w, False, NoEventMask, &ev);
            return;
        }
    }
    XKillClient(d, cur->w);
}

void win_center(const Arg arg) {
    if (!cur) return;
    win_size(cur->w, &(int){0}, &(int){0}, &ww, &wh);

    int px, py, di;
    unsigned int du;
    Window dw;
    XQueryPointer(d, root, &dw, &dw, &px, &py, &di, &di, &du);

    int n;
    XineramaScreenInfo *info = XineramaQueryScreens(d, &n);
    int mx = 0, my = 0, mw = sw, mh = sh;
    if (info) {
        for (int i = 0; i < n; i++) {
            if (px >= info[i].x_org && px < info[i].x_org + info[i].width &&
                py >= info[i].y_org && py < info[i].y_org + info[i].height) {
                mx = info[i].x_org; my = info[i].y_org;
                mw = info[i].width; mh = info[i].height;
                break;
            }
        }
        XFree(info);
    }

    int sx = mx + (mw - (int)ww) / 2;
    int sy = my + (mh - (int)wh) / 2;
    XMoveWindow(d, cur->w, sx, sy);

    cur->monitor = mon_at_win(cur->w);
    int m = cur->monitor;
    cur->cx = screen_to_canvas(sx, workspaces[ws].pan_x[m]);
    cur->cy = screen_to_canvas(sy, workspaces[ws].pan_y[m]);
}

void win_fs(const Arg arg) {
    if (!cur) return;
    if (!cur->f)
        win_size(cur->w, &cur->wx, &cur->wy, &cur->ww, &cur->wh);

    int px, py, di;
    unsigned int du;
    Window dw;
    XQueryPointer(d, root, &dw, &dw, &px, &py, &di, &di, &du);

    int n;
    XineramaScreenInfo *info = XineramaQueryScreens(d, &n);
    int mx = 0, my = 0, mw = sw, mh = sh;
    if (info) {
        for (int i = 0; i < n; i++) {
            if (px >= info[i].x_org && px < info[i].x_org + info[i].width &&
                py >= info[i].y_org && py < info[i].y_org + info[i].height) {
                mx = info[i].x_org; my = info[i].y_org;
                mw = info[i].width; mh = info[i].height;
                break;
            }
        }
        XFree(info);
    }

    if ((cur->f = cur->f ? 0 : 1))
        XMoveResizeWindow(d, cur->w, mx, my, mw, mh);
    else
        XMoveResizeWindow(d, cur->w, cur->wx, cur->wy, cur->ww, cur->wh);

    win_round_corners(cur->w, cur->f ? 0 : ROUND_CORNERS);
}

void win_to_ws(const Arg arg) {
    int tmp = ws;
    if (arg.i == tmp || arg.i < 0 || arg.i >= ws_count) return;
    ws_sel(arg.i);
    win_add(cur->w);
    ws_save(arg.i);
    ws_sel(tmp);
    win_del(cur->w);
    XUnmapWindow(d, cur->w);
    ws_save(tmp);
    ws_switch(tmp);
    if (list) win_focus(list);
}

void canvas_focus(client *c) {
    if (!c) return;

    int m = c->monitor;

    int n;
    XineramaScreenInfo *info = XineramaQueryScreens(d, &n);
    if (!info) {
        win_focus(c);
        return;
    }

    int mx = 0, my = 0, mw = sw, mh = sh;
    if (m < n) {
        mx = info[m].x_org;
        my = info[m].y_org;
        mw = info[m].width;
        mh = info[m].height;
    }
    XFree(info);

    unsigned int cw, ch;
    win_size(c->w, &(int){0}, &(int){0}, &cw, &ch);

    int target_sx = mx + (mw - (int)cw) / 2;
    int target_sy = my + (mh - (int)ch) / 2;

    workspaces[ws].pan_x[m] = c->cx - target_sx;
    workspaces[ws].pan_y[m] = c->cy - target_sy;

    canvas_apply_all();
    XRaiseWindow(d, c->w);
    win_focus(c);
}

void win_prev(const Arg arg) {
    if (!cur) return;
    canvas_focus(cur->prev);
}

void win_next(const Arg arg) {
    if (!cur) return;
    canvas_focus(cur->next);
}

void ws_switch(int i) {
    ws   = i;
    list = ws_list[i];
    workspaces[i].clients = list;
    long data[1] = {(long)i};
    XChangeProperty(d, root,
                    XInternAtom(d, "_NET_CURRENT_DESKTOP", False),
                    XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)data, 1);
}

void ws_go(const Arg arg) {
    int tmp = ws;
    if (arg.i == ws || arg.i < 0 || arg.i >= ws_count) return;
    ws_save(ws);
    ws_sel(arg.i);
    for win XMapWindow(d, c->w);
    ws_sel(tmp);
    for win XUnmapWindow(d, c->w);
    ws_sel(arg.i);
    ws_switch(arg.i);
    canvas_apply_all();
    if (list)
        win_focus(list);
    else {
        cur = NULL;
        XSetInputFocus(d, root, RevertToPointerRoot, CurrentTime);
        XFlush(d);
    }
}

void ws_focusnext(const Arg arg) {
    int px, py, di;
    unsigned int du;
    Window dw;
    XQueryPointer(d, root, &dw, &dw, &px, &py, &di, &di, &du);

    int n;
    XineramaScreenInfo *info = XineramaQueryScreens(d, &n);
    if (!info || n < 2) { XFree(info); return; }

    int cur_mon = 0;
    for (int i = 0; i < n; i++) {
        if (px >= info[i].x_org && px < info[i].x_org + info[i].width &&
            py >= info[i].y_org && py < info[i].y_org + info[i].height) {
            cur_mon = i; break;
        }
    }
    int next = (cur_mon + 1) % n;
    XWarpPointer(d, None, root, 0, 0, 0, 0,
                 info[next].x_org + info[next].width  / 2,
                 info[next].y_org + info[next].height / 2);
    XFree(info);
    XFlush(d);
}

void move_nextmon(const Arg arg) {
    int px, py, di;
    unsigned int du;
    Window dw;
    XQueryPointer(d, root, &dw, &dw, &px, &py, &di, &di, &du);

    int n;
    XineramaScreenInfo *info = XineramaQueryScreens(d, &n);
    if (!info || n < 2) { XFree(info); return; }

    int cur_mon = -1;
    for (int i = 0; i < n; i++) {
        if (px >= info[i].x_org && px < info[i].x_org + info[i].width &&
            py >= info[i].y_org && py < info[i].y_org + info[i].height) {
            cur_mon = i; break;
        }
    }
    if (cur_mon == -1) { XFree(info); return; }

    int next = (cur_mon + 1) % n;
    XWarpPointer(d, None, root, 0, 0, 0, 0,
                 info[next].x_org + info[next].width  / 2,
                 info[next].y_org + info[next].height / 2);

    if (cur) {
        unsigned int cw, ch, bw, depth;
        int cwx, cwy;
        Window rr;
        XGetGeometry(d, cur->w, &rr, &cwx, &cwy, &cw, &ch, &bw, &depth);
        int new_sx = info[next].x_org + (info[next].width  - (int)cw) / 2;
        int new_sy = info[next].y_org + (info[next].height - (int)ch) / 2;
        XMoveWindow(d, cur->w, new_sx, new_sy);
        cur->monitor = next;
        cur->cx = screen_to_canvas(new_sx, workspaces[ws].pan_x[next]);
        cur->cy = screen_to_canvas(new_sy, workspaces[ws].pan_y[next]);
    }

    XFree(info);
    XFlush(d);
}

void win_round_corners(Window w, int rad) {
    unsigned int rw, rh;
    unsigned int dia = 2 * (unsigned int)rad;
    win_size(w, &(int){0}, &(int){0}, &rw, &rh);
    if (rw < dia || rh < dia) return;
    Pixmap mask = XCreatePixmap(d, w, rw, rh, 1);
    if (!mask) return;
    XGCValues xgcv;
    GC shape_gc = XCreateGC(d, mask, 0, &xgcv);
    if (!shape_gc) { XFreePixmap(d, mask); return; }
    XSetForeground(d, shape_gc, 0);
    XFillRectangle(d, mask, shape_gc, 0, 0, rw, rh);
    XSetForeground(d, shape_gc, 1);
    XFillArc(d, mask, shape_gc, 0,        0,        dia, dia, 0, 23040);
    XFillArc(d, mask, shape_gc, rw-dia-1, 0,        dia, dia, 0, 23040);
    XFillArc(d, mask, shape_gc, 0,        rh-dia-1, dia, dia, 0, 23040);
    XFillArc(d, mask, shape_gc, rw-dia-1, rh-dia-1, dia, dia, 0, 23040);
    XFillRectangle(d, mask, shape_gc, rad, 0,   rw - dia, rh);
    XFillRectangle(d, mask, shape_gc, 0,   rad, rw,       rh - dia);
    XShapeCombineMask(d, w, ShapeBounding, 0, 0, mask, ShapeSet);
    XFreePixmap(d, mask);
    XFreeGC(d, shape_gc);
}

void configure_request(XEvent *e) {
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XConfigureWindow(d, ev->window, ev->value_mask,
                     &(XWindowChanges){
                         .x = ev->x, .y = ev->y,
                         .width = ev->width, .height = ev->height,
                         .sibling = ev->above, .stack_mode = ev->detail,
                     });
    win_round_corners(ev->window, ROUND_CORNERS);
}

void map_request(XEvent *e) {
    Window w = e->xmaprequest.window;
    XSelectInput(d, w, StructureNotifyMask | EnterWindowMask);
    win_size(w, &wx, &wy, &ww, &wh);
    win_add(w);
    cur = list->prev;
    if (wx + wy == 0) win_center((Arg){0});
    {
        int sx = 0, sy = 0;
        unsigned int dw2, dh2;
        win_size(w, &sx, &sy, &dw2, &dh2);
        cur->monitor = mon_at_win(w);
        int m = cur->monitor;
        cur->cx = screen_to_canvas(sx, workspaces[ws].pan_x[m]);
        cur->cy = screen_to_canvas(sy, workspaces[ws].pan_y[m]);
    }
    win_round_corners(w, ROUND_CORNERS);
    XMapWindow(d, w);
    win_focus(list->prev);
}

void mapping_notify(XEvent *e) {
    XMappingEvent *ev = &e->xmapping;
    if (ev->request == MappingKeyboard || ev->request == MappingModifier) {
        XRefreshKeyboardMapping(ev);
        input_grab(root);
    }
}

void run(const Arg arg) {
    if (fork()) return;
    if (d) close(ConnectionNumber(d));
    setsid();
    execvp((char *)arg.com[0], (char **)arg.com);
}

void input_grab(Window root) {
    unsigned int i, j;
    unsigned int modifiers[] = {0, LockMask, numlock, numlock | LockMask};
    XModifierKeymap *modmap  = XGetModifierMapping(d);
    KeyCode code;

    for (i = 0; i < 8; i++)
        for (int k = 0; k < modmap->max_keypermod; k++)
            if (modmap->modifiermap[i * modmap->max_keypermod + k] ==
                XKeysymToKeycode(d, 0xff7f))
                numlock = (1 << i);

    XUngrabKey(d, AnyKey, AnyModifier, root);

    for (i = 0; i < sizeof(keys) / sizeof(*keys); i++)
        if ((code = XKeysymToKeycode(d, keys[i].keysym)))
            for (j = 0; j < sizeof(modifiers) / sizeof(*modifiers); j++)
                XGrabKey(d, code, keys[i].mod | modifiers[j], root, True,
                         GrabModeAsync, GrabModeAsync);

    for (i = 1; i < 4; i++)
        for (j = 0; j < sizeof(modifiers) / sizeof(*modifiers); j++)
            XGrabButton(d, i, MOD | modifiers[j], root, True,
                        ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                        GrabModeAsync, GrabModeAsync, 0, 0);

    XFreeModifiermap(modmap);
}

int main(void) {
    XEvent ev;
    if (!(d = XOpenDisplay(0))) exit(1);
    signal(SIGCHLD, SIG_IGN);
    XSetErrorHandler(xerror);

    int s = DefaultScreen(d);
    root  = RootWindow(d, s);
    sw    = XDisplayWidth(d, s);
    sh    = XDisplayHeight(d, s);

    ws_alloc_arrays();
    ws = 1;

    long n = (long)ws_count;
    XChangeProperty(d, root,
                    XInternAtom(d, "_NET_NUMBER_OF_DESKTOPS", False),
                    XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&n, 1);

    XSelectInput(d, root, SubstructureRedirectMask);
    XDefineCursor(d, root, XCreateFontCursor(d, 68));
    input_grab(root);

    while (1 && !XNextEvent(d, &ev))
        if (events[ev.type])
            events[ev.type](&ev);
}
