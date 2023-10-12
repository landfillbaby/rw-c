.PHONY: rw install clean
PREFIX ?= /usr/local
INSTALL ?= install
define ccver :=
printf '#ifdef __clang__\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n' \
'#if __clang_major__ > 11' 'y' '#endif' '#elif defined __GNUC__' \
'#if __GNUC__ > 10' 'y' '#endif' '#endif' | $(CC) -E -P -x c -
endef
ifeq ($(shell $(LINK.c) -dumpmachine | cut -d- -f1)_$(shell $(ccver)),x86_64_y)
CFLAGS ?= -O3 -Wall -Wextra -pipe -flto=auto -march=x86-64-v2
else
CFLAGS ?= -O3 -Wall -Wextra -pipe -flto=auto
endif
# CPPFLAGS ?= -DNDEBUG
ifeq ($(OS),Windows_NT)
LDFLAGS ?= -s -Wl,--as-needed,--gc-sections,--no-insert-timestamp
else
LDFLAGS ?= -s -Wl,--as-needed,--gc-sections
endif
all: rw rw_hugemalloc
rw: rw_realloc
	cp rw_realloc$(EXEEXT) rw$(EXEEXT)
rw_realloc: rw_realloc.c
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@
rw_hugemalloc: rw_hugemalloc.c
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@
install: rw
	$(INSTALL) $^ $(DESTDIR)$(PREFIX)/bin/
clean:
	$(RM) rw
