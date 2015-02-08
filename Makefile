CFLAGS += -std=c99 -pedantic -Wall -Wextra
PREFIX = /usr/local

BIN = plaintext
OBJ = plaintext.o util.o

$(BIN): $(OBJ)

all: $(BIN)

clean:
	rm -f $(BIN) $(OBJ)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $(BIN) $(DESTDIR)$(PREFIX)/bin/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(BIN)
