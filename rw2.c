// vi: sw=2 tw=80
// WIP !!
/* based on:
https://github.com/jridgewell/rw
moreutils sponge
Python _io_FileIO_readall_impl and shutil.copyfile */
#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
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
  int memfd = memfd_create("rw", 0);
  if(memfd == -1) {
    perror("ERROR reading");
    return 1;
  }
  ssize_t r = splice(0, 0, memfd, 0, SSIZE_MAX, 0);
  if(r < 0) {
    if(errno == EINVAL) r = copy_file_range(0, 0, memfd, 0, SSIZE_MAX, 0);
    if(r < 0) {
      perror("ERROR reading");
      close(memfd);
      return 1;
    }
  }
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
    close(memfd);
    return 2;
  }
  off64_t z = 0;
  ssize_t r2 = splice(memfd, &z, f, 0, (size_t)r, 0);
  if(r2 < 0) {
    if(errno == EINVAL) r2 = copy_file_range(memfd, &z, f, 0, (size_t)r, 0);
    if(r2 < 0) {
      perror("ERROR writing");
      close(f);
      close(memfd);
      return 2;
    }
  }
  if(r2 != r) W("ERROR writing: wrote less than read!");
  if(close(f)) {
    perror("ERROR writing");
    close(memfd);
    return 2;
  }
  close(memfd);
}
