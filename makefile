# This version of the makefile seems correctly includes
# header file dependencies, and will re-build
# .o files if any of the included .h files change
# It also assumes that raylib has been cloned to a 
# directory at the same level as the Europi directory
# and that raylib has been compiled with using the 
# following command from within the ../raylib/src directory:
#   For RPi 3 & Earlier: 
#   make PLATFORM=PLATFORM_RPI
#
#   For RPi 4:
#   make PLATFORM=PLATFORM_DRM
# 
# this builds ../raylib/release/rpi/libraylib.a
#
# This Makefile defaults to using PLATFORM=PLATFORM_DRM
# but can be overridden by invoking it using
#
# sudo make PLATFORM=PLATFORM_RPI
#
PLATFORM           ?= PLATFORM_DRM
OBJS := europi.o europi_func1.o europi_func2.o europi_gui.o

ifeq ($(PLATFORM),PLATFORM_DRM)
	INCLUDES = -I. -I../raylib/src -I../raylib/src/external -I/usr/include/libdrm
	LIBS = -lpigpio -lraylib -lGLESv2 -lEGL -lpthread -lrt -lm -lgbm -ldrm -ldl
	LFLAGS = -L. -L../raylib/src -lGLESv2 -lEGL -ldrm -lgbm
	CFLAGS = -O2 -s -Wall -std=gnu99 -fgnu89-inline -Wno-unused-variable -DEGL_NO_X11
	PLATFORM = PLATFORM_DRM
endif
ifeq ($(PLATFORM),PLATFORM_RPI)
	INCLUDES = -I. -I../raylib/src -I../raylib/src/external -I../raylib/release/libs/rpi -I/opt/vc/include -I/opt/vc/include/interface/vcos/pthreads
 	LIBS = -lpigpio -lraylib -lbrcmGLESv2 -lbrcmEGL -lpthread -lrt -lm -lbcm_host -lopenal
	LFLAGS = -L. -L../raylib/src -L../raylib/release/libs/rpi -L/opt/vc/lib
	CFLAGS = -O2 -s -Wall -std=gnu99 -fgnu89-inline -Wno-unused-variable
	PLATFORM = PLATFORM_RPI
endif

ifeq ($(PLATFORM),PLATFORM_DESKTOP)
    GRAPHICS ?= GRAPHICS_API_OPENGL_21
	#CFLAGS = -O1 -s -Wall -std=c99 -Wno-unused-variable
	CFLAGS = -O1 -s -Wall -std=gnu99 -Wno-unused-variable
	INCLUDES = -I/usr/local/include -I. -I../raylib/src -I../raylib/src/external
	LFLAGS = -L. -L/usr/local/lib -L../raylib/src -L../raylib
 	LIBS = -lpigpio -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
	PLATFORM = PLATFORM_DESKTOP
endif


# link
europi: $(OBJS)
	gcc $(OBJS) $(CFLAGS) $(INCLUDES) $(LFLAGS) $(LIBS) -D$(PLATFORM) -o europi 

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

# compile and generate dependency info;
# more complicated dependency computation, so all prereqs listed
# will also become command-less, prereq-less targets
#   sed:    strip the target (everything before colon)
#   sed:    remove any continuation backslashes
#   fmt -1: list words one per line
#   sed:    strip leading spaces
#   sed:    add trailing colons
%.o: %.c
	gcc -c $(CFLAGS) $*.c -o $*.o
	gcc -MM $(CFLAGS) $*.c > $*.d
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

# remove compilation products
clean:
	rm -f europi *.o *.d
	
