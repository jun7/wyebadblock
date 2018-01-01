# wyebadblock
An adblock extension for [wyeb](https://github.com/jun7/wyeb), also webkit2gtk browsers.

most of code of this are from https://github.com/GNOME/epiphany/tree/master/embed/web-extension


### usage:

    make
    make install

then
copy **easylist.txt** to ~/.config/wyebadblock

this only checks 'easylist.txt'



## for webkit2gtk's browsers
### luakit

	sudo ln -s /lib/wyebrowser/adblock.so /lib/luakit

### vimb

	sudo ln -s /lib/wyebrowser/adblock.so /lib/vimb

### lariza

	mkdir -p ~/.config/lariza/web_extensions
	ln -s /lib/wyebrowser/adblock.so ~/.config/lariza/web_extensions
