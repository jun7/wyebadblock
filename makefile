#CFLAGS += -Wall -Wno-deprecated-declarations
CFLAGS += -Wno-deprecated-declarations
#LDFLAGS=
EXTENSION_DIR=$(DESTDIR)/usr/lib/wyebrowser
DAPPNAME=-DAPPNAME='"wyebadblock"'

ifdef DEBUG
	DDEBUG=-DDEBUG=${DEBUG}

ifneq ($(DEBUG), 0)
	CFLAGS += -Wall
endif
endif

all: adblock.so wyebab librun.o testrun

adblock.so: ephy-uri-tester.c ephy-uri-tester.h librun.o makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< librun.o -shared -fPIC \
		`pkg-config --cflags --libs gtk+-3.0 glib-2.0 webkit2gtk-4.0` \
		-DEXTENSION_DIR=\"$(EXTENSION_DIR)\" \
		$(DDEBUG) $(DAPPNAME) -DISEXT

wyebab: ephy-uri-tester.c ephy-uri-tester.h librun.o makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< librun.o \
		`pkg-config --cflags --libs glib-2.0 libsoup-2.4` \
		-DEXTENSION_DIR=\"$(EXTENSION_DIR)\" \
		$(DDEBUG) $(DAPPNAME)

librun.o: wyebrun.c wyebrun.h makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $< -fPIC\
		`pkg-config --cflags --libs glib-2.0` \
		-DDEBUG=0

testrun: wyebrun.c wyebrun.h makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< \
		`pkg-config --cflags --libs glib-2.0` \
		-DDEBUG=1

clean:
	rm -f adblock.so
	rm -f wyebab
	rm -f librun.o
	rm -f testrun

install: all
	install -Dm755 wyebab     $(DESTDIR)/usr/bin/wyebab
	install -Dm755 adblock.so $(EXTENSION_DIR)/adblock.so

re: clean all

uninstall:
	rm -f  $(DESTDIR)/usr/bin/wyebab
	rm -f  $(EXTENSION_DIR)/adblock.so
	-rmdir $(EXTENSION_DIR)
