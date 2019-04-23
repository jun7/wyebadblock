LISTNAME=easylist.txt
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
EXTENSION_DIR ?= $(PREFIX)/lib/wyebrowser
PKG_CONFIG ?= pkg-config

ifeq ($(DEBUG), 1)
	CFLAGS += -Wall
else
	DEBUG = 0
	CFLAGS += -Wno-deprecated-declarations
endif
DDEBUG=-DDEBUG=${DEBUG}

all: adblock.so wyebab librun.o testrun

adblock.so: ab.c ephy-uri-tester.c ephy-uri-tester.h librun.o makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< librun.o -shared -fPIC \
		`$(PKG_CONFIG) --cflags --libs gtk+-3.0 glib-2.0 webkit2gtk-4.0` \
		$(DDEBUG) -DISEXT -DEXENAME=\"wyebab\"

wyebab: ab.c ephy-uri-tester.c ephy-uri-tester.h librun.o makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< librun.o \
		`$(PKG_CONFIG) --cflags --libs glib-2.0 gio-2.0` \
		$(DDEBUG) -DDIRNAME=\"wyebadblock\" -DLISTNAME=\"$(LISTNAME)\"

librun.o: wyebrun.c wyebrun.h makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $< -fPIC\
		`$(PKG_CONFIG) --cflags --libs glib-2.0` \
		$(DDEBUG)

testrun: wyebrun.c wyebrun.h makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< \
		`$(PKG_CONFIG) --cflags --libs glib-2.0 gio-2.0` \
		$(DDEBUG) -DTESTER=1

clean:
	rm -f adblock.so
	rm -f wyebab
	rm -f librun.o
	rm -f testrun

install:
	install -Dm755 wyebab     $(DESTDIR)$(BINDIR)/wyebab
	install -Dm755 adblock.so $(DESTDIR)$(EXTENSION_DIR)/adblock.so

uninstall:
	rm -f  $(BINDIR)/wyebab
	rm -f  $(EXTENSION_DIR)/adblock.so
	-rmdir $(EXTENSION_DIR)


re: clean all
#	$(MAKE) clean
#	$(MAKE) all

full: re install
