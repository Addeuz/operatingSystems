#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if(argc < 1) {
    printf("Not enough arguments!");
    exit(EXIT_FAILURE);
  }
  
  FILE *fr = fopen(argv[1], "r");
  FILE *fw = fopen(argv[2], "w");

  int c;
  while((c = getc(fr)) != EOF) {
    putc(c, fw);
  }

  fclose(fw);
  fclose(fr);

}
