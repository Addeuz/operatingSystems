#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {

  if(argc < 2) {
    printf("Not enough arguments!\n");
    exit(EXIT_FAILURE);
  }
  int fr = open(argv[1], O_RDONLY);
  int fw = open(argv[2], O_WRONLY);

  char c;
  int n;
  int writeResult;
  while((n = read(fr, &c, 1)) > 0){
    write(fw, &c, n);
  }
  

  close(fr);
  close(fw);

  return 0;
}
