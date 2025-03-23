


RGB_LIBRARY=../rpi-rgb-led-matrix/lib/librgbmatrix.a

CFLAGS=-Wall -Wextra -O2
LDFLAGS=-L../rpi-rgb-led-matrix/lib
LDLIBS=-lrgbmatrix -lm -lstdc++ -lrt

all: basic.exe

basic/main.o: basic/main.c ../src/clay_renderer_matrix.c

basic.exe: basic/main.o ../src/clay_renderer_matrix.c $(RGB_LIBRARY)
	$(LINK.c) -o basic.exe basic/main.o $(LDFLAGS) $(LDLIBS)

.PHONY: all clean
clean:
	rm -f basic/*.o basic.exe

$(RGB_LIBRARY):
	@$(MAKE) -C ../rpi-rgb-led-matrix

