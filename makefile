LISTNAME=easylist.txt
PREFIX ?= /usr
WEBKITVER ?= 4.1
WEBKIT ?= webkit2gtk-$(WEBKITVER)
EXTENSION_DIR ?= $(PREFIX)/lib/wyebrowser

ifneq ($(WEBKITVER), 4.0)
	VERDIR=/$(WEBKITVER)
endif
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
		`pkg-config --cflags --libs gtk+-3.0 glib-2.0 $(WEBKIT)` \
		$(DDEBUG) -DISEXT -DEXENAME=\"wyebab\"

wyebab: ab.c ephy-uri-tester.c ephy-uri-tester.h librun.o makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< librun.o \
		`pkg-config --cflags --libs glib-2.0 gio-2.0` \
		$(DDEBUG) -DDIRNAME=\"wyebadblock\" -DLISTNAME=\"$(LISTNAME)\"

librun.o: wyebrun.c wyebrun.h makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $< -fPIC\
		`pkg-config --cflags --libs glib-2.0` \
		$(DDEBUG)

testrun: wyebrun.c wyebrun.h makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< \
		`pkg-config --cflags --libs glib-2.0 gio-2.0` \
		$(DDEBUG) -DTESTER=1

clean:
	rm -f adblock.so
	rm -f wyebab
	rm -f librun.o
	rm -f testrun

install:
	install -Dm755 wyebab     $(DESTDIR)$(PREFIX)/bin/wyebab
	install -Dm755 adblock.so $(DESTDIR)$(EXTENSION_DIR)$(VERDIR)/adblock.so

uninstall:
	rm -f  $(PREFIX)/bin/wyebab
	rm -f  $(EXTENSION_DIR)$(VERDIR)/adblock.so
	-rmdir $(EXTENSION_DIR)$(VERDIR)
	-rmdir $(EXTENSION_DIR)


re: clean all
#	$(MAKE) clean
#	$(MAKE) all

full: re install
