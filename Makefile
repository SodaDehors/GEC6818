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
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_extras.c src/extras.c src/backlight.c src/bmp.c $(LDLIBS) -o build/tests/test_extras
	./build/tests/test_extras
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_lcd.c src/bmp.c $(LDLIBS) -o build/tests/test_lcd
	./build/tests/test_lcd
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_media_flow.c $(filter-out src/lcd.c src/event.c,$(wildcard src/*.c)) $(LDLIBS) -o build/tests/test_media_flow
	./build/tests/test_media_flow

preview:
	mkdir -p build/preview
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/ui_preview.c $(filter-out src/lcd.c src/event.c,$(wildcard src/*.c)) $(LDLIBS) -o build/preview/ui_preview
	./build/preview/ui_preview
