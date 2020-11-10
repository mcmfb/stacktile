// Copyright © 2020 Mateus Carmo Martins de Freitas Barbosa
//
// This program is licensed under the GNU General Public License, version 3.
// See LICENSE.txt.
//

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <wayland-server-core.h>

extern "C" {
#define static

#include <wlr/backend.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

#undef static
}

#include "cursor.h"
#include "keyboard.h"
#include "seat.h"
#include "server.h"
#include "output.h"
#include "view.h"

static bool server_init(Server *server) {
    if (server == NULL) {
        return false;
    }

    server->display = wl_display_create();
    server->backend = wlr_backend_autocreate(server->display, NULL);

    server->renderer = wlr_backend_get_renderer(server->backend);
    wlr_renderer_init_wl_display(server->renderer, server->display);

    wlr_compositor_create(server->display, server->renderer);
    wlr_data_device_manager_create(server->display);

    server->output_layout = wlr_output_layout_create();

    wl_list_init(&server->outputs);
    server->new_output.notify = handle_new_output;
    wl_signal_add(&server->backend->events.new_output, &server->new_output);

    wl_list_init(&server->views);
    server->xdg_shell = wlr_xdg_shell_create(server->display);
    server->new_xdg_surface.notify = handle_new_xdg_surface;
    wl_signal_add(&server->xdg_shell->events.new_surface, &server->new_xdg_surface);

    server->cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(server->cursor, server->output_layout);

    server->cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
    wlr_xcursor_manager_load(server->cursor_mgr, 1);

    server->cursor_motion.notify = handle_cursor_motion;
    wl_signal_add(&server->cursor->events.motion, &server->cursor_motion);

    server->cursor_motion_absolute.notify = handle_cursor_motion_absolute;
    wl_signal_add(&server->cursor->events.motion_absolute, &server->cursor_motion_absolute);

    server->cursor_button.notify = handle_cursor_button;
    wl_signal_add(&server->cursor->events.button, &server->cursor_button);

    server->cursor_axis.notify = handle_cursor_axis;
    wl_signal_add(&server->cursor->events.axis, &server->cursor_axis);

    server->cursor_frame.notify = handle_cursor_frame;
    wl_signal_add(&server->cursor->events.frame, &server->cursor_frame);

    wl_list_init(&server->keyboards);
    server->new_input.notify = handle_new_input;
    wl_signal_add(&server->backend->events.new_input, &server->new_input);

    server->seat = wlr_seat_create(server->display, "seat0");

    server->request_cursor.notify = seat_handle_request_cursor;
    wl_signal_add(&server->seat->events.request_set_cursor, &server->request_cursor);

    server->request_set_selection.notify = seat_handle_request_set_selection;
    wl_signal_add(&server->seat->events.request_set_selection, &server->request_set_selection);

    const char *socket = wl_display_add_socket_auto(server->display);
    if (!socket) {
        wlr_backend_destroy(server->backend);
        return false;
    }

    if (!wlr_backend_start(server->backend)) {
        wlr_backend_destroy(server->backend);
        wl_display_destroy(server->display);
        return false;
    }

    setenv("WAYLAND_DISPLAY", socket, true);
    wlr_log(WLR_INFO, "Initialized Wayland compositor on WAYLAND_DISPLAY=%s", socket);

    return true;
}

int main(int argc, char *argv[]) {
    wlr_log_init(WLR_DEBUG, NULL);
    char *startup_cmd = NULL;

    int c;
    while ((c = getopt(argc, argv, "s:h")) != -1) {
        switch (c) {
        case 's':
            startup_cmd = optarg;
            break;
        default:
            printf("Usage: %s [-s startup command]\n", argv[0]);
            return 0;
        }
    }
    if (optind < argc) {
        printf("Usage: %s [-s startup command]\n", argv[0]);
        return 0;
    }

    Server server;
    if (!server_init(&server)) {
        return 1;
    }
    if (startup_cmd) {
        pid_t child = fork();
        if (child == 0) {
            execl("/bin/sh", "/bin/sh", "-c", startup_cmd, (void *)NULL);
        } else if (child < 0) {
            wlr_log(WLR_ERROR, "failed to fork child process!");
            wl_display_destroy(server.display);
            return 1;
        }
    }
    wl_display_run(server.display);

    wl_display_destroy_clients(server.display);
    wl_display_destroy(server.display);
    return 0;
}
