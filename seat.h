// Copyright © 2020 Mateus Carmo Martins de Freitas Barbosa
//
// This program is licensed under the GNU General Public License, version 3.
// See LICENSE.txt.
//

#ifndef STACKTILE_SEAT_H
#define STACKTILE_SEAT_H

#include <wayland-server-core.h>

void handle_new_input(wl_listener *listener, void *data);
void seat_handle_request_cursor(wl_listener *listener, void *data);
void seat_handle_request_set_selection(wl_listener *listener, void *data);

#endif /* STACKTILE_SEAT_H */
