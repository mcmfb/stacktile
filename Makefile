WAYLAND_PROTOCOLS := $(shell pkg-config --variable=pkgdatadir wayland-protocols)
WAYLAND_SCANNER := $(shell pkg-config --variable=wayland_scanner wayland-scanner)
INCLUDE := \
	 $(shell pkg-config --cflags wlroots) \
	 $(shell pkg-config --cflags wayland-server) \
	 $(shell pkg-config --cflags xkbcommon)
LIBS := \
	 $(shell pkg-config --libs wlroots) \
	 $(shell pkg-config --libs wayland-server) \
	 $(shell pkg-config --libs xkbcommon)

OBJS := cursor.o keyboard.o output.o seat.o server.o view.o

xdg-shell-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		$(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

xdg-shell-protocol.c: xdg-shell-protocol.h
	$(WAYLAND_SCANNER) private-code \
		$(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

$(OBJS): %.o: %.cpp xdg-shell-protocol.h
	$(CXX) $(CXXFLAGS) -c -g -Werror \
		$(INCLUDE) -I. \
		-DWLR_USE_UNSTABLE \
		-o $@ $<

xdg-shell-protocol.o: xdg-shell-protocol.c xdg-shell-protocol.h
	$(CC) $(CFLAGS) -c -g -Werror \
		$(INCLUDE) -I. \
		-DWLR_USE_UNSTABLE \
		-o $@ $<

stacktile: $(OBJS) xdg-shell-protocol.o
	$(CXX) $(CXXFLAGS) \
		-o $@ $^ \
		$(LIBS)

clean:
	rm -f stacktile xdg-shell-protocol.h xdg-shell-protocol.c xdg-shell-protocol.o $(OBJS)

.DEFAULT_GOAL=stacktile
.PHONY: clean
