// Copyright © 2020 Mateus Carmo Martins de Freitas Barbosa
//
// This program is licensed under the GNU General Public License, version 3.
// See LICENSE.txt.
//

#include <wayland-server-core.h>

extern "C" {
#define static

#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_seat.h>

#undef static
}

#include "cursor.h"
#include "keyboard.h"
#include "seat.h"
#include "server.h"

void seat_handle_request_cursor(wl_listener *listener, void *data) {
    Server *server = wl_container_of(listener, server, request_cursor);
    auto event = reinterpret_cast<wlr_seat_pointer_request_set_cursor_event*>(data);
    wlr_seat_client *focused_client = server->seat->pointer_state.focused_client;

    if (focused_client == event->seat_client) {
        wlr_cursor_set_surface(
            server->cursor,
            event->surface,
            event->hotspot_x,
            event->hotspot_y
        );
    }
}

void seat_handle_request_set_selection(wl_listener *listener, void *data) {
    Server *server = wl_container_of(listener, server, request_set_selection);
    auto event = reinterpret_cast<wlr_seat_request_set_selection_event*>(data);

    wlr_seat_set_selection(server->seat, event->source, event->serial);
}

void handle_new_input(wl_listener *listener, void *data) {
    Server *server = wl_container_of(listener, server, new_input);
    auto device = reinterpret_cast<wlr_input_device*>(data);
    switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD:
        handle_new_keyboard(server, device);
        break;
    case WLR_INPUT_DEVICE_POINTER:
        handle_new_pointer(server, device);
        break;
    default:
        break;
    }
    // We always have a cursor, even if there are no pointer devices,
    // so we always include that capability.
    uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
    if (!wl_list_empty(&server->keyboards)) {
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;
    }
    wlr_seat_set_capabilities(server->seat, caps);
}


