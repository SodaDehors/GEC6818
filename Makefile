CC = arm-linux-gcc
CFLAGS ?= -O2 -std=gnu99 -Wall -Wextra -Wpedantic
CPPFLAGS += -Iinc
LDFLAGS ?= -static
LDLIBS += -lm
SOURCES := main.c $(wildcard src/*.c)
TARGET ?= bin/main

.PHONY: all test preview
all: $(TARGET)

$(TARGET): $(SOURCES) $(wildcard inc/*.h) Makefile
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) $(LDFLAGS) $(LDLIBS) -o $@

test:
	mkdir -p build/tests
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_core.c src/bmp.c src/media.c $(LDLIBS) -o build/tests/test_core
	./build/tests/test_core
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/fake_player.c -o build/tests/fake_player
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_player.c src/player.c $(LDLIBS) -o build/tests/test_player
	./build/tests/test_player
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_event.c $(LDLIBS) -o build/tests/test_event
	./build/tests/test_event

preview:
	mkdir -p build/preview
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/ui_preview.c src/app.c src/bmp.c src/font.c src/media.c src/music.c src/photo.c src/player.c src/ui.c src/video.c $(LDLIBS) -o build/preview/ui_preview
	./build/preview/ui_preview
