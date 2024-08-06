#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int MainArgc, char *MainArgv[]) {
  // stdin: 0, stdout: 2, stderr: 3
  FILE *stream = stdin;
  char *line = NULL;
  size_t len = 0;
  ssize_t nread;

  if (MainArgc > 2) {
    fprintf(stderr, "Expected 1 argument, recieved %d", MainArgc);
    exit(EXIT_FAILURE);
  }

  if (MainArgc == 2) {
    if ((stream = fopen(MainArgv[1], "r")) == NULL) {
      fprintf(stderr, "File %s not found.", MainArgv[1]);
      exit(EXIT_FAILURE);
    }
  }

  printf("witsshell> ");
  while ((nread = getline(&line, &len, stream)) != -1) {
    // fwrite(line, nread, 1, stdout);

    // any string retrieved with getline ends with \n
    if (strcmp(line, "exit\n") == 0) {
      exit(EXIT_SUCCESS);
    }
    printf("witsshell> ");
  }

  free(line);
  fclose(stream);
  exit(EXIT_SUCCESS);
}
