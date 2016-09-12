CC = g++

FLAGS := $(FLAGS) --std=c++11

#Controls wheter graphics are active
# Can be true or false, VPR overrides this value in it's makefile
ENABLE_GRAPHICS = false

ifneq (,$(findstring true, $(ENABLE_GRAPHICS)))
  # The following block defines the graphics library directories. If X11 library
  # or fontconfig is located elsewhere, change it here.
  ifneq (,$(findstring true, $(MAC_OS)))
	#Mac
    LIB_DIR := $(LIB_DIR) -L/usr/X11/lib -L/opt/X11/lib
  else  
	#Unix-like
    LIB_DIR := $(LIB_DIR) -L/usr/lib/X11
    PACKAGEINSTALL := if cat /etc/issue | grep Ubuntu -c >>/dev/null; then if ! dpkg -l | grep libx11-dev -c >>/dev/null; then sudo apt-get install libx11-dev; fi; fi;
    PACKAGEINSTALL := if cat /etc/issue | grep Ubuntu -c >>/dev/null; then if ! dpkg -l | grep libxft-dev -c >>/dev/null; then sudo apt-get install libxft-dev; fi; fi;
    PACKAGENOTIFICATION :=                        if cat /etc/issue | grep Ubuntu -c >>/dev/null; then if ! dpkg -l | grep libx11-dev -c >>/dev/null; then printf "\n\n\n\n*****************************************************\n* VPR has detected that graphics are enabled,       *\n* but the required graphics package libx11-dev      *\n* is missing. Try:                                  *\n* a) Type 'sudo apt-get install libx11-dev' to      *\n*    install manually.                              *\n* b) If libx11-dev is installed, point the Makefile *\n*    to where your X11 libraries are installed.     *\n* c) If you wish to run VPR without graphics, set   *\n*    the flag ENABLE_GRAPHICS = false at the top    *\n*    of the Makefile in VPR's parent directory.     *\n*****************************************************\n\n\n\n"; fi; fi;
    PACKAGENOTIFICATION := $(PACKAGENOTIFICATION) if cat /etc/issue | grep Ubuntu -c >>/dev/null; then if ! dpkg -l | grep libxft-dev -c >>/dev/null; then printf "\n\n\n\n*****************************************************\n* VPR has detected that graphics are enabled,       *\n* but the required graphics package libxft-dev      *\n* is missing. Try:                                  *\n* a) Type 'sudo apt-get install libxft-dev' to      *\n*    install manually.                              *\n* b) If libxft-dev is installed, point the Makefile *\n*    to where your X11 libraries are installed.     *\n* c) If you wish to run VPR without graphics, set   *\n*    the flag ENABLE_GRAPHICS = false at the top    *\n*    of the Makefile in VPR's parent directory.     *\n*****************************************************\n\n\n\n"; fi; fi;
  endif
  LIB := $(LIB) -lX11 -lXft -lfontconfig
  FLAGS := $(FLAGS) $(shell pkg-config --cflags freetype2)
else
  FLAGS := $(FLAGS) -DNO_GRAPHICS
endif

SRC = src/graphics.c
OBJS = $(SRC:.c=.o)
DEPS = $(OBJS:.o=.d)


all: libeasygl.a

libeasygl.a: $(OBJS)
	ar rcs $@ $^


%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(OBJS) $(DEPS) libeasygl.a
