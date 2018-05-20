# wyebadblock
An adblock extension for [wyeb](https://github.com/jun7/wyeb), also webkit2gtk browsers.

most of code of this are from https://github.com/GNOME/epiphany/tree/master/embed/web-extension

wyebad is shared by clients, So even nowadays, browsers spawn procs for each windows,
wyebad keeps single server proc that makes less memory and less cpu times.
Don't worry, wyeb wills quit automatically when there is no client and 30 secs past.

For exsample, while epiphay's webproc uses 240M RAM for
[a page](http://simple-adblock.com/faq/testing-your-adblocker/), wyeb uses 160M for the same page.
Of course the adblock uses 80M but not gain.

### usage:

	make
	sudo make install

then
copy **easylist.txt** to ~/.config/wyebadblock/

wyebadblock only checks 'easylist.txt'

Testing element hiding is not supported though,
You can check if it works on http://simple-adblock.com/faq/testing-your-adblocker/

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

### luakit

	sudo ln -s /lib/wyebrowser/adblock.so /lib/luakit

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
Per domain CSS hider rule is not supported

	wyebab -css > user.css

And add the user.css to your browser as user css.
For wyeb, just copy the user.css to the conf dir.

Make sure the huge css takes mamory a lot.

## Shell

	wyebab

Reads stdin outputs to stdout.
blank + enter quits.

	wyebab requst_uri + ' ' + page_uri

Outputs result
Keeps server 30 sec
