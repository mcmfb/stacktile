// Copyright © 2020 Mateus Carmo Martins de Freitas Barbosa
//
// This program is licensed under the GNU General Public License, version 3.
// See LICENSE.txt.
//

#include <wayland-server-core.h>

extern "C" {
#define static


#include <wlr/types/wlr_box.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <xkbcommon/xkbcommon.h>

#undef static
}

#include "keyboard.h"
#include "server.h"
#include "view.h"

static void keyboard_handle_modifiers(wl_listener *listener, void *data) {
    Keyboard *keyboard = wl_container_of(listener, keyboard, modifiers);
    wlr_seat_set_keyboard(keyboard->server->seat, keyboard->device);
    wlr_seat_keyboard_notify_modifiers(
        keyboard->server->seat,
        &keyboard->device->keyboard->modifiers
    );
}

static bool handle_keybinding(Server *server, xkb_keysym_t sym) {
     // This function assumes the prefix key is held down.
    switch (sym) {
    case XKB_KEY_Escape:
        wl_display_terminate(server->display);
        break;
    case XKB_KEY_F1:
    {
        // Cycle to the next view
        if (wl_list_length(&server->views) < 2) {
            break;
        }
        View *current_view = wl_container_of(
            server->views.next,
            current_view,
            link
        );
        View *next_view = wl_container_of(
            current_view->link.next,
            next_view,
            link
        );
        focus_view(next_view, next_view->xdg_surface->surface);
        // Move the previous view to the end of the list
        wl_list_remove(&current_view->link);
        wl_list_insert(server->views.prev, &current_view->link);
        break;
    }
    default:
        return false;
    }
    return true;
}

static void keyboard_handle_key(wl_listener *listener, void *data) {
    Keyboard *keyboard = wl_container_of(listener, keyboard, key);
    Server *server = keyboard->server;
    auto event = reinterpret_cast<wlr_event_keyboard_key*>(data);
    wlr_seat *seat = server->seat;

    uint32_t keycode = event->keycode + 8;
    const xkb_keysym_t *syms;
    int nsyms = xkb_state_key_get_syms(
        keyboard->device->keyboard->xkb_state,
        keycode,
        &syms
    );

    // TODO: make this configurable.
    enum wlr_keyboard_modifier prefix_key_mask = WLR_MODIFIER_LOGO;

    bool handled = false;
    uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->device->keyboard);
    if ((modifiers & prefix_key_mask) && event->state == WLR_KEY_PRESSED) {
        for (int i = 0; i < nsyms; i++) {
            handled = handle_keybinding(server, syms[i]);
        }
    }

    if (!handled) {
        wlr_seat_set_keyboard(seat, keyboard->device);
        wlr_seat_keyboard_notify_key(
            seat,
            event->time_msec,
            event->keycode,
            event->state
        );
    }
}

void handle_new_keyboard(Server *server,
                         wlr_input_device *device) {
    Keyboard *keyboard = new Keyboard;
    keyboard->server = server;
    keyboard->device = device;

    xkb_rule_names rules = { 0 };
    xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    xkb_keymap *keymap = xkb_map_new_from_names(
        context,
        &rules,
        XKB_KEYMAP_COMPILE_NO_FLAGS
    );

    wlr_keyboard_set_keymap(device->keyboard, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
    wlr_keyboard_set_repeat_info(device->keyboard, 25, 600);

    keyboard->modifiers.notify = keyboard_handle_modifiers;
    wl_signal_add(&device->keyboard->events.modifiers, &keyboard->modifiers);
    keyboard->key.notify = keyboard_handle_key;
    wl_signal_add(&device->keyboard->events.key, &keyboard->key);

    wlr_seat_set_keyboard(server->seat, device);

    wl_list_insert(&server->keyboards, &keyboard->link);
}
