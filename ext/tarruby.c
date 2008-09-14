#include <fcntl.h>
#include <errno.h>
#ifdef HAVE_ZLIB_H
#include <zlib.h>
#endif
#ifdef HAVE_BZLIB_H
#include <bzlib.h>
#endif
#include "libtar.h"
#include "ruby.h"

#ifdef _WIN32
void tarruby_interrupted();
#endif

#ifndef RSTRING_PTR
#define RSTRING_PTR(s) (RSTRING(s)->ptr)
#endif
#ifndef RSTRING_LEN
#define RSTRING_LEN(s) (RSTRING(s)->len)
#endif

#ifdef TARRUBY_EXPORTS
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

#ifndef S_ISREG
#define S_ISREG(m) (((m) & (_S_IFMT)) == (_S_IFREG))
#endif

#ifndef O_ACCMODE
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)
#endif

#define INT2TIME(i) rb_funcall(rb_cTime, rb_intern("at"), 1, INT2NUM(i))

#define VERSION "0.1.3"

static VALUE Tar;
static VALUE Error;

struct tarruby_tar {
  TAR *tar;
  int extracted;
};

#ifdef HAVE_ZLIB_H
// copy from libtar.c
// Copyright 1998-2003 University of Illinois Board of Trustees
// Copyright 1998-2003 Mark D. Roth
static int gzopen_frontend(char *pathname, int oflags, int mode) {
  char *gzoflags;
  gzFile gzf;
#ifndef _WIN32
  int fd;
#endif

  switch (oflags & O_ACCMODE) {
  case O_WRONLY:
    gzoflags = "wb";
    break;

  case O_RDONLY:
    gzoflags = "rb";
    break;

  default:
  case O_RDWR:
    errno = EINVAL;
    return -1;
  }

#ifndef _WIN32
  fd = open(pathname, oflags, mode);

  if (fd == -1) {
    return -1;
  }

  if ((oflags & O_CREAT) && fchmod(fd, mode)) {
    return -1;
  }

  gzf = gzdopen(fd, gzoflags);
#else
  gzf = gzopen(pathname, gzoflags);
#endif
  if (!gzf) {
    errno = ENOMEM;
    return -1;
  }

  return (int) gzf;
}

static tartype_t gztype = {
  (openfunc_t)  gzopen_frontend,
  (closefunc_t) gzclose,
  (readfunc_t)  gzread,
  (writefunc_t) gzwrite
};
#endif

#ifdef HAVE_BZLIB_H
static int bzopen_frontend(char *pathname, int oflags, int mode) {
  char *bzoflags;
  BZFILE *bzf;
#ifndef _WIN32
  int fd;
#endif

  switch (oflags & O_ACCMODE) {
  case O_WRONLY:
    bzoflags = "wb";
    break;

  case O_RDONLY:
    bzoflags = "rb";
    break;

  default:
  case O_RDWR:
    errno = EINVAL;
    return -1;
  }

#ifndef _WIN32
  fd = open(pathname, oflags, mode);

  if (fd == -1) {
    return -1;
  }

  if ((oflags & O_CREAT) && fchmod(fd, mode)) {
    return -1;
  }

  bzf = BZ2_bzdopen(fd, bzoflags);
#else
  bzf = BZ2_bzopen(pathname, bzoflags);
#endif
  if (!bzf) {
    errno = ENOMEM;
    return -1;
  }

  return (int) bzf;
}

static tartype_t bztype = {
  (openfunc_t)  bzopen_frontend,
  (closefunc_t) BZ2_bzclose,
  (readfunc_t)  BZ2_bzread,
  (writefunc_t) BZ2_bzwrite
};
#endif

static void strip_sep(char *path) {
  int len = strlen(path);

  if (path[len - 1] == '/' || path[len - 1] == '\\') {
    path[len - 1] = '\0';
  }
}

static VALUE tarruby_tar_alloc(VALUE klass) {
  struct tarruby_tar *p = ALLOC(struct tarruby_tar);

  p->tar = NULL;
  p->extracted = 1;

  return Data_Wrap_Struct(klass, 0, -1, p);
}

/* */
static VALUE tarruby_close0(VALUE self, int abort) {
  struct tarruby_tar *p_tar;

  Data_Get_Struct(self, struct tarruby_tar, p_tar);

  if(tar_close(p_tar->tar) != 0 && abort) {
    rb_raise(Error, "Close archive failed: %s", strerror(errno));
  }

  return Qnil;
}

