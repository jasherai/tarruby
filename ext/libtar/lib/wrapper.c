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

#ifdef _WIN32
static int __tarruby_is_interrupted__ = 0;

static BOOL WINAPI interrupted_handler(DWORD CtrlType) {
	if (CTRL_C_EVENT == CtrlType) {
		__tarruby_is_interrupted__ = 1;
		return FALSE;
	} else {
		return TRUE;
	}
}

void tarruby_interrupted() {
	SetConsoleCtrlHandler(interrupted_handler, TRUE);
}
#endif

int
tar_extract_glob(TAR *t, char *globname, char *prefix)
{
	char *filename;
	char buf[MAXPATHLEN];
	int i;

	while ((i = th_read(t)) == 0)
	{
#ifdef _WIN32
		if (__tarruby_is_interrupted__) {
			errno = EINTR;
			__tarruby_is_interrupted__ = 0;
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
		// modified by SUGAWARA Genki <sgwr_dts@yahoo.co.jp>
		//if (tar_extract_file(t, filename) != 0)
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

#ifdef DEBUG
	printf("==> tar_extract_all(TAR *t, \"%s\")\n",
	       (prefix ? prefix : "(null)"));
#endif

	while ((i = th_read(t)) == 0)
	{
#ifdef DEBUG
		puts("    tar_extract_all(): calling th_get_pathname()");
#endif
#ifdef _WIN32
		if (__tarruby_is_interrupted__) {
			errno = EINTR;
			__tarruby_is_interrupted__ = 0;
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
#ifdef _WIN32 // modified by SUGAWARA Genki <sgwr_dts@yahoo.co.jp>
    int errorp = 0;
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

#ifndef _WIN32 // modified by SUGAWARA Genki <sgwr_dts@yahoo.co.jp>
	dp = opendir(realdir);
#else
	dp = opendir0(realdir, &errorp);
#endif
	if (dp == NULL)
	{
#ifndef _WIN32 // modified by SUGAWARA Genki <sgwr_dts@yahoo.co.jp>
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
#ifdef _WIN32
		if (__tarruby_is_interrupted__) {
			errno = EINTR;
			__tarruby_is_interrupted__ = 0;
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


