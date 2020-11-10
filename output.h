// Copyright © 2020 Mateus Carmo Martins de Freitas Barbosa
//
// This program is licensed under the GNU General Public License, version 3.
// See LICENSE.txt.
//

#ifndef STACKTILE_OUTPUT_H
#define STACKTILE_OUTPUT_H

struct Server;

struct Output {
    wl_list link;
    Server *server;
    wlr_output *output;
    wl_listener frame;
};

void handle_new_output(wl_listener *listener, void *data);

#endif /* STACKTILE_OUTPUT_H */