/* */
static VALUE tarruby_close(VALUE self) {
  return tarruby_close0(self, 1);
}

static VALUE tarruby_s_open0(int argc, VALUE *argv, VALUE self, tartype_t *tartype) {
  VALUE tar, pathname, oflags, mode, options;
  struct tarruby_tar *p_tar;
  char *s_pathname;
  int i_oflags, i_mode = 0644, i_options = 0;

  rb_scan_args(argc, argv, "22", &pathname, &oflags, &mode, &options);
  Check_Type(pathname, T_STRING);
  s_pathname = RSTRING_PTR(pathname);
  i_oflags = NUM2INT(oflags);
  if (!NIL_P(mode)) { i_mode = NUM2INT(mode); }
  if (!NIL_P(options)) { i_options = NUM2INT(options); }

  tar = rb_funcall(Tar, rb_intern("new"), 0);
  Data_Get_Struct(tar, struct tarruby_tar, p_tar);

  if (tar_open(&p_tar->tar, s_pathname, tartype, i_oflags, i_mode, i_options) == -1) {
    rb_raise(Error, "Open archive failed: %s", strerror(errno));
  }

  if (rb_block_given_p()) {
    VALUE retval;
    int status;

    retval = rb_protect(rb_yield, tar, &status);
    tarruby_close0(tar, 0);

    if (status != 0) {
      rb_jump_tag(status);
    }

    return retval;
  } else {
    return tar;
  }
}

/* */
static VALUE tarruby_s_open(int argc, VALUE *argv, VALUE self) {
  return tarruby_s_open0(argc, argv, self, NULL);
}

#ifdef HAVE_ZLIB_H
/* */
static VALUE tarruby_s_gzopen(int argc, VALUE *argv, VALUE self) {
  return tarruby_s_open0(argc, argv, self, &gztype);
}
#endif

#ifdef HAVE_BZLIB_H
/* */
static VALUE tarruby_s_bzopen(int argc, VALUE *argv, VALUE self) {
  return tarruby_s_open0(argc, argv, self, &bztype);
}
#endif

/* */
static VALUE tarruby_append_file(int argc, VALUE *argv, VALUE self) {
  VALUE realname, savename;
  struct tarruby_tar *p_tar;
  char *s_realname, *s_savename = NULL;

  rb_scan_args(argc, argv, "11", &realname, &savename);
  Check_Type(realname, T_STRING);
  s_realname = RSTRING_PTR(realname);
  strip_sep(s_realname);

  if (!NIL_P(savename)) {
    Check_Type(savename, T_STRING);
    s_savename = RSTRING_PTR(savename);
    strip_sep(s_savename);
  }

  Data_Get_Struct(self, struct tarruby_tar, p_tar);

  if (tar_append_file(p_tar->tar, s_realname, s_savename) != 0) {
    rb_raise(Error, "Append file failed: %s", strerror(errno));
  }

  return Qnil;
}

/* */
static VALUE tarruby_append_tree(int argc, VALUE *argv, VALUE self) {
  VALUE realdir, savedir;
  struct tarruby_tar *p_tar;
  char *s_realdir, *s_savedir = NULL;

  rb_scan_args(argc, argv, "11", &realdir, &savedir);
  Check_Type(realdir, T_STRING);
  s_realdir = RSTRING_PTR(realdir);
  strip_sep(s_realdir);

  if (!NIL_P(savedir)) {
    Check_Type(savedir, T_STRING);
    s_savedir = RSTRING_PTR(savedir);
    strip_sep(s_savedir);
  }

  Data_Get_Struct(self, struct tarruby_tar, p_tar);

  if (tar_append_tree(p_tar->tar, s_realdir, s_savedir) != 0) {
    rb_raise(Error, "Append tree failed: %s", strerror(errno));
  }

  return Qnil;
}

/* */
static VALUE tarruby_extract_file(VALUE self, VALUE realname) {
  struct tarruby_tar *p_tar;
  char *s_realname;

  Check_Type(realname, T_STRING);
  s_realname = RSTRING_PTR(realname);
  Data_Get_Struct(self, struct tarruby_tar, p_tar);

  if (tar_extract_file(p_tar->tar, s_realname) != 0) {
    rb_raise(Error, "Extract file failed: %s", strerror(errno));
  }

  p_tar->extracted = 1;

  return Qnil;
}

static int tarruby_extract_buffer0(char *buf, int len, void *data) {
  VALUE buffer = (VALUE) data;
  rb_str_buf_cat(buffer, buf, len);
  return 0;
}

