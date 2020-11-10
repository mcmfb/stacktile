// Copyright © 2020 Mateus Carmo Martins de Freitas Barbosa
//
// This program is licensed under the GNU General Public License, version 3.
// See LICENSE.txt.
//

#ifndef STACKTILE_SERVER_H
#define STACKTILE_SERVER_H

#include <wayland-server-core.h>
#include "cursor.h"
struct wlr_backend;
struct wlr_renderer;
struct wlr_xdg_shell;
struct wlr_cursor;
struct wlr_xcursor_manager;
struct wlr_seat;
struct wlr_output_layout;
struct View;

struct Server {
    wl_display *display;
    wlr_backend *backend;
    wlr_renderer *renderer;

    wlr_xdg_shell *xdg_shell;
    wl_listener new_xdg_surface;
    wl_list views;

    wlr_cursor *cursor;
    wlr_xcursor_manager *cursor_mgr;
    wl_listener cursor_motion;
    wl_listener cursor_motion_absolute;
    wl_listener cursor_button;
    wl_listener cursor_axis;
    wl_listener cursor_frame;

    wlr_seat *seat;
    wl_listener new_input;
    wl_listener request_cursor;
    wl_listener request_set_selection;
    wl_list keyboards;
    CursorMode cursor_mode;
    View *grabbed_view;
    double grab_x, grab_y;
    wlr_box grab_geobox;
    uint32_t resize_edges;

    wlr_output_layout *output_layout;
    wl_list outputs;
    wl_listener new_output;
};

#endif /* STACKTILE_SERVER_H */
