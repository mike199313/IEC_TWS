EXE     = cpldupdate-i2c
CFILE  = *.cpp
DEPPKGS = gio-unix-2.0 glib-2.0
CC      ?= $(CROSS_COMPILE)g++
CFLAGS += -std=c++17
CFLAGS += $(shell pkg-config --cflags $(DEPPKG))
LIBS += $(shell pkg-config --libs $(LIBPKGS))
LIBS += -lsystemd -lsdbusplus -lobmc-i2c

all: $(EXE)
$(EXE): $(CFILE)
	$(CC) -g  $(CFLAGS) $(INCLUDES) -g -o $@ $^ -lstdc++ $(LDFLAGS)

clean:
	rm -f $(EXE) *.o *.d
