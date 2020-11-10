// Copyright © 2020 Mateus Carmo Martins de Freitas Barbosa
//
// This program is licensed under the GNU General Public License, version 3.
// See LICENSE.txt.
//

#ifndef STACKTILE_VIEW_H
#define STACKTILE_VIEW_H

#include <wayland-server-core.h>

struct wlr_xdg_surface;
struct wlr_surface;
struct Server;

struct View {
    wl_list link;
    Server *server;
    wlr_xdg_surface *xdg_surface;
    wl_listener map;
    wl_listener unmap;
    wl_listener destroy;
    wl_listener request_move;
    wl_listener request_resize;
    bool mapped;
    int x, y;
};

void focus_view(View *view, wlr_surface *surface);

View *desktop_view_at(Server *server,
                      double lx, double ly,
                      wlr_surface **surface,
                      double *sx, double *sy);

void handle_new_xdg_surface(wl_listener *listener, void *data);

#endif /* STACKTILE_VIEW_H */
