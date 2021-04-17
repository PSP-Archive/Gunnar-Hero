TARGET = gunnarhero
OBJS = main.o gunnarhero.o piece.o song.o overlay.o menumode.o mp3player.o tinyxml.o tinyxmlparser.o tinyxmlerror.o ../fishfight/callbacks.o ../fishfight/mesh.o ../fishfight/vram.o ../fishfight/loadpng.o ../fishfight/textureholder.o

CFLAGS = -O2 -G0 -Wall -D_PSP -I../fishfight -g -DFF_TEXTURE_ENABLE -DTIXML_USE_STL
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LIBS = -L/usr/local/pspsdk/lib -lpng -lz -ljpeg -lmad -lfreetype -lmikmod -lmmio -lpspaudiolib -lpspaudio -lstdc++ -lpspgum -lpsprtc -lpspgu -lm -lpsppower
LDFLAGS =

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Gunnar Hero
PSP_EBOOT_ICON = icon0.png
PSP_EBOOT_PIC1 = pic1.png

PSPSDK=$(shell psp-config --pspsdk-path)
CFLAGS += -I$(PSPSDK)/../include/freetype2
PSPBIN = $(PSPSDK)/../bin
include $(PSPSDK)/lib/build.mak

