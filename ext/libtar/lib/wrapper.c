/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2003 Mark D. Roth
**  All rights reserved.
**
**  wrapper.c - libtar high-level wrapper code
**
**  Mark D. Roth <roth@uiuc.edu>
**  Campus Information Technologies and Educational Services
**  University of Illinois at Urbana-Champaign
*/

#include <internal.h>

#include <stdio.h>
#include <sys/param.h>
#include <dirent.h>
#include <errno.h>

#ifdef STDC_HEADERS
# include <string.h>
#endif

#ifdef HAVE_SIGACTION
#include <signal.h>
#endif

#if defined(_WIN32) || defined(HAVE_SIGACTION)
static int __tarruby_interrupted__ = 0;
#endif

#ifdef _WIN32
static BOOL WINAPI interrupted_handler(DWORD CtrlType) {
	if (CTRL_C_EVENT == CtrlType) {
		__tarruby_interrupted__ = 1;
		return FALSE;
	} else {
		return TRUE;
	}
}

void tarruby_interrupted() {
	SetConsoleCtrlHandler(interrupted_handler, TRUE);
}
#endif

#ifdef HAVE_SIGACTION
static void interrupted_handler(int no) {
	__tarruby_interrupted__ = 1;
}

void tarruby_interrupted() {
	char buff[256];
	int ret;
	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = interrupted_handler;
	sa.sa_flags |= SA_RESTART;
	sigaction(SIGINT, &sa, NULL);
}
#endif

int
tar_extract_glob(TAR *t, char *globname, char *prefix)
{
	char *filename;
	char buf[MAXPATHLEN];
	int i;

#if defined(_WIN32) || defined(HAVE_SIGACTION)
	__tarruby_interrupted__ = 0;
#endif

	while ((i = th_read(t)) == 0)
	{
#if defined(_WIN32) || defined(HAVE_SIGACTION)
		if (__tarruby_interrupted__) {
			errno = EINTR;
			__tarruby_interrupted__ = 0;
			return -1;
		}
#endif
		filename = th_get_pathname(t);
		if (fnmatch(globname, filename, FNM_PATHNAME | FNM_PERIOD))
		{
			if (TH_ISREG(t) && tar_skip_regfile(t)){
				free(filename);
				return -1;
			}
			continue;
		}
		if (t->options & TAR_VERBOSE)
			th_print_long_ls(t);
		if (prefix != NULL)
			snprintf(buf, sizeof(buf), "%s/%s", prefix, filename);
		else
			strlcpy(buf, filename, sizeof(buf));
		if (tar_extract_file(t, buf) != 0) {
			free(filename);
			return -1;
		}
		free(filename);
	}

	return (i == 1 ? 0 : -1);
}


int
tar_extract_all(TAR *t, char *prefix)
{
	char *filename;
	char buf[MAXPATHLEN];
	int i;

#if defined(_WIN32) || defined(HAVE_SIGACTION)
	__tarruby_interrupted__ = 0;
#endif

#ifdef DEBUG
	printf("==> tar_extract_all(TAR *t, \"%s\")\n",
	       (prefix ? prefix : "(null)"));
#endif

	while ((i = th_read(t)) == 0)
	{
#ifdef DEBUG
		puts("    tar_extract_all(): calling th_get_pathname()");
#endif
#if defined(_WIN32) || defined(HAVE_SIGACTION)
		if (__tarruby_interrupted__) {
			errno = EINTR;
			__tarruby_interrupted__ = 0;
			return -1;
		}
#endif
		filename = th_get_pathname(t);
		if (t->options & TAR_VERBOSE)
			th_print_long_ls(t);
		if (prefix != NULL)
			snprintf(buf, sizeof(buf), "%s/%s", prefix, filename);
		else
			strlcpy(buf, filename, sizeof(buf));
#ifdef DEBUG
		printf("    tar_extract_all(): calling tar_extract_file(t, "
		       "\"%s\")\n", buf);
#endif
		if (tar_extract_file(t, buf) != 0){
			free(filename);
			return -1;
		}
		free(filename);
	}

	return (i == 1 ? 0 : -1);
}


int
tar_append_tree(TAR *t, char *realdir, char *savedir)
{
	char realpath[MAXPATHLEN];
	char savepath[MAXPATHLEN];
	struct dirent *dent;
	DIR *dp;
	struct stat s;
#if defined(_WIN32)
	int errorp = 0;
#endif
#if defined(_WIN32) || defined(HAVE_SIGACTION)
	__tarruby_interrupted__ = 0;
#endif

#ifdef DEBUG
	printf("==> tar_append_tree(0x%lx, \"%s\", \"%s\")\n",
	       t, realdir, (savedir ? savedir : "[NULL]"));
#endif

	if (tar_append_file(t, realdir, savedir) != 0)
		return -1;

#ifdef DEBUG
	puts("    tar_append_tree(): done with tar_append_file()...");
#endif

#ifndef _WIN32
	dp = opendir(realdir);
#else
	dp = opendir0(realdir, &errorp);
#endif
	if (dp == NULL)
	{
#ifndef _WIN32
		if (errno == ENOTDIR)
			return 0;
#else
		if (errorp == ENOTDIR) {
			return 0;
		}
#endif
		return -1;
	}
	while ((dent = readdir(dp)) != NULL)
	{
#if defined(_WIN32) || defined(HAVE_SIGACTION)
		if (__tarruby_interrupted__) {
			errno = EINTR;
			__tarruby_interrupted__ = 0;
			return -1;
		}
#endif
		if (strcmp(dent->d_name, ".") == 0 ||
		    strcmp(dent->d_name, "..") == 0)
			continue;

		snprintf(realpath, MAXPATHLEN, "%s/%s", realdir,
			 dent->d_name);
		if (savedir)
			snprintf(savepath, MAXPATHLEN, "%s/%s", savedir,
				 dent->d_name);

		if (lstat(realpath, &s) != 0)
			return -1;

		if (S_ISDIR(s.st_mode))
		{
			if (tar_append_tree(t, realpath,
					    (savedir ? savepath : NULL)) != 0)
				return -1;
			continue;
		}

		if (tar_append_file(t, realpath,
				    (savedir ? savepath : NULL)) != 0)
			return -1;
	}

	closedir(dp);

	return 0;
}


