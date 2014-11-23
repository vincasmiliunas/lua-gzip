TARGET = gzip.so
PKG_NAME = lua-gzip
CFLAGS += -O2 -fPIC
LDFLAGS += -shared -lz

all: $(TARGET)

$(TARGET): $(PKG_NAME).o
	$(CC) $(LDFLAGS) $< -o $@

$(PKG_NAME).o: $(PKG_NAME).c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *.so

test:
	lua test.lua
