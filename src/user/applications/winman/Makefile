
BUILDDIR := build

CXX := ccache g++

CFLAGS:=-I../../../subsys/native/include `freetype-config --cflags` -O2
CFLAGS:=$(CFLAGS) `libpng-config --cflags` `sdl-config --cflags`
CFLAGS:=$(CFLAGS) `pkg-config --cflags cairo pangocairo pango`
CFLAGS:=$(CFLAGS) -I../../libraries/libui/src -I../../libraries/libui/include
CFLAGS:=$(CFLAGS) -DTARGET_LINUX -DPNG_SKIP_SETJMP_CHECK -I.
LIBS:=`freetype-config --libs` `libpng-config --libs` `sdl-config --libs`
LIBS:=$(LIBS) `pkg-config --libs cairo pangocairo pango`
LIBS:=$(LIBS) -lrt

TUI_LIBS:=-L$(BUILDDIR) -lui -lbsd

LIBUI_PATH:=../../libraries/libui/src
TUI_PATH:=../TUI

WINMAN_SRCS:=util-linux.cc Png.cc objects.cc winman.cc
WINMAN_OBJS:=$(patsubst %.cc,$(BUILDDIR)/winman-%.o,$(WINMAN_SRCS))

LIBUI_SRCS:=$(LIBUI_PATH)/Widget.cc
LIBUI_OBJS:=$(patsubst $(LIBUI_PATH)/%.cc,$(BUILDDIR)/libui-%.o,$(LIBUI_SRCS))

TUI_SRCS:=$(TUI_PATH)/main.cc $(TUI_PATH)/Header.cc $(TUI_PATH)/Terminal.cc $(TUI_PATH)/Xterm.cc $(TUI_PATH)/Xterm-colours.cc $(TUI_PATH)/Font.cc $(TUI_PATH)/environment.cc
TUI_OBJS:=$(patsubst $(TUI_PATH)/%.cc,$(BUILDDIR)/tui-%.o,$(TUI_SRCS)) $(BUILDDIR)/winman-util-linux.o

TARGETS:=$(BUILDDIR)/winman $(BUILDDIR)/libui.so $(BUILDDIR)/tui


all: $(BUILDDIR) $(TARGETS)

clean:
	rm -f $(BUILDDIR)/*.o
	rm -f $(TARGETS)


$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(WINMAN_OBJS): $(WINMAN_SRCS)

$(LIBUI_OBJS): $(LIBUI_SRCS)

$(TUI_OBJS): $(TUI_SRCS)

$(BUILDDIR)/winman: $(WINMAN_OBJS)
	$(CXX) $(CFLAGS) -o $@ $+ $(LIBS)

$(BUILDDIR)/libui.so: $(LIBUI_OBJS)
	$(CXX) $(CFLAGS) -shared -o $@ $+

$(BUILDDIR)/tui: $(TUI_OBJS)
	$(CXX) $(CFLAGS) -o $@ $+ $(LIBS) $(TUI_LIBS)

$(BUILDDIR)/winman-%.o: %.cc
	$(CXX) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/libui-%.o: $(LIBUI_PATH)/%.cc
	$(CXX) $(CFLAGS) -fPIC -c -o $@ $<

$(BUILDDIR)/tui-%.o: $(TUI_PATH)/%.cc
	$(CXX) $(CFLAGS) -c -o $@ $<
