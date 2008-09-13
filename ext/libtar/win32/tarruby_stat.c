#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>

int tarruby_stat(const char *path, struct stat *buf) {
  int len;
  char *path0;

  len = strlen(path);

  if (path[len - 1] == '/' || path[len - 1] == '\\') {
    path0 = _alloca(len);
    memcpy_s(path0, len, path, len - 1);
    path0[len - 1] = '\0';
    return stat(path0, buf);
  } else {
    return stat(path, buf);
  }
}
