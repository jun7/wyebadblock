# wyebadblock
An adblock extension using the easylist.txt for [wyeb](https://github.com/jun7/wyeb), also webkit2gtk browsers.

most of code of this are from https://github.com/GNOME/epiphany/tree/master/embed/web-extension

wyebab is shared by clients, So even nowadays, browsers spawn procs for each windows,
wyebab keeps single server proc that makes less RAM and less CPU times.

For example, while an epiphay's webproc uses 240M RAM for
[a page](http://simple-adblock.com/faq/testing-your-adblocker/), wyeb uses 160M for the same page.
Of course wyebab uses 80M but not gain.

Don't worry, wyebab wills quit automatically when there is no client and 30 secs past.

## usage:

	make
	sudo make install

then
copy the **easylist.txt** to ~/.config/wyebadblock/

wyebadblock only checks 'easylist.txt'

Testing element hiding is not supported though,
You can check whether it works on http://simple-adblock.com/faq/testing-your-adblocker/


## Addition for other webkit2gtk browsers
webkit2gtk loads extensions in a dir designated by each apps.
So we have to make the link of wyebab in the dir.

Following paths are for Arch Linux. Make sure on other distros, the paths are may different.

### surfer

See the Surfer's README

https://github.com/nihilowy/surfer

### epiphany

	sudo ln -s /usr/lib/wyebrowser/adblock.so /usr/lib64/epiphany/web-extensions

This lacks epiphany's functionalities about adblock (e.g. can't disable), but saves RAM and CPU.
Make sure while you keep enable epiphany's one, it is working too.

### surf
	sudo mkdir usr/local/lib/surf
	sudo ln -s /usr/lib/wyebrowser/adblock.so /usr/local/lib/surf

There is a PKGBUILD file. see the 'PKGBULDs' dir.

### vimb

	sudo ln -s /usr/lib/wyebrowser/adblock.so /usr/lib/vimb

There is a PKGBUILD file. see the 'PKGBULDs' dir.

If the path doesn't work, Check https://fanglingsu.github.io/vimb/howto.html#block

### luakit

	sudo ln -s /usr/lib/wyebrowser/adblock.so /lib/luakit

### lariza

	mkdir -p ~/.config/lariza/web_extensions
	ln -s /usr/lib/wyebrowser/adblock.so ~/.config/lariza/web_extensions


### Others

search 'webkit_web_context_set_web_extensions_directory' in its source code
and make the link from the dir to the wyebadblock just like above browsers.


---


## Element Hiding
Per domain CSS hider rule is not supported

	wyebab --css > user.css

And add the user.css to your browser as a user css.
For wyeb, just copy the user.css to the conf dir.

Make sure that huge css takes RAM a lot.
Also it is often used to detect adblock.


## Disabling

Setting chars(whatever) to the env value $DISABLE_ADBLOCK disables adblock.

### For source code:
set string ";adblock:false;" as the user data of the
webkit_web_context_set_web_extensions_initialization_user_data;


#### Runtime

	g_object_set_data(G_OBJECT(webkitwebpage), "adblock", GINT_TO_POINTER('n'));

in any extension in the same process.

#### Full Control

If you want full control of wyebab as wyeb does, wyebab has api mode.
Set string ";wyebabapi;" as the extensions_init's data.
In api mode wyebab doesn't block URIs but keeps alive server proc and
set the check function to the page object.
So you can call the check func where ever as below.

	bool (*checkf)(const char *, const char *) =
		g_object_get_data(G_OBJECT(webkitwebpage), "wyebcheck");
	if (checkf)
		passed = checkf(requesturi, pageuri);


or

Use the shell interface below.

or

Use wyebrun like the client code of wyebab in the ISEXT part of the ab.c.

## Shell

	wyebab

Reads stdin outputs to stdout and keeps server.
blank + enter quits.

	wyebab "requst_uri page_uri"

Outputs result and
Keeps server 30 secs.
