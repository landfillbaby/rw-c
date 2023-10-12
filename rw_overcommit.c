// vi: sw=2 tw=80
/* based on:
https://github.com/jridgewell/rw
moreutils sponge */
#ifdef _WIN32
#error "Windows doesn't do overcommit"
#endif
#define _FILE_OFFSET_BITS 64
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef CHECKMEM
#include <errno.h>
#endif
#ifndef RW_SIZE
#ifdef __ANDROID__
#define RW_SIZE (1ull << 38) // 256 GiB
#else
#define RW_SIZE (1ull << (sizeof(void *) < 8 ? 40 : 46)) // 1 or 64 TiB
#endif
#endif
#ifdef DOFREE
#define F free(buf)
#else
#define F 0
#endif
static int usage(void) {
#define W(x) write(2, x, sizeof(x) - 1)
  W("Usage: rw [[-a] FILE]\n\
Read stdin into memory then open and write to FILE or stdout.\n\
-a: append to file instead of overwriting.\n");
  return -1;
}
int main(int argc, char **argv) {
  char *outname = 0;
  bool append = 0;
#ifdef USE_GETOPT
  for(int opt; (opt = getopt(argc, argv, "a")) != -1;) {
    if(opt == 'a') append = 1;
    else return usage();
  }
  if(optind < argc) outname = argv[optind];
#else
  if(argc > 3) return usage();
  if(argc == 3) {
    if(*argv[1] != '-' || argv[1][1] != 'a' || argv[1][2] || *argv[2] == '-')
      return usage();
    append = 1;
    outname = argv[2];
  } else if(argc == 2) {
    if(*argv[1] == '-') return usage();
    outname = argv[1];
  }
#endif
  char *buf = malloc(RW_SIZE);
  if(!buf) {
    perror("ERROR reading");
    F;
    return 1;
  }
  size_t bytes_read = 0;
  ssize_t n;
  while((n = read(0, buf + bytes_read, RW_SIZE - bytes_read)) > 0) {
    bytes_read += (size_t)n;
#ifdef CHECKMEM
    if(bytes_read == RW_SIZE) {
      switch(read(0, buf, 1)) {
	case 0: goto maxsize;
	case 1: errno = EFBIG;
      }
      perror("ERROR reading");
      F;
      return 1;
    }
#endif
  }
  if(n < 0) {
    perror("ERROR reading");
    F;
    return 1;
  }
#ifdef SHRINK_BEFORE_WRITE
  if(!bytes_read) {
    free(buf);
#ifdef DOFREE
    buf = 0;
#endif
  } else if(bufsize > bytes_read) {
    char *newbuf = realloc(buf, bytes_read);
    if(!newbuf) {
      perror("ERROR reading");
      F;
      return 1;
    }
    buf = newbuf;
  }
#endif
#ifdef CHECKMEM
maxsize:;
#endif
  int f = outname
      ? open(outname, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC),
	  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
      : 1;
  if(f == -1) {
    perror("ERROR writing");
    F;
    return 2;
  }
  for(char *ptr = buf; bytes_read; bytes_read -= (size_t)n, ptr += (size_t)n)
    if((n = write(f, ptr, bytes_read)) < 0) {
      perror("ERROR writing");
      close(f);
      F;
      return 2;
    }
  if(close(f)) {
    perror("ERROR writing");
    F;
    return 2;
  }
  F;
}
