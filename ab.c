/*
Copyright 2018 jun7@hush.com

This file is part of wyebadblock.

wyebadblock is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

wyebadblock is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with wyebadblock.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "wyebrun.h"

#if DEBUG
# define D(f, ...) g_print("#"#f"\n", __VA_ARGS__);
# define DD(a) g_print("#"#a"\n");
#else
# define D(f, ...) ;
# define DD(a) ;
#endif


#if ISEXT

#include <webkit2/webkit-web-extension.h>
static bool check(const char *requri, const char *pageuri)
{
	char *uris = g_strconcat(requri, " ", pageuri, NULL);
	char *ruri = wyebget(EXENAME, uris);
	g_free(uris);

	if (ruri && !*ruri) return false;
	return true;
}
static gboolean reqcb(WebKitWebPage *kp, WebKitURIRequest *req,
		WebKitURIResponse *r, gpointer p)
{
	if (g_object_get_data(G_OBJECT(kp), "adblock") == (gpointer)'n')
		return false;

	static bool first = true;
	if (first)
	{
		if (webkit_uri_request_get_http_headers(req))
			first = false;
		else //no head is local data. so haven't to block
			return false;
	}

	if (check(webkit_uri_request_get_uri(req),
				webkit_web_page_get_uri(kp))) return false;
	return true;
}

#if WEBKIT_MAJOR_VERSION > 2 || WEBKIT_MINOR_VERSION > 26
static gboolean msgcb(WebKitWebPage *kp, WebKitUserMessage *msg, gpointer p)
{
	if (strcmp("wyebadblock", webkit_user_message_get_name(msg))) return false;
	g_object_set_data(G_OBJECT(kp), "adblock",
		g_variant_get_boolean(webkit_user_message_get_parameters(msg)) ? NULL :
		GINT_TO_POINTER('n'));
	return true;
}
#endif

static gboolean keepcb(WebKitWebPage *kp)
{
	if (g_object_get_data(G_OBJECT(kp), "adblock") != (gpointer)'n')
		wyebkeep(EXENAME, 30);
	return true;
}

static bool apimode;
static void pageinit(WebKitWebExtension *ex, WebKitWebPage *kp)
{
	DD(pageinit)

	if (!apimode)
		g_signal_connect(kp, "send-request", G_CALLBACK(reqcb), NULL);
#if WEBKIT_MAJOR_VERSION > 2 || WEBKIT_MINOR_VERSION > 26
	g_signal_connect(kp, "user-message-received", G_CALLBACK(msgcb), NULL);
#endif

	g_object_set_data(G_OBJECT(kp), "wyebcheck", check);

	keepcb(kp);
	g_object_weak_ref(G_OBJECT(kp), (GWeakNotify)g_source_remove,
			GUINT_TO_POINTER(g_timeout_add(11 * 1000, (GSourceFunc)keepcb, kp)));
}

G_MODULE_EXPORT void webkit_web_extension_initialize_with_user_data(
		WebKitWebExtension *ex, const GVariant *v)
{
	bool hasarg = false;
	const char *str;
	if (v && g_variant_is_of_type((GVariant *)v, G_VARIANT_TYPE_STRING) &&
		(str = g_variant_get_string((GVariant *)v, NULL)))
	{
		bool enable = true;
		char **args = g_strsplit(str, ";", -1);
		for (char **arg = args; *arg; arg++)
		{
			if (g_str_has_prefix(*arg, "adblock:"))
			{
				enable = !strcmp(*arg + 8, "true");
				hasarg = true;
			}
			if (!strcmp(*arg, "wyebabapi"))
				apimode = true;
		}
		g_strfreev(args);
		if (!enable) return;
	}

	if (!hasarg && *(g_getenv("DISABLE_ADBLOCK") ?: "") != '\0')
		return;

	g_signal_connect(ex, "page-created", G_CALLBACK(pageinit), NULL);
}



#else

#include "ephy-uri-tester.c"

static EphyUriTester *tester;
static GThread *initt;

static gpointer inittcb(gpointer p)
{
	ephy_uri_tester_load(tester);
	return NULL;
}

static void monitorcb(
		GFileMonitor *m, GFile *f, GFile *o, GFileMonitorEvent e, gpointer p)
{
	if (e == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT ||
			e == G_FILE_MONITOR_EVENT_DELETED)
		exit(0);
}
static bool tryload(const char *parent)
{
	char *path = g_build_filename(parent, DIRNAME, LISTNAME, NULL);

	GFile *gf = g_file_new_for_path(path);
	GFileMonitor *gm = g_file_monitor_file(gf,
			G_FILE_MONITOR_NONE, NULL, NULL);
	g_signal_connect(gm, "changed", G_CALLBACK(monitorcb), NULL);
	g_object_unref(gf);

	if (g_file_test(path, G_FILE_TEST_EXISTS))
	{
		filter_file = g_file_new_for_path(path);
		tester = ephy_uri_tester_new("/foo/bar");
		initt = g_thread_new("init", inittcb, NULL);
	}
	g_free(path);
	return tester;
}
static void init()
{
	DD(wyebad init)
	if (tryload(g_get_user_config_dir())) return;

	for (const gchar * const *d = g_get_system_data_dirs(); *d; d++)
		if (tryload(*d)) return;

	char *dir = g_build_filename(g_get_user_config_dir(), DIRNAME, NULL);
	if (!g_file_test(dir, G_FILE_TEST_EXISTS))
		g_mkdir_with_parents(dir, 0700);
	g_free(dir);
}

static char *datafunc(char *req)
{
	static GMutex datam;
	g_mutex_lock(&datam);

	if (initt)
	{
		g_thread_join(initt);
		initt = NULL;
	}

	//req uri + ' ' + page uri
	char **args = g_strsplit(req, " ", 2);

	char *ret = !tester ? g_strdup(args[0]) :
		ephy_uri_tester_rewrite_uri(tester, args[0],  args[1] ?: args[0]);

	g_strfreev(args);

#if DEBUG
	if (ret)
		D(ret %s, ret)
	else
		D(BLOCKED %s, req)
#endif

	g_mutex_unlock(&datam);
	return ret;
}


int main(int argc, char **argv)
{
	DD(This bin is compiled with DEBUG=1)

	if (argc == 1)
	{
		wyebclient(argv[0]);
	}
	else if (!strcmp(argv[1], WYEBPREFIX))
	{
		init();
		wyebsvr(argc, argv, datafunc);
	}
	else if (!strcmp(argv[1], "-css") || !strcmp(argv[1], "--css"))
	{
		init();
		if (!initt)
		{
			g_print("#There is no "LISTNAME"\n");
			exit(0);
		}
		g_thread_join(initt);

		g_print("%s", tester->blockcss->str);
		g_print("\n\n\n\n"
				"{display:none !important} /*\n"
				"{opacity:0 !important; position:absolute !important;}\n"
				"/**/\n"
				);
		//g_print(tester->blockcssprivate->str);
	}
	else
	{
		wyebkeep(argv[0], 30);
		g_print("%s", wyebget(argv[0], argv[1]));
	}

	exit(0);
}

#endif
