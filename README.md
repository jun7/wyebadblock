# wyebadblock
An adblock extension for [wyeb](https://github.com/jun7/wyeb), also webkit2gtk browsers.

most of code of this are from https://github.com/GNOME/epiphany/tree/master/embed/web-extension


### usage:

    make
    make install

then
copy **easylist.txt** to ~/.config/wyebadblock

this only checks 'easylist.txt'


---

Setting chars(whatever) to the env value $DISABLE_ADBLOCK disables adblock.

For source code:
set string ";adblock:false;" as user data of the
webkit_web_context_set_web_extensions_initialization_user_data;


---

## webkit2gtk browsers
On Arch Linux

### luakit

	sudo ln -s /lib/wyebrowser/adblock.so /lib/luakit

### vimb

	sudo ln -s /lib/wyebrowser/adblock.so /lib/vimb
	
There is a PKGBUILD file. see PKGBULDs dir.
For luakit, just change the strings in the file 'vimb' -> 'luakit'.

### lariza

	mkdir -p ~/.config/lariza/web_extensions
	ln -s /lib/wyebrowser/adblock.so ~/.config/lariza/web_extensions

