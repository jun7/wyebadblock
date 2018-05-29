/*
Copyright 2018 jun7@hush.mail

This file is part of wyebrun.

wyebrun is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

wyebrun is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with wyebrun.  If not, see <http://www.gnu.org/licenses/>.
*/


//getpid
#include <sys/types.h>
#include <unistd.h>

//flock
#include <sys/file.h>

#include "wyebrun.h"

#define ROOTNAME "wyebrun"
#define PREFIX WYEBPREFIX
#define INPUT  "wyebinput"
#define PING   "wyebping"

#define DUNTIL WYEBDUNTIL
#define DPINGTIME 1000

#define P(f, ...) g_print(#f"\n", __VA_ARGS__);

#if DEBUG
static gint64 start;
# define D(f, ...) g_print("#"#f"\n", __VA_ARGS__);
# define DD(a) g_print("#"#a"\n");
#else
# define D(...) ;
# define DD(a) ;
#endif

typedef enum {
	//to svr
	CSuntil = 'u',
	CSdata  = 'd',
	CSping  = 'p',

	//to client
	CCwoke  = 'w',
	CCret   = 'r', //retrun data
	CClost  = 'l', //we lost the req
} Com;





//shared
static void fatal(int i)
{
	P(\n!!! fatal %d !!!\n, i)
	exit(1);
}
static void mkdirif(char *path)
{
	char *dir = g_path_get_dirname(path);
	if (!g_file_test(dir, G_FILE_TEST_EXISTS))
		g_mkdir_with_parents(dir, 0700);

	g_free(dir);
}
static char *ipcpath(char *exe, char *name)
{
	return g_build_filename(g_get_user_runtime_dir(),
			ROOTNAME,exe, name, NULL);
}
static char *preparepp(char *exe, char *name)
{
	char *path = ipcpath(exe, name);
	if (!g_file_test(path, G_FILE_TEST_EXISTS))
	{
		mkdirif(path);
		mkfifo(path, 0600);
	}
	return path;
}

static bool ipcsend(char *exe, char *name,
		Com type, char *caller, char *data)
{
	//D(ipcsend exe:%s name:%s, exe, name)
	char *path = preparepp(exe, name);

	char *esc  = g_strescape(data ?: "", "");
	char *line = g_strdup_printf("%c%s:%s\n", type, caller ?: "", esc);
	g_free(esc);

	int pp = open(path, O_WRONLY | O_NONBLOCK);

	bool ret = write(pp, line, strlen(line)) != -1;
	g_free(line);

	close(pp);

	g_free(path);
	return ret;
}

static gboolean ipccb(GIOChannel *ch, GIOCondition c, gpointer p);

static GSource *ipcwatch(char *exe, char *name, GMainContext *ctx) {
	char *path = preparepp(exe, name);

	GIOChannel *io = g_io_channel_new_file(path, "r+", NULL);
	GSource *watch = g_io_create_watch(io, G_IO_IN);
	g_io_channel_unref(io);
	g_source_set_callback(watch, (GSourceFunc)ipccb, NULL, NULL);
	g_source_attach(watch, ctx);

	g_free(path);
	return watch;
}



//@server
static char *svrexe = NULL;
static GMainLoop *sloop = NULL;
static wyebdataf dataf = NULL;
static gboolean quit(gpointer p)
{
	DD(\nsvr quits\n)
	g_main_loop_quit(sloop);
	return false;
}
static void until(int sec)
{
	if (!sloop) return;

	static guint last = 0;
	if (last)
		g_source_remove(last);
	last = g_timeout_add_full(G_PRIORITY_LOW * 2, sec * 1000, quit, NULL, NULL);
}
static gpointer pingt(gpointer p)
{
	GMainContext *ctx = g_main_context_new();
	ipcwatch(svrexe, PING, ctx);
	g_main_loop_run(g_main_loop_new(ctx, true));
	return NULL;
}

void wyebwatch(char *exe, char *caller, wyebdataf func)
{
	svrexe = exe;
	dataf = func;
	until(DUNTIL);

	g_thread_new("ping", pingt, NULL);
	ipcwatch(exe, INPUT, g_main_context_default());

	if (!ipcsend(exe, caller, CCwoke, "", NULL))
		fatal(1);
}

static gboolean svrinit(char *caller)
{
	wyebwatch(svrexe, caller, dataf);
	return false;
}

bool wyebsvr(int argc, char **argv, wyebdataf func)
{
	if (argc < 2 || !g_str_has_prefix(argv[1], PREFIX)) return false;

	svrexe = argv[0];
	dataf = func;
	sloop = g_main_loop_new(NULL, false);
	g_idle_add((GSourceFunc)svrinit, argv[1]);
	g_main_loop_run(sloop);

	return true;
}

