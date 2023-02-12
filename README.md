# wyebadblock
An adblock extension using the easylist.txt for [wyeb](https://github.com/jun7/wyeb), also webkit2gtk browsers.

most of code of this are from https://github.com/GNOME/epiphany/tree/master/embed

wyebab is shared by clients, So even nowadays, browsers spawn procs for each windows,
wyebab keeps single server proc that makes less RAM and less CPU times.

For example, while an epiphay's webproc uses 240M RAM for
a page, wyeb uses 160M for the same page.
Of course wyebab uses 80M but not gain.

Don't worry, wyebab wills quit automatically when there is no client and 30 secs past.

## usage:

depends: gtk+-3.0 glib-2.0 webkit2gtk-4.0

	make
	sudo make install

then
copy the **easylist.txt** to ~/.config/wyebadblock/

wyebadblock only checks 'easylist.txt'

You can check whether wyebab has found easylist.txt by `wyebab --css`

## for webkit2gtk-4.1
	WEBKITVER=4.1 make
	sudo WEBKITVER=4.1 make install
Make sure this adds a dir /4.1 before the adblock.so

## Addition for other webkit2gtk browsers
webkit2gtk loads extensions in a dir designated by each apps.
So we have to make a link to wyebab in the dir.

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

	sudo ln -s /usr/lib/wyebrowser/4.1/adblock.so /usr/lib/vimb

If the path doesn't work, check https://fanglingsu.github.io/vimb/howto.html#block

### luakit

	sudo ln -s /usr/lib/wyebrowser/adblock.so /lib/luakit

### lariza

	mkdir -p ~/.config/lariza/web_extensions
	ln -s /usr/lib/wyebrowser/adblock.so ~/.config/lariza/web_extensions

Since lariza enables sandbox, wyebadblock does not work without changing 'webkit_web_context_set_sandbox_enabled'.

### badwolf

	mkdir -p ${XDG_DATA_HOME:-$HOME/.local/share}/badwolf/webkit-web-extensions/
	ln -s /usr/lib/wyebrowser/adblock.so ${XDG_DATA_HOME:-$HOME/.local/share}/badwolf/webkit-web-extensions/

### Others

search 'webkit_web_context_set_web_extensions_directory' in its source code
and make the link from the dir to the wyebadblock just like above browsers.

Also make sure 'webkit_web_context_set_sandbox_enabled' is not set.

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

or

Move the easylist.txt.

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