/* */
static VALUE tarruby_extract_buffer(VALUE self) {
  VALUE buffer;
  struct tarruby_tar *p_tar;
  int i;

  Data_Get_Struct(self, struct tarruby_tar, p_tar);
  buffer = rb_str_new("", 0);

  if ((i = tar_extract_function(p_tar->tar, (void *) buffer,  tarruby_extract_buffer0)) == -1) {
    rb_raise(Error, "Extract buffer failed: %s", strerror(errno));
  }

  p_tar->extracted = 1;

  return (i == 0) ? buffer : Qnil;
}

/* */
static VALUE tarruby_extract_glob(int argc, VALUE *argv, VALUE self) {
  VALUE globname, prefix;
  struct tarruby_tar *p_tar;
  char *s_globname, *s_prefix = NULL;

  rb_scan_args(argc, argv, "11", &globname, &prefix);
  Check_Type(globname, T_STRING);
  s_globname = RSTRING_PTR(globname);

  if (!NIL_P(prefix)) {
    Check_Type(prefix, T_STRING);
    s_prefix = RSTRING_PTR(prefix);
  }

  Data_Get_Struct(self, struct tarruby_tar, p_tar);

  if (tar_extract_glob(p_tar->tar, s_globname, s_prefix) != 0) {
    rb_raise(Error, "Extract archive failed: %s", strerror(errno));
  }

  p_tar->extracted = 1;

  return Qnil;
}

/* */
static VALUE tarruby_extract_all(int argc, VALUE *argv, VALUE self) {
  VALUE prefix;
  struct tarruby_tar *p_tar;
  char *s_prefix = NULL;

  rb_scan_args(argc, argv, "01", &prefix);

  if (!NIL_P(prefix)) {
    Check_Type(prefix, T_STRING);
    s_prefix = RSTRING_PTR(prefix);
  }

  Data_Get_Struct(self, struct tarruby_tar, p_tar);

  if (tar_extract_all(p_tar->tar, s_prefix) != 0) {
    rb_raise(Error, "Extract archive failed: %s", strerror(errno));
  }

  p_tar->extracted = 1;

  return Qnil;
}

static void tarruby_skip_regfile_if_not_extracted(struct tarruby_tar *p) {
  if (!p->extracted) {
    if (TH_ISREG(p->tar) && tar_skip_regfile(p->tar) != 0) {
      rb_raise(Error, "Read archive failed: %s", strerror(errno));
    }

    p->extracted = 1;
  }
}

/* */
static VALUE tarruby_read(VALUE self) {
  struct tarruby_tar *p_tar;
  int i;

  Data_Get_Struct(self, struct tarruby_tar, p_tar);
  tarruby_skip_regfile_if_not_extracted(p_tar);

  if ((i = th_read(p_tar->tar)) == -1) {
    rb_raise(Error, "Read archive failed: %s", strerror(errno));
  }

  p_tar->extracted = 0;

  return (i == 0) ? Qtrue : Qfalse;
}

/* */
static VALUE tarruby_each(VALUE self) {
  struct tarruby_tar *p_tar;
  int i;

  Data_Get_Struct(self, struct tarruby_tar, p_tar);
  tarruby_skip_regfile_if_not_extracted(p_tar);

  while ((i = th_read(p_tar->tar)) == 0) {
    p_tar->extracted = 0;
    rb_yield(self);
    tarruby_skip_regfile_if_not_extracted(p_tar);
  }

  if (i == -1) {
    rb_raise(Error, "Read archive failed: %s", strerror(errno));
  }

  return Qnil;
}

/* */
static VALUE tarruby_crc(VALUE self) {
  struct tarruby_tar *p_tar;
  Data_Get_Struct(self, struct tarruby_tar, p_tar);
  return INT2NUM(th_get_crc(p_tar->tar));
}

/* */
static VALUE tarruby_size(VALUE self) {
  struct tarruby_tar *p_tar;
  Data_Get_Struct(self, struct tarruby_tar, p_tar);
  return INT2NUM(th_get_size(p_tar->tar));
}

/* */
static VALUE tarruby_mtime(VALUE self) {
  struct tarruby_tar *p_tar;
  Data_Get_Struct(self, struct tarruby_tar, p_tar);
  return INT2TIME(th_get_mtime(p_tar->tar));
}

