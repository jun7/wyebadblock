# wyebadblock
An adblock extension for [wyeb](https://github.com/jun7/wyeb), also webkit2gtk browsers.

most of code of this are from https://github.com/GNOME/epiphany/tree/master/embed/web-extension


### usage:

	make
	sudo make install

then
copy **easylist.txt** to ~/.config/wyebadblock/

wyebadblock only checks 'easylist.txt'


You can check if it works on http://simple-adblock.com/faq/testing-your-adblocker/
Testing element hiding is not supported though.

### Disabling

Setting chars(whatever) to the env value $DISABLE_ADBLOCK disables adblock.

For source code:
set string "adblock:false;" as the user data of the
webkit_web_context_set_web_extensions_initialization_user_data;


---


## For webkit2gtk browsers
On Arch Linux

### surf
	sudo mkdir usr/local/lib/surf
	sudo ln -s /usr/lib/wyebrowser/adblock.so /usr/local/lib/surf

See vimb below to manage the link by pacman

### vimb

	sudo ln -s /usr/lib/wyebrowser/adblock.so /usr/lib/vimb

There is a PKGBUILD file. see the 'PKGBULDs' dir.

### lariza

	mkdir -p ~/.config/lariza/web_extensions
	ln -s /usr/lib/wyebrowser/adblock.so ~/.config/lariza/web_extensions


### Others

webkit2gtk loads extensions in a dir designated by each apps.
So we have to know which dir is the dir.

search 'webkit_web_context_set_web_extensions_directory' in source code
and make link from the dir to the wyebadblock as above.


---


## Element Hiding
Per domain CSS hider rule is not supported and this may crash webkit2gtk.

	make cssoutput
	./cssoutput > user.css

And add the user.css to your browser as user css.
On wyeb, just copy the user.css to the conf dir.
