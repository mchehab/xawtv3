
# targets to build
TARGETS-plugins := \
	libng/plugins/flt-gamma.so \
	libng/plugins/flt-invert.so \
	libng/plugins/flt-disor.so \
	libng/plugins/conv-mjpeg.so \
	libng/plugins/read-avi.so \
	libng/plugins/write-avi.so
#	libng/plugins/conv-audio.so
ifeq ($(FOUND_LQT),yes)
TARGETS-plugins += \
	libng/plugins/read-qt.so \
	libng/plugins/write-qt.so
endif
ifeq ($(FOUND_DV),yes)
TARGETS-plugins += \
	libng/plugins/read-dv.so \
	libng/plugins/write-dv.so
endif
ifeq ($(FOUND_OS),linux)
TARGETS-plugins += \
	libng/plugins/drv0-v4l2.so \
	libng/plugins/drv0-v4l2-old.so \
	libng/plugins/drv1-v4l.so \
	libng/plugins/snd-oss.so
endif
ifeq ($(FOUND_OS),bsd)
TARGETS-plugins += \
	libng/plugins/drv0-bsd.so \
	libng/plugins/snd-oss.so
endif

GONE-plugins := \
	$(libdir)/invert.so \
	$(libdir)/nop.so \
	$(libdir)/flt-nop.so

# libraries to link
libng/plugins/read-qt.so  : LDLIBS := $(QT_LIBS)
libng/plugins/write-qt.so : LDLIBS := $(QT_LIBS)
libng/plugins/read-dv.so  : LDLIBS := $(DV_LIBS)
libng/plugins/write-dv.so : LDLIBS := $(DV_LIBS)

# global targets
all:: $(TARGETS-plugins)

install::
	$(INSTALL_DIR) $(libdir)
	$(INSTALL_PROGRAM) -s $(TARGETS-plugins) $(libdir)
	rm -f $(GONE-plugins)

clean::
	rm -f $(TARGETS-plugins)

libng/plugins/conv-mjpeg.so: libng/plugins/conv-mjpeg.o
libng/plugins/drv0-bsd.so:   libng/plugins/drv0-bsd.o
libng/plugins/drv0-v4l2-old.so: libng/plugins/drv0-v4l2-old.o
libng/plugins/flt-debug.so:  libng/plugins/flt-debug.o
libng/plugins/flt-disor.so:  libng/plugins/flt-disor.o
libng/plugins/flt-gamma.so:  libng/plugins/flt-gamma.o
libng/plugins/flt-invert.so: libng/plugins/flt-invert.o
libng/plugins/read-avi.so:   libng/plugins/read-avi.o
libng/plugins/read-dv.so:    libng/plugins/read-dv.o
libng/plugins/read-qt.so:    libng/plugins/read-qt.o
libng/plugins/snd-oss.so:    libng/plugins/snd-oss.o
libng/plugins/write-avi.so:  libng/plugins/write-avi.o
libng/plugins/write-dv.so:   libng/plugins/write-dv.o
libng/plugins/write-qt.so:   libng/plugins/write-qt.o

libng/plugins/drv0-v4l2.so: \
	libng/plugins/drv0-v4l2.o \
	libng/plugins/struct-v4l2.o \
	libng/plugins/struct-dump.o

libng/plugins/drv1-v4l.so: \
	libng/plugins/drv1-v4l.o \
	libng/plugins/struct-v4l.o \
	libng/plugins/struct-dump.o

libng/plugins/struct-dump.o: structs/struct-dump.c
	@$(echo_compile_c)
	@$(compile_c)
	@$(fixup_deps)

libng/plugins/struct-v4l.o: structs/struct-v4l.c
	@$(echo_compile_c)
	@$(compile_c)
	@$(fixup_deps)

libng/plugins/struct-v4l2.o: structs/struct-v4l2.c
	@$(echo_compile_c)
	@$(compile_c)
	@$(fixup_deps)