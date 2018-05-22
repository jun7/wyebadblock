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

#ifndef _WYEBRUN_H
#define _WYEBRUN_H

#include <stdbool.h>
#include <glib/gstdio.h>

#define WYEBPREFIX "-wyeb"
#define WYEBDUNTIL 3

//client
//wyebrun spawns the exe if wyebsend failes
char *wyebreq(   char *exe, char *req);
void  wyebsend(  char *exe, char *req);
void  wyebuntil( char *exe, int   sec); //keep alive. default is 3s
//loop the wyebuntil. to stop, use g_source_remove
guint wyebloop(  char *exe, int   sec, int loopsec);
//send stdin to svr and ret data to stdout
//blank and enter exit it
void  wyebclient(char *exe);

//server
typedef char *(*wyebdataf)(char *req);
//server is spawned with an arg the caller
bool wyebsvr(int argc, char **argv, wyebdataf func);
//or if there is own GMainLoop
void wyebwatch(char *exe, char *caller, wyebdataf func);
//the caller is used to send the res meaning we are ready.
//3 sec left or client will die


#endif //_WYEBRUN_H