/* */
static VALUE tarruby_devmajor(VALUE self) {
  struct tarruby_tar *p_tar;
  Data_Get_Struct(self, struct tarruby_tar, p_tar);
  return INT2NUM(th_get_devmajor(p_tar->tar));
}

/* */
static VALUE tarruby_devminor(VALUE self) {
  struct tarruby_tar *p_tar;
  Data_Get_Struct(self, struct tarruby_tar, p_tar);
  return INT2NUM(th_get_devminor(p_tar->tar));
}

/* */
static VALUE tarruby_linkname(VALUE self) {
  struct tarruby_tar *p_tar;
  Data_Get_Struct(self, struct tarruby_tar, p_tar);
  return rb_str_new2(th_get_linkname(p_tar->tar));
}

/* */
static VALUE tarruby_pathname(VALUE self) {
  struct tarruby_tar *p_tar;
  Data_Get_Struct(self, struct tarruby_tar, p_tar);
  return rb_str_new2(th_get_pathname(p_tar->tar));
}

/* */
static VALUE tarruby_mode(VALUE self) {
  struct tarruby_tar *p_tar;
  Data_Get_Struct(self, struct tarruby_tar, p_tar);
  return LONG2NUM(th_get_mode(p_tar->tar));
}

/* */
static VALUE tarruby_uid(VALUE self) {
  struct tarruby_tar *p_tar;
  Data_Get_Struct(self, struct tarruby_tar, p_tar);
  return LONG2NUM(th_get_uid(p_tar->tar));
}

/* */
static VALUE tarruby_gid(VALUE self) {
  struct tarruby_tar *p_tar;
  Data_Get_Struct(self, struct tarruby_tar, p_tar);
  return LONG2NUM(th_get_gid(p_tar->tar));
}

/* */
static VALUE tarruby_print(VALUE self) {
  struct tarruby_tar *p_tar;

  Data_Get_Struct(self, struct tarruby_tar, p_tar);
  th_print(p_tar->tar);

  return Qnil;
}

/* */
static VALUE tarruby_print_long_ls(VALUE self) {
  struct tarruby_tar *p_tar;

  Data_Get_Struct(self, struct tarruby_tar, p_tar);
  th_print_long_ls(p_tar->tar);

  return Qnil;
}

/* */
static VALUE tarruby_is_reg(VALUE self) {
  struct tarruby_tar *p_tar;

  Data_Get_Struct(self, struct tarruby_tar, p_tar);

  return TH_ISREG(p_tar->tar) ? Qtrue : Qfalse;
}

/* */
static VALUE tarruby_is_lnk(VALUE self) {
  struct tarruby_tar *p_tar;

  Data_Get_Struct(self, struct tarruby_tar, p_tar);

  return TH_ISLNK(p_tar->tar) ? Qtrue : Qfalse;
}

/* */
static VALUE tarruby_is_sym(VALUE self) {
  struct tarruby_tar *p_tar;

  Data_Get_Struct(self, struct tarruby_tar, p_tar);

  return TH_ISSYM(p_tar->tar) ? Qtrue : Qfalse;
}

/* */
static VALUE tarruby_is_chr(VALUE self) {
  struct tarruby_tar *p_tar;

  Data_Get_Struct(self, struct tarruby_tar, p_tar);

  return TH_ISCHR(p_tar->tar) ? Qtrue : Qfalse;
}

/* */
static VALUE tarruby_is_blk(VALUE self) {
  struct tarruby_tar *p_tar;

  Data_Get_Struct(self, struct tarruby_tar, p_tar);

  return TH_ISBLK(p_tar->tar) ? Qtrue : Qfalse;
}

/* */
static VALUE tarruby_is_dir(VALUE self) {
  struct tarruby_tar *p_tar;

  Data_Get_Struct(self, struct tarruby_tar, p_tar);

  return TH_ISDIR(p_tar->tar) ? Qtrue : Qfalse;
}

/* */
static VALUE tarruby_is_fifo(VALUE self) {
  struct tarruby_tar *p_tar;

  Data_Get_Struct(self, struct tarruby_tar, p_tar);

  return TH_ISFIFO(p_tar->tar) ? Qtrue : Qfalse;
}

/* */
static VALUE tarruby_is_longname(VALUE self) {
  struct tarruby_tar *p_tar;

  Data_Get_Struct(self, struct tarruby_tar, p_tar);

  return TH_ISLONGNAME(p_tar->tar) ? Qtrue : Qfalse;
}

