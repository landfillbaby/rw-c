// vi: sw=2 tw=80
/* based on:
https://github.com/jridgewell/rw
moreutils sponge
Python _io_FileIO_readall_impl and shutil.copyfile */
#define _FILE_OFFSET_BITS 64
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#if !defined CHECKMEM && SIZE_MAX < 1ull << 50
#define CHECKMEM
#endif
#ifdef _WIN32
#define _CRT_NONSTDC_NO_WARNINGS
#include <io.h>
#ifndef CHUNK
#define CHUNK (BUFSIZ > 0x100000ul ? BUFSIZ : 0x100000ul)
#endif
// TODO: fread fwrite?
#define ssize_t int
#define read(x, y, z) read(x, y, (unsigned)((z) > INT_MAX ? INT_MAX : (z)))
#define write(x, y, z) write(x, y, (unsigned)((z) > INT_MAX ? INT_MAX : (z)))
#else
#include <unistd.h>
#define setmode(...) ((void)0)
#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifndef CHUNK
#define CHUNK (BUFSIZ > 0x10000ul ? BUFSIZ : 0x10000ul)
#endif
#endif
#ifdef DOFREE
#define F free(buf)
#else
#define F ((void)0)
#endif
static int usage(void) {
#define W(x) ((void)!write(2, x "\n", sizeof(x))) // __wur is stupid
  W("Usage: rw [[-a] FILE]\n\
Read stdin into memory then open and write to FILE or stdout.\n\
-a: append to file instead of overwriting.");
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
  size_t bufsize = CHUNK;
  char *buf = malloc(bufsize);
  if(!buf) {
    perror("ERROR reading");
    F;
    return 1;
  }
  size_t bytes_read = 0;
  ssize_t n;
  while((n = read(0, buf + bytes_read, bufsize - bytes_read)) > 0) {
    bytes_read += (size_t)n;
    if(bytes_read == bufsize) {
#ifdef CHECKMEM
      if(bufsize == SIZE_MAX) {
	switch(read(0, buf, 1)) {
	  case 0: goto maxsize;
	  case 1:
	    W("ERROR reading: File too big");
	    F;
	    return 1;
	}
	perror("ERROR reading");
	F;
	return 1;
      }
#endif
      size_t addend = bufsize / 8;
      if(addend < CHUNK) addend = CHUNK;
#ifdef CHECKMEM
      if(bufsize + addend < bufsize || bufsize + addend > SIZE_MAX)
	bufsize = SIZE_MAX;
      else
#endif
	bufsize += addend;
      char *newbuf = realloc(buf, bufsize);
      if(!newbuf) {
	perror("ERROR reading");
	F;
	return 1;
      }
      buf = newbuf;
    }
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
