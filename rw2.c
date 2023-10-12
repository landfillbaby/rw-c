// vi: sw=2 tw=80
/* based on:
https://github.com/jridgewell/rw
moreutils sponge
Python _io_FileIO_readall_impl and shutil.copyfile */
#define _FILE_OFFSET_BITS 64
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#ifdef CHECKMEM
#include <errno.h>
#endif
#ifdef _WIN32 // TODO: test
#define _CRT_NONSTDC_NO_WARNINGS
#include <io.h>
#else
#include <unistd.h>
#define setmode(...) 0
#ifndef O_BINARY
#define O_BINARY 0
#endif
#endif
#ifdef DOFREE
#define F free(buf)
#else
#define F 0
#endif
#ifndef MEMSIZE
#ifdef __ANDROID__
#define MEMSIZE (1ull << 38) // 256 GiB
#elif sizeof(void *) < 8
#define MEMSIZE (1ull << 40) // 1 TiB
#else
#define MEMSIZE (1ull << 46) // 64 TiB
#endif
#endif
static int usage(void) {
#define W(x) write(2, x, sizeof(x) - 1)
  W("Usage: rw [[-a] FILE]\n\
Read stdin into memory then open and write to FILE or stdout.\n\
-a: append to file instead of overwriting.\n");
  return -1;
}
int main(int argc, char **argv) {
  setmode(0, O_BINARY);
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
  char *buf = malloc(MEMSIZE);
  if(!buf) {
    perror("ERROR reading");
    return 1;
  }
  size_t bytes_read = 0;
  ssize_t n;
  while((n = read(0, buf + bytes_read, MEMSIZE - bytes_read)) > 0) {
    bytes_read += (size_t)n;
#ifdef CHECKMEM
    if(bytes_read == MEMSIZE) {
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
#ifdef CHECKMEM
maxsize:;
#endif
  int f;
  if(!outname) {
    setmode(1, O_BINARY);
    f = 1;
  } else if((f = open(outname,
		 O_WRONLY | O_BINARY | O_CREAT | (append ? O_APPEND : O_TRUNC),
#ifdef _WIN32
		 S_IREAD | S_IWRITE
#else
		 S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH
#endif
		 ))
      == -1) {
    perror("ERROR writing");
    F;
    return 2;
  }
  for(char *ptr = buf; bytes_read;) { // TODO: loop formatting
    n = write(f, ptr, bytes_read);
    if(n < 0) {
      perror("ERROR writing");
      close(f);
      F;
      return 2;
    }
    bytes_read -= (size_t)n;
    ptr += (size_t)n;
  }
  if(close(f)) {
    perror("ERROR writing");
    F;
    return 2;
  }
  F;
}
