// Copyright © 2020 Mateus Carmo Martins de Freitas Barbosa
//
// This program is licensed under the GNU General Public License, version 3.
// See LICENSE.txt.
//

#ifndef STACKTILE_CURSOR_H
#define STACKTILE_CURSOR_H

#include <wayland-server-core.h>

struct Server;

enum CursorMode {
    STACKTILE_CURSOR_PASSTHROUGH,
    STACKTILE_CURSOR_MOVE,
    STACKTILE_CURSOR_RESIZE,
};

void handle_new_pointer(Server *server,
                        wlr_input_device *device);

void handle_cursor_axis(wl_listener *listener, void *data);
void handle_cursor_frame(wl_listener *listener, void *data);
void handle_cursor_motion(wl_listener *listener, void *data);
void handle_cursor_motion_absolute(wl_listener *listener, void *data);
void handle_cursor_button(wl_listener *listener, void *data);

#endif /* STACKTILE_CURSOR_H */