/* */
static VALUE tarruby_is_longlink(VALUE self) {
  struct tarruby_tar *p_tar;

  Data_Get_Struct(self, struct tarruby_tar, p_tar);

  return TH_ISLONGLINK(p_tar->tar) ? Qtrue : Qfalse;
}

void DLLEXPORT Init_tarruby() {
  Tar = rb_define_class("Tar", rb_cObject);
  rb_define_alloc_func(Tar, tarruby_tar_alloc);
  rb_include_module(Tar, rb_mEnumerable);
  rb_funcall(Tar, rb_intern("private_class_method"), 1, ID2SYM(rb_intern("new")));

  Error = rb_define_class_under(Tar, "Error", rb_eStandardError);

  rb_define_const(Tar, "VERSION", rb_str_new2(VERSION));

  rb_define_const(Tar, "GNU",           INT2NUM(TAR_GNU));           /* use GNU extensions */
  rb_define_const(Tar, "VERBOSE",       INT2NUM(TAR_VERBOSE));       /* output file info to stdout */
  rb_define_const(Tar, "NOOVERWRITE",   INT2NUM(TAR_NOOVERWRITE));   /* don't overwrite existing files */
  rb_define_const(Tar, "IGNORE_EOT",    INT2NUM(TAR_IGNORE_EOT));    /* ignore double zero blocks as EOF */
  rb_define_const(Tar, "CHECK_MAGIC",   INT2NUM(TAR_CHECK_MAGIC));   /* check magic in file header */
  rb_define_const(Tar, "CHECK_VERSION", INT2NUM(TAR_CHECK_VERSION)); /* check version in file header */
  rb_define_const(Tar, "IGNORE_CRC",    INT2NUM(TAR_IGNORE_CRC));    /* ignore CRC in file header */

  rb_define_singleton_method(Tar, "open", tarruby_s_open, -1);
#ifdef HAVE_ZLIB_H
  rb_define_singleton_method(Tar, "gzopen", tarruby_s_gzopen, -1);
#endif
#ifdef HAVE_BZLIB_H
  rb_define_singleton_method(Tar, "bzopen", tarruby_s_bzopen, -1);
#endif
  rb_define_method(Tar, "close", tarruby_close, 0);
  rb_define_method(Tar, "append_file", tarruby_append_file, -1);
  rb_define_method(Tar, "append_tree", tarruby_append_tree, -1);
  rb_define_method(Tar, "extract_file", tarruby_extract_file, 1);
  rb_define_method(Tar, "extract_buffer", tarruby_extract_buffer, 0);
  rb_define_method(Tar, "extract_glob", tarruby_extract_glob, -1);
  rb_define_method(Tar, "extract_all", tarruby_extract_all, -1);
  rb_define_method(Tar, "read", tarruby_read, 0);
  rb_define_method(Tar, "each", tarruby_each, 0);
  rb_define_method(Tar, "crc", tarruby_crc, 0);
  rb_define_method(Tar, "size", tarruby_size, 0);
  rb_define_method(Tar, "mtime", tarruby_mtime, 0);
  rb_define_method(Tar, "devmajor", tarruby_devmajor, 0);
  rb_define_method(Tar, "devminor", tarruby_devminor, 0);
  rb_define_method(Tar, "linkname", tarruby_linkname, 0);
  rb_define_method(Tar, "pathname", tarruby_pathname, 0);
  rb_define_method(Tar, "mode", tarruby_mode, 0);
  rb_define_method(Tar, "uid", tarruby_uid, 0);
  rb_define_method(Tar, "gid", tarruby_gid, 0);
  rb_define_method(Tar, "print", tarruby_print, 0);
  rb_define_method(Tar, "print_long_ls", tarruby_print_long_ls, 0);
  rb_define_method(Tar, "reg?", tarruby_is_reg, 0);
  rb_define_method(Tar, "lnk?", tarruby_is_lnk, 0);
  rb_define_method(Tar, "sym?", tarruby_is_sym, 0);
  rb_define_method(Tar, "chr?", tarruby_is_chr, 0);
  rb_define_method(Tar, "blk?", tarruby_is_blk, 0);
  rb_define_method(Tar, "dir?", tarruby_is_dir, 0);
  rb_define_method(Tar, "fifo?", tarruby_is_fifo, 0);
  rb_define_method(Tar, "longname?", tarruby_is_longname, 0);
  rb_define_method(Tar, "longlink?", tarruby_is_longlink, 0);
#if defined(_WIN32) || defined(HAVE_SIGACTION)
  tarruby_interrupted();
#endif
}
