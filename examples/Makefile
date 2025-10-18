RGB_SOURCE=../submodules/rpi-rgb-led-matrix
RGB_LIB=$(RGB_SOURCE)/lib
RGB_LIBRARY=$(RGB_LIB)/librgbmatrix.a

CFLAGS=-Wall -Wextra -Wno-unknown-pragmas -O2
LDFLAGS=-L$(RGB_LIB)
LDLIBS=-lrgbmatrix -lm -lstdc++ -lrt

all: basic.exe

SRCS=$(wildcard ../src/*.c)

basic/main.o: basic/main.c $(SRCS)

basic.exe: basic/main.o $(RGB_LIBRARY)
	$(LINK.c) -o basic.exe basic/main.o $(LDFLAGS) $(LDLIBS)

.PHONY: all clean
clean:
	rm -f basic/*.o basic.exe

$(RGB_LIBRARY):
	@$(MAKE) -C $(RGB_SOURCE)

