// vi: sw=2 tw=80
/* based on:
https://github.com/jridgewell/rw
moreutils sponge
Python _io_FileIO_readall_impl
*/
#define _FILE_OFFSET_BITS 64
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#if BUFSIZ < 8192
#define CHUNK 8192
#elif(BUFSIZ >= (1 << 26))
#error "unreasonable BUFSIZ > 64 MiB defined"
#else
#define CHUNK BUFSIZ
#endif
#ifdef _WIN32
#error "Windows TODO"
#endif
static int usage(void) {
  fputs("Usage: rw [[-a] FILE]\n\
Read stdin into memory then open and write to FILE or stdout.\n\
-a: append to file instead of overwriting.\n",
      stderr);
  return -1;
}
int main(int argc, char **argv) {
  char *outname = 0;
  bool append = 0;
#ifdef USE_GETOPT
  int opt;
  while((opt = getopt(argc, argv, "a")) != -1) {
    if(opt == 'a') append = 1;
    else
      return usage();
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
  while(1) {
    if(bytes_read >= bufsize) {
      size_t old_bufsize = bufsize, addend;
      if(bytes_read > 65536) addend = bytes_read >> 3;
      else
	addend = 256 + bytes_read;
      if(addend < CHUNK) addend = CHUNK;
      if(addend + bytes_read < bytes_read) bufsize = SIZE_MAX;
      else
	bufsize = addend + bytes_read;
      if(old_bufsize < bufsize) {
	buf = realloc(buf, bufsize);
	if(!buf) {
	  perror("ERROR");
	  return 1;
	}
      }
    }
    ssize_t n = read(0, buf + bytes_read, bufsize - bytes_read);
    if(!n) break;
    if(n < 0) {
      perror("ERROR");
      return 1;
    }
    bytes_read += (size_t)n;
  }
  FILE *f = outname ? fopen(outname, append ? "ab" : "wb") : stdout;
  if(!f) {
    perror("ERROR");
    return 1;
  }
  while(bytes_read) {
    size_t n = fwrite(buf, 1, bytes_read, f);
    if(!n) {
      perror("ERROR");
      fclose(f);
      return 1;
    }
    bytes_read -= n;
  }
  if(fclose(f)) {
    perror("ERROR");
    return 1;
  }
}
