// Copyright © 2020 Mateus Carmo Martins de Freitas Barbosa
//
// This program is licensed under the GNU General Public License, version 3.
// See LICENSE.txt.
//

#include <wayland-server-core.h>

extern "C" {
#define static

#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>

#undef static
}

#include "cursor.h"
#include "server.h"
#include "view.h"

void handle_new_pointer(Server *server,
                        wlr_input_device *device) {
    wlr_cursor_attach_input_device(server->cursor, device);
}

void handle_cursor_axis(wl_listener *listener, void *data) {
    Server *server = wl_container_of(listener, server, cursor_axis);
    auto event = reinterpret_cast<wlr_event_pointer_axis*>(data);
    wlr_seat_pointer_notify_axis(
        server->seat,
        event->time_msec,
        event->orientation,
        event->delta,
        event->delta_discrete,
        event->source
    );
}

void handle_cursor_frame(wl_listener *listener, void *data) {
    Server *server = wl_container_of(listener, server, cursor_frame);
    wlr_seat_pointer_notify_frame(server->seat);
}

static void process_cursor_move(Server *server, uint32_t time) {
    server->grabbed_view->x = server->cursor->x - server->grab_x;
    server->grabbed_view->y = server->cursor->y - server->grab_y;
}

static void process_cursor_resize(Server *server, uint32_t time) {
    // In a more fleshed-out compositor,
    // you'd wait for the client to prepare a buffer at the new size, then
    // commit any movement that was prepared.
    //
    View *view = server->grabbed_view;
    double border_x = server->cursor->x - server->grab_x;
    double border_y = server->cursor->y - server->grab_y;
    int new_left = server->grab_geobox.x;
    int new_right = server->grab_geobox.x + server->grab_geobox.width;
    int new_top = server->grab_geobox.y;
    int new_bottom = server->grab_geobox.y + server->grab_geobox.height;

    if (server->resize_edges & WLR_EDGE_TOP) {
        new_top = border_y;
        if (new_top >= new_bottom) {
            new_top = new_bottom - 1;
        }
    } else if (server->resize_edges & WLR_EDGE_BOTTOM) {
        new_bottom = border_y;
        if (new_bottom <= new_top) {
            new_bottom = new_top + 1;
        }
    }
    if (server->resize_edges & WLR_EDGE_LEFT) {
        new_left = border_x;
        if (new_left >= new_right) {
            new_left = new_right - 1;
        }
    } else if (server->resize_edges & WLR_EDGE_RIGHT) {
        new_right = border_x;
        if (new_right <= new_left) {
            new_right = new_left + 1;
        }
    }

    wlr_box geo_box;
    wlr_xdg_surface_get_geometry(view->xdg_surface, &geo_box);
    view->x = new_left - geo_box.x;
    view->y = new_top - geo_box.y;

    int new_width = new_right - new_left;
    int new_height = new_bottom - new_top;
    wlr_xdg_toplevel_set_size(view->xdg_surface, new_width, new_height);
}

static void process_cursor_motion(Server *server, uint32_t time) {
    if (server->cursor_mode == STACKTILE_CURSOR_MOVE) {
        process_cursor_move(server, time);
        return;
    } else if (server->cursor_mode == STACKTILE_CURSOR_RESIZE) {
        process_cursor_resize(server, time);
        return;
    }

    double sx, sy;
    wlr_seat *seat = server->seat;
    wlr_surface *surface = NULL;
    View *view = desktop_view_at(
        server,
        server->cursor->x,
        server->cursor->y,
        &surface,
        &sx,
        &sy
    );
    if (!view) {
        wlr_xcursor_manager_set_cursor_image(
                server->cursor_mgr, "left_ptr", server->cursor);
    }
    if (surface) {
        bool focus_changed = seat->pointer_state.focused_surface != surface;
        wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
        if (!focus_changed) {
            wlr_seat_pointer_notify_motion(seat, time, sx, sy);
        }
    } else {
        wlr_seat_pointer_clear_focus(seat);
    }
}

void handle_cursor_motion(wl_listener *listener, void *data) {
    Server *server = wl_container_of(listener, server, cursor_motion);
    auto event = reinterpret_cast<wlr_event_pointer_motion*>(data);

    wlr_cursor_move(
        server->cursor,
        event->device,
        event->delta_x,
        event->delta_y
    );
    process_cursor_motion(server, event->time_msec);
}

void handle_cursor_motion_absolute(wl_listener *listener, void *data) {
    // This event is forwarded by the cursor when a pointer emits an _absolute_
    // motion event, from 0..1 on each axis. This happens, for example, when
    // wlroots is running under a Wayland window rather than KMS+DRM, and you
    // move the mouse over the window. You could enter the window from any edge,
    // so we have to warp the mouse there. There is also some hardware which
    // emits these events.
    //
    Server *server = wl_container_of(listener, server, cursor_motion_absolute);
    auto event = reinterpret_cast<wlr_event_pointer_motion_absolute*>(data);

    wlr_cursor_warp_absolute(server->cursor, event->device, event->x, event->y);
    process_cursor_motion(server, event->time_msec);
}

void handle_cursor_button(wl_listener *listener, void *data) {
    Server *server = wl_container_of(listener, server, cursor_button);
    auto event = reinterpret_cast<wlr_event_pointer_button*>(data);

    wlr_seat_pointer_notify_button(
        server->seat,
        event->time_msec,
        event->button,
        event->state
    );
    double sx, sy;
    wlr_surface *surface;
    View *view = desktop_view_at(
        server,
        server->cursor->x,
        server->cursor->y,
        &surface,
        &sx,
        &sy
    );
    if (event->state == WLR_BUTTON_RELEASED) {
        server->cursor_mode = STACKTILE_CURSOR_PASSTHROUGH;
    } else {
        focus_view(view, surface);
    }
}
