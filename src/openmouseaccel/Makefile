CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -std=c99
CPPFLAGS += -Iinclude
LDLIBS += -lm

CORE = src/oma_core.c src/oma_config.c src/oma_filter.c

all: test prefs-host

test: build/test_core
	./build/test_core

build/test_core: $(CORE) tests/test_core.c
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(CORE) tests/test_core.c $(LDLIBS)

prefs-host: build/OpenMouseAccelPrefs-host

build/OpenMouseAccelPrefs-host: $(CORE) prefs/OpenMouseAccelPrefs.c
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(CORE) prefs/OpenMouseAccelPrefs.c $(LDLIBS)

clean:
	rm -rf build

.PHONY: all test prefs-host clean