static void getdata(char *caller, char *req)
{
	char *data = dataf(req);
	if (*caller && !ipcsend(svrexe, caller, CCret, "", data))
		fatal(2);

	g_free(data);
}




//@client
static GMutex retm;
static GMainLoop *cloop;
static GMainContext *wctx = NULL;
static char *retdata = NULL;
static char *pid()
{
	static char *pid = NULL;
	if (!pid) pid = g_strdup_printf(PREFIX"%d", getpid());
	return pid;
}

static void spawnsvr(char *exe)
{
	char **argv = g_new0(char*, 2);
	argv[0] = exe;
	argv[1] = pid();
	GError *err = NULL;
	if (!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH,
				NULL, NULL, NULL, &err))
	{
		g_print("err %s", err->message);
		g_error_free(err);
	}

	g_free(argv);
}
static gboolean pingloop(gpointer p)
{
	if (!ipcsend((char *)p, PING, CSping, pid(), NULL))
		g_mutex_unlock(&retm);

	return true;
}
static char *pppath = NULL;
static void removepp()
{
	remove(pppath);
}
static gpointer watcht(char *exe)
{
	wctx = g_main_context_new();
	cloop = g_main_loop_new(wctx, true);

	GSource *watch = ipcwatch(exe, pid(), wctx);

	if (!pppath)
	{
		pppath = ipcpath(exe, pid());
		atexit(removepp);
	}

	g_mutex_unlock(&retm);
	g_main_loop_run(cloop);

	g_source_unref(watch);
	g_main_context_unref(wctx);
	g_main_loop_unref(cloop);
	cloop = NULL;

	return NULL;
}
static void watchstart(char *exe)
{
	g_mutex_lock(&retm);

	g_thread_new("watch", (GThreadFunc)watcht, exe);

	g_mutex_lock(&retm);
	g_mutex_unlock(&retm);
}
static gboolean timeout(gpointer p)
{
	g_mutex_unlock(&retm);
	return false;
}

static GHashTable *lastsec = NULL;
static void reuntil(char *exe)
{
	if (!lastsec) return;
	int sec = GPOINTER_TO_INT(
		g_hash_table_lookup(lastsec, exe));
	if (sec)
		wyebuntil(exe, sec);
}

//don't free
static char *request(char *exe, Com type, char *caller, char *req)
{
	g_free(retdata);
	retdata = NULL;

	if (!cloop) watchstart(exe);

	if (caller)
		g_mutex_lock(&retm);

	if (!ipcsend(exe, INPUT, type, caller, req))
	{ //svr is not running
		char *path = ipcpath(exe, "lock");
		if (!g_file_test(path, G_FILE_TEST_EXISTS))
			mkdirif(path);

		int lock = open(path, O_RDONLY | O_CREAT, S_IRUSR);
		g_free(path);

		//retry in single proc
		if (!ipcsend(exe, INPUT, type, caller, req))
		{
			if (!caller)
				g_mutex_lock(&retm);

			GSource *tout = g_timeout_source_new(DUNTIL * 1000);
			g_source_set_callback(tout, timeout, NULL, NULL);
			g_source_attach(tout, wctx);

			spawnsvr(exe);
			g_mutex_lock(&retm);
			g_mutex_unlock(&retm);

			g_source_destroy(tout);
			g_source_unref(tout);

			//wyebloop doesn't know svr quits
			reuntil(exe);

			if (caller)
				g_mutex_lock(&retm);
			if (!ipcsend(exe, INPUT, type, caller, req))
				fatal(3); //spawning svr failse is fatal
		}
		close(lock);
	}

	if (caller)
	{
		GSource *ping = g_timeout_source_new(DPINGTIME);
		g_source_set_callback(ping, (GSourceFunc)pingloop, exe, NULL);
		g_source_attach(ping, wctx); //attach to ping thread

		g_mutex_lock(&retm);
		g_mutex_unlock(&retm);

		g_source_destroy(ping);
		g_source_unref(ping);
	}

	return retdata;
}

char *wyebreq(char *exe, char *req)
{
	return request(exe, CSdata, pid(), req);
}
void wyebsend(char *exe, char *req)
{
	request(exe, CSdata, NULL, req);
}
typedef struct {
	char *exe;
	int   sec;
} wyebsst;
static void wsfree(wyebsst *ss)
{
	g_free(ss->exe);
	g_free(ss);
}
static gboolean untilcb(wyebsst *ss)
{
	if (!lastsec)
		lastsec = g_hash_table_new(g_str_hash, g_str_equal);

	g_hash_table_replace(lastsec, ss->exe, GINT_TO_POINTER(ss->sec));

	char *str = g_strdup_printf("%d", ss->sec);
	request(ss->exe, CSuntil, NULL, str);
	g_free(str);

	return false;
}
void wyebuntil(char *exe, int sec)
{
	wyebsst *ss = g_new(wyebsst, 1);
	ss->exe = g_strdup(exe);
	ss->sec = sec;
	g_idle_add_full(G_PRIORITY_DEFAULT, (GSourceFunc)untilcb,
			ss, (GDestroyNotify)wsfree);
}

