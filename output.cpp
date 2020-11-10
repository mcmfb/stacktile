// Copyright © 2020 Mateus Carmo Martins de Freitas Barbosa
//
// This program is licensed under the GNU General Public License, version 3.
// See LICENSE.txt.
//

#include <wayland-server-core.h>

extern "C" {
#define static

#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_xdg_shell.h>

#undef static
}

#include "output.h"
#include "server.h"
#include "view.h"

struct RenderData {
    wlr_output *output;
    wlr_renderer *renderer;
    View *view;
    timespec *when;
};

static void render_surface(wlr_surface *surface,
                           int sx, int sy,
                           void *data) {
    auto rdata = reinterpret_cast<RenderData*>(data);
    View *view = rdata->view;
    wlr_output *output = rdata->output;

    wlr_texture *texture = wlr_surface_get_texture(surface);
    if (texture == NULL) {
        return;
    }

    // The view has a position in layout coordinates.
    // We need to translate that to output-local coordinates.
    //
    double ox = 0, oy = 0;
    wlr_output_layout_output_coords(view->server->output_layout, output, &ox, &oy);
    ox += view->x + sx, oy += view->y + sy;

    wlr_box box {
        static_cast<int>(ox * output->scale),
        static_cast<int>(oy * output->scale),
        static_cast<int>(surface->current.width * output->scale),
        static_cast<int>(surface->current.height * output->scale),
    };

    float matrix[9];
    enum wl_output_transform transform = wlr_output_transform_invert(
        surface->current.transform
    );
    wlr_matrix_project_box(
        matrix,
        &box,
        transform,
        0,
        output->transform_matrix
    );

    wlr_render_texture_with_matrix(rdata->renderer, texture, matrix, 1);

    wlr_surface_send_frame_done(surface, rdata->when);
}

static void output_frame(wl_listener *listener, void *data) {
    Output *output = wl_container_of(listener, output, frame);
    wlr_renderer *renderer = output->server->renderer;

    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (!wlr_output_attach_render(output->output, NULL)) {
        return;
    }
    // The "effective" resolution can change if you rotate your outputs.
    int width, height;
    wlr_output_effective_resolution(output->output, &width, &height);

    wlr_renderer_begin(renderer, width, height);

    float color[4] = {0.3, 0.3, 0.3, 1.0};
    wlr_renderer_clear(renderer, color);

    // Because our view list is ordered front-to-back, we iterate over it backwards.
    View *view;
    wl_list_for_each_reverse(view, &output->server->views, link) {
        if (!view->mapped) {
            continue;
        }
        RenderData rdata {
            output->output,
            renderer,
            view,
            &now,
        };
        wlr_xdg_surface_for_each_surface(
            view->xdg_surface,
            render_surface,
            &rdata
        );
    }

    // this function is a no-op when hardware cursors are in use.
    wlr_output_render_software_cursors(output->output, NULL);

    wlr_renderer_end(renderer);
    wlr_output_commit(output->output);
}

void handle_new_output(wl_listener *listener, void *data) {
    Server *server = wl_container_of(listener, server, new_output);
    auto _wlr_output = reinterpret_cast<wlr_output*>(data);

    if (!wl_list_empty(&_wlr_output->modes)) {
        wlr_output_mode *mode = wlr_output_preferred_mode(_wlr_output);
        wlr_output_set_mode(_wlr_output, mode);
        wlr_output_enable(_wlr_output, true);
        if (!wlr_output_commit(_wlr_output)) {
            return;
        }
    }

    Output *output = new Output;
    output->output = _wlr_output;
    output->server = server;
    output->frame.notify = output_frame;
    wl_signal_add(&_wlr_output->events.frame, &output->frame);
    wl_list_insert(&server->outputs, &output->link);

    // The add_auto function arranges outputs from left-to-right in the order
    // they appear. A more sophisticated compositor would let the user configure
    // the arrangement of outputs in the layout.
    wlr_output_layout_add_auto(server->output_layout, _wlr_output);
}
