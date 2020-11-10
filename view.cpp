// Copyright © 2020 Mateus Carmo Martins de Freitas Barbosa
//
// This program is licensed under the GNU General Public License, version 3.
// See LICENSE.txt.
//

#include <wayland-server-core.h>

extern "C" {
#define static

#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/edges.h>

#undef static
}

#include "server.h"
#include "view.h"

void focus_view(View *view, wlr_surface *surface) {
    // Note: this function only deals with keyboard focus.
    if (view == NULL) {
        return;
    }
    Server *server = view->server;
    wlr_seat *seat = server->seat;
    wlr_surface *prev_surface = seat->keyboard_state.focused_surface;
    if (prev_surface == surface) {
        // Don't re-focus an already focused surface.
        return;
    }
    if (prev_surface) {
        wlr_xdg_surface *previous = wlr_xdg_surface_from_wlr_surface(
            seat->keyboard_state.focused_surface
        );
        wlr_xdg_toplevel_set_activated(previous, false);
    }
    wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);

    // Move the view to the front
    wl_list_remove(&view->link);
    wl_list_insert(&server->views, &view->link);

    wlr_xdg_toplevel_set_activated(view->xdg_surface, true);
    wlr_seat_keyboard_notify_enter(
        seat,
        view->xdg_surface->surface,
        keyboard->keycodes,
        keyboard->num_keycodes,
        &keyboard->modifiers
    );
}

static bool view_at(View *view,
                    double lx, double ly,
                    wlr_surface **surface,
                    double *sx, double *sy) {
    double view_sx = lx - view->x;
    double view_sy = ly - view->y;

    double _sx, _sy;
    wlr_surface *_surface = NULL;
    _surface = wlr_xdg_surface_surface_at(
            view->xdg_surface, view_sx, view_sy, &_sx, &_sy);

    if (_surface != NULL) {
        *sx = _sx;
        *sy = _sy;
        *surface = _surface;
        return true;
    }

    return false;
}

View *desktop_view_at(Server *server,
                      double lx, double ly,
                      wlr_surface **surface,
                      double *sx, double *sy) {
    // This relies on server->views being ordered from top-to-bottom.
    View *view;
    wl_list_for_each(view, &server->views, link) {
        if (view_at(view, lx, ly, surface, sx, sy)) {
            return view;
        }
    }
    return NULL;
}

static void xdg_surface_map(wl_listener *listener, void *data) {
    View *view = wl_container_of(listener, view, map);
    view->mapped = true;
    focus_view(view, view->xdg_surface->surface);
}

static void xdg_surface_unmap(wl_listener *listener, void *data) {
    View *view = wl_container_of(listener, view, unmap);
    view->mapped = false;
}

static void xdg_surface_destroy(wl_listener *listener, void *data) {
    View *view = wl_container_of(listener, view, destroy);
    wl_list_remove(&view->link);
    free(view);
}

static void begin_interactive(View *view,
                              enum CursorMode mode,
                              uint32_t edges) {
    Server *server = view->server;
    wlr_surface *focused_surface = server->seat->pointer_state.focused_surface;
    if (view->xdg_surface->surface != focused_surface) {
        return;
    }
    server->grabbed_view = view;
    server->cursor_mode = mode;

    if (mode == STACKTILE_CURSOR_MOVE) {
        server->grab_x = server->cursor->x - view->x;
        server->grab_y = server->cursor->y - view->y;
    } else {
        wlr_box geo_box;
        wlr_xdg_surface_get_geometry(view->xdg_surface, &geo_box);

        double border_x = (view->x + geo_box.x) + ((edges & WLR_EDGE_RIGHT) ? geo_box.width : 0);
        double border_y = (view->y + geo_box.y) + ((edges & WLR_EDGE_BOTTOM) ? geo_box.height : 0);
        server->grab_x = server->cursor->x - border_x;
        server->grab_y = server->cursor->y - border_y;

        server->grab_geobox = geo_box;
        server->grab_geobox.x += view->x;
        server->grab_geobox.y += view->y;

        server->resize_edges = edges;
    }
}


static void xdg_toplevel_request_move(wl_listener *listener, void *data) {
    // This event is raised when a client would like to begin an interactive
    // move, typically because the user clicked on their client-side
    // Note that a more sophisticated compositor should check the
    // provied serial against a list of button press serials sent to this
    // client, to prevent the client from requesting this whenever they want.
    //
    View *view = wl_container_of(listener, view, request_move);
    begin_interactive(view, STACKTILE_CURSOR_MOVE, 0);
}

static void xdg_toplevel_request_resize(wl_listener *listener, void *data) {
    // This event is raised when a client would like to begin an interactive
    // resize, typically because the user clicked on their client-side
    // decorations. Note that a more sophisticated compositor should check the
    // provied serial against a list of button press serials sent to this
    // client, to prevent the client from requesting this whenever they want.
    //
    auto event = reinterpret_cast<wlr_xdg_toplevel_resize_event*>(data);
    View *view = wl_container_of(listener, view, request_resize);
    begin_interactive(view, STACKTILE_CURSOR_RESIZE, event->edges);
}

void handle_new_xdg_surface(wl_listener *listener, void *data) {
    Server *server = wl_container_of(listener, server, new_xdg_surface);
    auto xdg_surface = reinterpret_cast<wlr_xdg_surface*>(data);
    if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
        return;
    }

    View *view = new View;
    view->server = server;
    view->xdg_surface = xdg_surface;

    view->map.notify = xdg_surface_map;
    wl_signal_add(&xdg_surface->events.map, &view->map);
    view->unmap.notify = xdg_surface_unmap;
    wl_signal_add(&xdg_surface->events.unmap, &view->unmap);
    view->destroy.notify = xdg_surface_destroy;
    wl_signal_add(&xdg_surface->events.destroy, &view->destroy);

    wlr_xdg_toplevel *toplevel = xdg_surface->toplevel;
    view->request_move.notify = xdg_toplevel_request_move;
    wl_signal_add(&toplevel->events.request_move, &view->request_move);
    view->request_resize.notify = xdg_toplevel_request_resize;
    wl_signal_add(&toplevel->events.request_resize, &view->request_resize);

    wl_list_insert(&server->views, &view->link);
}