static gboolean loopcb(wyebsst *ss)
{
	untilcb(ss);
	return true;
}
guint wyebloop(char *exe, int sec, int loopsec)
{
	wyebsst *ss = g_new(wyebsst, 1);
	ss->exe = g_strdup(exe);
	ss->sec = sec;

	loopcb(ss);
	return g_timeout_add_full(G_PRIORITY_DEFAULT,
			loopsec * 1000,
			(GSourceFunc)loopcb,
			ss, (GDestroyNotify)wsfree);
}

static gboolean tcinputcb(GIOChannel *ch, GIOCondition c, char *exe)
{
	char *line;

	if (G_IO_STATUS_EOF == g_io_channel_read_line(ch, &line, NULL, NULL, NULL))
		exit(0);

	if (!line) return true;

	g_strstrip(line);
	if (!strlen(line))
		exit(0);

#if DEBUG
	if (g_str_has_prefix(line, "l"))
	{
		start = g_get_monotonic_time();
		for (int i = 0; i < 1000; i++)
		{
			char *is = g_strdup_printf("l%d", i);
			//g_print("loop %d ret %s\n", i, wyebreq(exe, is));
			if (*(line + 1) == 's')
				wyebsend(exe, is);
			else if (*(line + 1) == 'p')
				D(pint %s, is)
			else
				wyebreq(exe, is);
			g_free(is);
		}
		gint64 now = g_get_monotonic_time();
		char *time = g_strdup_printf("time %f", (now - start) / 1000000.0);
		wyebsend(exe, time);
		g_free(time);
	}
	else
		g_print("RET is %s\n", wyebreq(exe, line));
#else
	g_print("%s\n", wyebreq(exe, line)); //don't free
#endif

	g_free(line);
	return true;
}
static gboolean tcinit(char *exe)
{
	//wyebuntil(exe, 1);
	wyebloop(exe, 2, 1);

	GIOChannel *io = g_io_channel_unix_new(fileno(stdin));
	g_io_add_watch(io, G_IO_IN, (GIOFunc)tcinputcb, exe);

	return false;
}
void wyebclient(char *exe)
{
	//pid_t getpid(void);
	sloop = g_main_loop_new(NULL, false);
	g_idle_add((GSourceFunc)tcinit, exe);
	g_main_loop_run(sloop);
}



//@ipccb
static GHashTable *orders = NULL;
gboolean ipccb(GIOChannel *ch, GIOCondition c, gpointer p)
{
	if (!orders)
		orders = g_hash_table_new(g_str_hash, g_str_equal);

	char *line;
	g_io_channel_read_line(ch, &line, NULL, NULL, NULL);
	if (!line) return true;
	g_strchomp(line);

	char *unesc = g_strcompress(line);
	g_free(line);

	Com type  = *unesc;
	char *id  = unesc + 1;
	char *arg = strchr(unesc, ':');
	*arg++ = '\0';

#if DEBUG
	static int i = 0;
	D(ipccb%d %c/%s/%s;, i++, type ,id ,arg)
#endif

	static int lastuntil = DUNTIL;
	switch (type) {
	//server
	case CSuntil:
		until(lastuntil = atoi(arg));
		break;
	case CSdata:
		g_hash_table_add(orders, id);
		getdata(id, arg);
		g_hash_table_remove(orders, id);
		until(lastuntil);
		break;
	case CSping:
		if (!g_hash_table_lookup(orders, id))
			ipcsend(svrexe, id, CClost, NULL, NULL);
		break;

	//client
	case CCret:
		retdata = g_strdup(arg);
	case CClost:
	case CCwoke:
		//g_main_loop_quit(cloop);
		g_mutex_unlock(&retm);
		break;
	}

	g_free(unesc);
	return true;
}



//test
#if DEBUG
static char *testdata(char *req)
{
	//sleep(9);
	//g_free(req); //makes crash

	static int i = 0;
	return g_strdup_printf("%d th dummy data. req is %s", ++i, req);
}


int main(int argc, char **argv)
{
	start = g_get_monotonic_time();
//	gint64 now = g_get_monotonic_time();
//	D(time %ld %ld, now - start, now)

	//char path[PATH_MAX] = {0};
	//readlink("/proc/self/exe", path, PATH_MAX);
	//D(progrname %s, path)

	if (!wyebsvr(argc, argv, testdata))
		wyebclient(argv[0]);

	exit(0);
}
#endif
