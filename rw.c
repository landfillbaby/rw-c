// vi: sw=2 tw=80
/* based on:
https://github.com/jridgewell/rw
moreutils sponge
Python _io_FileIO_readall_impl and shutil.copyfile
*/
#define _FILE_OFFSET_BITS 64
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef CHECKMEM
#include <errno.h>
#endif
#ifdef _WIN32
#define _CRT_NONSTDC_NO_WARNINGS
#include <fcntl.h>
#include <io.h>
#ifndef CHUNK
#define CHUNK (BUFSIZ > (1 << 20) ? BUFSIZ : (1 << 20))
#endif
#else
#include <unistd.h>
#define setmode(...) 0
#ifndef CHUNK
#define CHUNK (BUFSIZ > (1 << 16) ? BUFSIZ : (1 << 16))
#endif
#endif
static int usage(void) {
  fputs("Usage: rw [[-a] FILE]\n\
Read stdin into memory then open and write to FILE or stdout.\n\
-a: append to file instead of overwriting.\n",
      stderr);
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
  else if(argc == 3) {
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
    perror("ERROR");
    return 1;
  }
  size_t bytes_read = 0;
  ssize_t n;
  while((n = read(0, buf + bytes_read, bufsize - bytes_read)) > 0) {
    bytes_read += (size_t)n;
    if(bytes_read == bufsize) {
#ifdef CHECKMEM
      if(bufsize == SIZE_MAX) {
	if(!read(0, buf, 1)) goto maxsize;
	if(!errno) errno = EFBIG;
	perror("ERROR");
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
      buf = realloc(buf, bufsize);
      if(!buf) {
	perror("ERROR");
	return 1;
      }
    }
  }
  if(n < 0) {
    perror("ERROR");
    return 1;
  }
#ifdef MINIFY_BEFORE_WRITE
  if(bufsize > bytes_read) {
    buf = realloc(buf, bytes_read);
    if(!buf) {
      perror("ERROR");
      return 1;
    }
  }
#endif
#ifdef CHECKMEM
maxsize:;
#endif
  FILE *f; // TODO: int fd?
  if(!outname) {
    setmode(1, O_BINARY);
    f = stdout;
  } else if(!(f = fopen(outname, append ? "ab" : "wb"))) {
    perror("ERROR");
    return 1;
  }
  if(bytes_read && !fwrite(buf, 1, bytes_read, f)) {
    perror("ERROR");
    fclose(f);
    return 1;
  }
  if(fclose(f)) {
    perror("ERROR");
    return 1;
  }
}
