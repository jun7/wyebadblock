/*
Copyright 2018 jun7@hush.com

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
// for musl-libc havent fcntl in file.h
#include <fcntl.h>

//monitor
#include <gio/gio.h>


#include "wyebrun.h"

#define ROOTNAME "wyebrun"
#define CLIDIR "clients"
#define PREFIX WYEBPREFIX
#define KEEPS WYEBKEEPSEC
#define INPUT  "wyebinput"
#define PING   "wyebping"
#define DPINGTIME 1000

#define P(f, ...) g_print(#f"\n", __VA_ARGS__);

#if DEBUG
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
			ROOTNAME, exe, name, NULL);
}
static bool ipcsend(char *exe, char *name,
		Com type, char *caller, char *data)
{
	char *path = ipcpath(exe, name);

	char *esc  = g_strescape(data ?: "", "");
	D(ipcsend exe:%s name:%s type:%c sizs:%lu, exe, name, type, strlen(esc ?: ""))
	char *line = g_strdup_printf("%c%s:%s\n", type, caller ?: "", esc);
	int len = strlen(line);
	g_free(esc);


	bool ret = false;
	int pp = open(path, O_WRONLY | O_NONBLOCK);
	if (pp > -1)
	{
		if (len > PIPE_BUF) //4096 => len is atomic
		{
			flock(pp, LOCK_EX);
			fcntl(pp, F_SETFL, 0); //clear O_NONBLOCK to write len > 65536;
		}
		else
			flock(pp, LOCK_SH);

		ret = write(pp, line, len) == len;
		close(pp);
	}

	g_free(line);
	g_free(path);

	//D(ipcsend ret %d, ret)
	return ret;
}

static gboolean ipccb(GIOChannel *ch, GIOCondition cnd, gpointer p);

static GSource *ipcwatch(char *exe, char *name, GMainContext *ctx, gpointer p)
{
	char *path = ipcpath(exe, name);
	if (!g_file_test(path, G_FILE_TEST_EXISTS))
	{
		mkdirif(path);
		mkfifo(path, 0600);
	}

	GIOChannel *io = g_io_channel_new_file(path, "r+", NULL);
	GSource *watch = g_io_create_watch(io, G_IO_IN);
	g_io_channel_unref(io);
	g_source_set_callback(watch, (GSourceFunc)ipccb, p, NULL);
	g_source_attach(watch, ctx);

	g_free(path);
	return watch;
}



//@server
static char *svrexe;
static GMainLoop *sloop;
static wyebdataf dataf;
static GHashTable *orders;
static GMutex ordersm;

static gboolean quitif(gpointer p)
{
	g_mutex_lock(&ordersm);
	if (!g_hash_table_size(orders))
	{
		DD(SVR QUITS\n)
		g_main_loop_quit(sloop);
	}
	g_mutex_unlock(&ordersm);
	return true;
}
static void until(int sec)
{
	if (!sloop) return;

	static guint last;
	static GMutex m;
	g_mutex_lock(&m);
	if (last)
		g_source_remove(last);
	last = g_timeout_add_full(G_PRIORITY_LOW * 2, sec * 1000, quitif, NULL, NULL);
	g_mutex_unlock(&m);
}
static gpointer pingt(gpointer p)
{
	GMainContext *ctx = g_main_context_new();
	ipcwatch(svrexe, PING, ctx, NULL);
	g_main_loop_run(g_main_loop_new(ctx, true));
	return NULL;
}
void wyebwatch(char *exe, char *caller, wyebdataf func)
{
	svrexe = exe;
	dataf = func;
	orders = g_hash_table_new(g_str_hash, g_str_equal);

	until(KEEPS);

	g_thread_unref(g_thread_new("ping", pingt, NULL));
	ipcwatch(exe, INPUT, g_main_context_default(), NULL);

	if (!ipcsend(CLIDIR, caller, CCwoke, "", NULL))
		fatal(1);
}

static void monitorcb(GFileMonitor *m, GFile *f, GFile *o, GFileMonitorEvent e,
		gpointer p)
{
	if (e == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT)
		g_timeout_add(100, quitif, NULL);
}
static gboolean svrinit(char *caller)
{
	wyebwatch(svrexe, caller, dataf);

	char path[PATH_MAX];
	(void)readlink("/proc/self/exe", path, PATH_MAX);
	D(exepath %s, path)
	GFile *gf = g_file_new_for_path(path);
	GFileMonitor *gm = g_file_monitor_file(
			gf, G_FILE_MONITOR_NONE, NULL, NULL);
	g_signal_connect(gm, "changed", G_CALLBACK(monitorcb), NULL);
	g_object_unref(gf);

	return false;
}

bool wyebsvr(int argc, char **argv, wyebdataf func)
{
	if (argc < 3 || strcmp(argv[1], PREFIX)) return false;

	svrexe = argv[0];
	dataf = func;
	sloop = g_main_loop_new(NULL, false);
	g_idle_add((GSourceFunc)svrinit, argv[2]);
	g_main_loop_run(sloop);

	return true;
}


typedef struct {
	char *caller;
	char *data;
} Dataargs;
static void getdata(gpointer p, gpointer ap)
{
	Dataargs *args = p;

	if (*args->caller)
	{
		g_mutex_lock(&ordersm);
		g_hash_table_add(orders, args->caller);
		g_mutex_unlock(&ordersm);
	}

	char *data = dataf(args->data);
	if (*args->caller && !ipcsend(CLIDIR, args->caller, CCret, "", data)) fatal(2);
	g_free(data);

	if (*args->caller)
	{
		g_mutex_lock(&ordersm);
		g_hash_table_remove(orders, args->caller);
		g_mutex_unlock(&ordersm);
	}

	g_free(args->caller);
	g_free(args->data);
	g_free(args);
}




//@client
typedef struct {
	GMutex retm;
	GMainContext *wctx;
	GMainLoop *loop;
	GSource *watch;
	char *pid;
	char *retdata;
	char *pppath;
	char *exe; //do not free. this is tmp
} Client;

static GSList *rmpath;
static GMutex rmm;
static void __attribute__((destructor)) removepp()
{
	for (; rmpath; rmpath = rmpath->next)
		remove(rmpath->data);
}
static Client *makecl()
{
	Client *c = g_new0(Client, 1);
//	g_mutex_init(&c->retm);
	static int tid;
	c->pid = g_strdup_printf("%d-%u", tid++, getpid());

	c->wctx   = g_main_context_new();
	c->loop   = g_main_loop_new(c->wctx, true);
	c->watch  = ipcwatch(CLIDIR, c->pid, c->wctx, c);
	c->pppath = ipcpath( CLIDIR, c->pid);
	g_mutex_lock(&rmm);
	rmpath = g_slist_prepend(rmpath, c->pppath);
	g_mutex_unlock(&rmm);

	g_thread_unref(g_thread_new("wait", (GThreadFunc)g_main_loop_run, c->loop));

	return c;
}
static void freecl(Client *c)
{
	g_main_loop_quit(c->loop);
	g_source_unref(c->watch);
	g_main_loop_unref(c->loop);
	g_main_context_unref(c->wctx);
	g_mutex_clear(&c->retm);
	g_free(c->pid);
	g_free(c->retdata);
	remove(c->pppath);
	g_mutex_lock(&rmm);
	rmpath = g_slist_remove(rmpath, c->pppath);
	g_mutex_unlock(&rmm);
	g_free(c->pppath);
	g_free(c);
}
static Client *getcl()
{
	static GMutex m;
	g_mutex_lock(&m);

	static GPrivate pc = G_PRIVATE_INIT((GDestroyNotify)freecl);
	Client *c = g_private_get(&pc);

	if (!c)
	{
		c = makecl();
		g_private_set(&pc, c);
	}
	g_mutex_unlock(&m);
	return c;
}

static GHashTable *lastsec;
static GMutex lastm;
static void setsec(char *exe, int sec)
{
	g_mutex_lock(&lastm);
	if (!lastsec) lastsec = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	g_hash_table_replace(lastsec, g_strdup(exe), g_strdup_printf("%d", sec));
	g_mutex_unlock(&lastm);
}
static char *keepstr(char *exe)
{
	g_mutex_lock(&lastm);
	char *ret = g_hash_table_lookup(lastsec, exe);
	g_mutex_unlock(&lastm);

#define Z(v) #v
	return ret ?: Z(KEEPS);
#undef Z
}

static gboolean pingloop(Client *c)
{
	if (!ipcsend(c->exe, PING, CSping, c->pid, keepstr(c->exe)))
		g_mutex_unlock(&c->retm);

	return true;
}
static gboolean timeoutcb(Client *c)
{
	g_mutex_unlock(&c->retm);
	return false;
}

//don't free
static char *request(char *exe, Com type, bool caller, char *data)
{
	Client *c = getcl();

	if (caller)
	{
		g_mutex_lock(&c->retm);
		g_free(c->retdata);
		c->retdata = NULL;
	}

	if (!ipcsend(exe, INPUT, type, caller ? c->pid : NULL, data))
	{ //svr is not running
		char *path = ipcpath(exe, "lock");
		if (!g_file_test(path, G_FILE_TEST_EXISTS))
			mkdirif(path);

		int lock = open(path, O_RDONLY | O_CREAT, S_IRUSR);
		g_free(path);
		flock(lock, LOCK_EX);

		//retry in single proc
		if (!ipcsend(exe, INPUT, type, caller ? c->pid : NULL, data))
		{
			if (!caller)
				g_mutex_lock(&c->retm);

			GSource *tout = g_timeout_source_new(KEEPS * 1000);
			g_source_set_callback(tout, (GSourceFunc)timeoutcb, c, NULL);
			g_source_attach(tout, c->wctx);


			char **argv = g_new0(char*, 4);
			argv[0] = exe;
			argv[1] = PREFIX;
			argv[2] = c->pid;
			GError *err = NULL;
			if (!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH,
						NULL, NULL, NULL, &err))
			{
				g_print("err %s", err->message);
				g_error_free(err);
			}
			g_free(argv);


			g_mutex_lock(&c->retm);
			g_mutex_unlock(&c->retm);

			g_source_destroy(tout);
			g_source_unref(tout);

			//wyebloop doesn't know svr quits
			wyebkeep(exe, 0);

			if (caller)
				g_mutex_lock(&c->retm);
			if (!ipcsend(exe, INPUT, type, caller ? c->pid : NULL, data))
				P(Spawning %s failed !!, exe)
		}
		close(lock);
	}

	if (caller)
	{
		GSource *ping = g_timeout_source_new(DPINGTIME);
		c->exe = exe;
		g_source_set_callback(ping, (GSourceFunc)pingloop, c, NULL);
		g_source_attach(ping, c->wctx); //attach to ping thread

		g_mutex_lock(&c->retm);
		g_mutex_unlock(&c->retm);

		g_source_destroy(ping);
		g_source_unref(ping);
	}

	return c->retdata;
}

char *wyebget(char *exe, char *data)
{
	return request(exe, CSdata, true, data);
}
void wyebsend(char *exe, char *data)
{
	request(exe, CSdata, false, data);
}
static gboolean keepcb(char *exe)
{
	request(exe, CSuntil, false, keepstr(exe));
	return false;
}
void wyebkeep(char *exe, int sec)
{
	if (sec) setsec(exe, sec);
	g_idle_add_full(G_PRIORITY_DEFAULT, (GSourceFunc)keepcb,
			g_strdup(exe), g_free);
}

static gboolean loopcb(char *exe)
{
	keepcb(exe);
	return true;
}
guint wyebloop(char *exe, int sec)
{
	if (sec) setsec(exe, sec);
	loopcb(exe);
	return g_timeout_add_full(G_PRIORITY_DEFAULT, sec * 300,
			(GSourceFunc)loopcb, g_strdup(exe), g_free);
}


#if TESTER
static void testget(gpointer p, gpointer ap)
{
//	P(ret %s - %s, wyebget(ap, p), (char *)p)
	wyebget(ap, p);
	g_free(p);
}
#endif
static gboolean tcinputcb(GIOChannel *ch, GIOCondition cnd, char *exe)
{
	char *line;
	if (cnd == G_IO_HUP || G_IO_STATUS_EOF ==
			g_io_channel_read_line(ch, &line, NULL, NULL, NULL))
		exit(0);

	if (!line) return true;

	g_strstrip(line);
	if (!strlen(line))
		exit(0);

#if TESTER
	if (g_str_has_prefix(line, "l"))
	{
		GThreadPool *pool = g_thread_pool_new(testget, exe, 32, false, NULL);

		gint64 start = g_get_monotonic_time();
		for (int i = 0; i < 100000; i++)
		{
			char *is = g_strdup_printf("l%d", i);
			//g_print("loop %d ret %s\n", i, wyebget(exe, is));
			if (*(line + 1) == 's')
				wyebsend(exe, is);
			else if (*(line + 1) == 'p')
				D(pint %s, is)
			else if (*(line + 1) == 'c')
			{
				gchar *data;
				g_spawn_command_line_sync("echo 'ret'", &data, NULL, NULL, NULL);
				g_strchomp(data);
				D(cmd %s %s, is, data)
				g_free(data);
			}
			else
			{
//				wyebget(exe, is);
				g_thread_pool_push(pool, is, NULL);
				is = NULL;
			}

			g_free(is);
		}
		g_thread_pool_free(pool, false, true);

		gint64 now = g_get_monotonic_time();
		P(time %f, (now - start) / 1000000.0)
	}
	else
#endif
	P(%s, wyebget(exe, line))

	g_free(line);
	return true;
}
static gboolean tcinit(char *exe)
{
#if DEBUG
	wyebloop(exe, 300);
#else
	wyebloop(exe, 2);
#endif

	GIOChannel *io = g_io_channel_unix_new(fileno(stdin));
	g_io_add_watch(io, G_IO_IN | G_IO_HUP, (GIOFunc)tcinputcb, exe);

	return false;
}
void wyebclient(char *exe)
{
	GMainLoop *loop = g_main_loop_new(NULL, false);
	g_idle_add((GSourceFunc)tcinit, exe);
	g_main_loop_run(loop);
}



//@ipccb
gboolean ipccb(GIOChannel *ch, GIOCondition cnd, gpointer p)
{
	//D(ipccb %c, svrexe ? 'S':'C')
	char *line;
	g_io_channel_read_line(ch, &line, NULL, NULL, NULL);
	if (!line) return true;

	Com type  = *line;
	char *id  = line + 1;
	char *data = strchr(line, ':');
	*data++ = '\0';
	g_strchomp(data);

#if DEBUG
	static int i;
	D(%c ipccb%d %c/%s/%lu;, svrexe ? 'S':'C', i++, type ,id, strlen(data))
#endif

	switch (type) {
	//server
	case CSdata:
	{
		Dataargs *args = g_new(Dataargs, 1);
		args->caller = g_strdup(id);
		args->data = g_strcompress(data);

		static GThreadPool *pool;
		if (!pool) pool = g_thread_pool_new(getdata, NULL, -1, false, NULL);
		g_thread_pool_push(pool, args, NULL);
		break;
	}
	case CSping:
		g_mutex_lock(&ordersm);
		if (!g_hash_table_lookup(orders, id))
			ipcsend(CLIDIR, id, CClost, NULL, NULL);
		g_mutex_unlock(&ordersm);
	case CSuntil:
		until(atoi(data));
		break;

	//client
	case CCret:
	case CClost:
	case CCwoke:
	{
		Client *c = p;
		if (type == CCret)
			c->retdata = g_strcompress(data);

		//for the case pinging at same time of ret
		g_mutex_trylock(&c->retm);
		g_mutex_unlock(&c->retm);
		g_thread_yield();
		break;
	}
	}

	g_free(line);
	return true;
}



//test
#if TESTER
static char *testdata(char *data)
{
	//sleep(9);
	//g_free(data); //makes crash

	static int i;
	return g_strdup_printf("%d th test data. req is %s", ++i, data);
}

int main(int argc, char **argv)
{
//	start = g_get_monotonic_time();
//	gint64 now = g_get_monotonic_time();
//	D(time %ld %ld, now - start, now)

	if (!wyebsvr(argc, argv, testdata))
		wyebclient(argv[0]);

	exit(0);
}
#endif
