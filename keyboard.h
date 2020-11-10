// Copyright © 2020 Mateus Carmo Martins de Freitas Barbosa
//
// This program is licensed under the GNU General Public License, version 3.
// See LICENSE.txt.
//

#ifndef STACKTILE_KEYBOARD_H
#define STACKTILE_KEYBOARD_H

#include <wayland-server-core.h>
struct wlr_input_device;
struct Server;

struct Keyboard {
    wl_list link;
    Server *server;
    wlr_input_device *device;

    wl_listener modifiers;
    wl_listener key;
};

void handle_new_keyboard(Server *server,
                         wlr_input_device *device);

#endif /* STACKTILE_KEYBOARD_H */
