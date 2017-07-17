#CFLAGS += -c -Wall -Wno-deprecated-declarations
CFLAGS += -Wno-deprecated-declarations
#LDFLAGS=
EXTENSION_DIR=$(DESTDIR)/usr/share/wyebrowser
DAPPNAME=-DAPPNAME='"wyebadblock"'

all: adblock.so

adblock.so: ephy-uri-tester.c ephy-uri-tester.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -shared -fPIC \
		`pkg-config --cflags --libs gtk+-3.0 glib-2.0 webkit2gtk-4.0` \
		-DEXTENSION_DIR=\"$(EXTENSION_DIR)\" \
		$(DDEBUG) $(DAPPNAME)

clean:
	rm -f adblock.so

install: all
	install -Dm755 adblock.so $(EXTENSION_DIR)/adblock.so

