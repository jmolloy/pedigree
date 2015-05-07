
BUILDDIR := build

CXX := g++

CFLAGS:=-I../../../subsys/native/include `freetype-config --cflags` -O2
CFLAGS:=$(CFLAGS) `libpng-config --cflags` `sdl-config --cflags`
CFLAGS:=$(CFLAGS) -I../../libraries/libui/src -I../../libraries/libui/include
CFLAGS:=$(CFLAGS) -DTARGET_LINUX -DPNG_SKIP_SETJMP_CHECK
LIBS:=`freetype-config --libs` `libpng-config --libs` `sdl-config --libs` -lcairo
LIBS:=$(LIBS) -lrt

SRCS:=util-linux.cc Png.cc objects.cc winman.cc
OBJS:=$(patsubst %.cc,$(BUILDDIR)/%.o,$(SRCS))


all: $(BUILDDIR) winman


$(BUILDDIR):
	mkdir $(BUILDDIR)

$(OBJS): $(SRCS)

winman: $(OBJS)
	$(CXX) $(CFLAGS) -o $(BUILDDIR)/winman $(OBJS) $(LIBS)

$(BUILDDIR)/%.o: %.cc
	$(CXX) $(CFLAGS) -c -o $@ $<
