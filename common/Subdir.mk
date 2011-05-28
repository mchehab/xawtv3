

OBJS-common-capture := \
	common/sound.o \
	common/webcam.o \
	common/frequencies.o \
	common/commands.o \
	common/parseconfig.o \
	common/capture.o \
	common/event.o \
	libng/libng.a

OBJS-common-input := \
	common/lirc.o \
	common/joystick.o \
	common/midictrl.o

common/channel-no-x11.o: CFLAGS += -DNO_X11=1

ifeq ($(FOUND_ALSA)$(FOUND_V4L2UTIL),yesyes)
  OBJS-common-alsa := common/alsa_stream.o
else
  OBJS-common-alsa :=
endif

common/channel-no-x11.o: common/channel.c
	@$(echo_compile_c)
	@$(compile_c)
	@$(fixup_deps)
