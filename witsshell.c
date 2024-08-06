#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void errmsg() {
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}
int main(int MainArgc, char *MainArgv[]) {
  // FILE Settings
  // stdin: 0, stdout: 2, stderr: 3
  FILE *stream = stdin;
  char *line = NULL;
  size_t len = 0;
  ssize_t nread;
  //
  // SETTINGS
  char path[] = "/usr/bin/env";

  if (MainArgc > 2) {
    errmsg();
    exit(EXIT_FAILURE);
  }

  if (MainArgc == 2) {
    if ((stream = fopen(MainArgv[1], "r")) == NULL) {
      errmsg();
      exit(EXIT_FAILURE);
    }
  }

  printf("witsshell> ");
  while ((nread = getline(&line, &len, stream)) != -1) {
    // remove newline
    line[strcspn(line, "\n")] = 0;

    // INSERT BUILT-INS HERE
    // any string retrieved with getline ends with \n
    if (strcmp(line, "exit") == 0) {
      exit(EXIT_SUCCESS);
    }

    // fwrite(line, nread, 1, stdout);
    int pid = fork();

    if (pid == 0) {
      /*
       * NixOS doesn't have a regular filesystem, so I have to execute bins
       * through env MAKE SURE TO MAKE IT WORK WITH REGULAR PATHS /bin/
       * /usr/bin/ etc etc etc TEST ON DISTROBOX
       * */
      char *env = path;
      // when searching different paths probably need to make a for loop
      // appending bin to the end of each path and attempting to run that, if
      // none run then err. strcat(bin, path);
      //  strcat(bin, " ");
      //  strcat(bin, line);
      char *argend = NULL;
      char *args[100];
      char *token;

      unsigned int j = 0;
      for (j = 0; (token = strsep(&line, " ")); j++) {
        args[j] = token;
      }

      // args array suplied to execv needs to be terminated by null pointer
      args[j++] = argend;

      execv(env, args);
      exit(EXIT_SUCCESS);
    } else {
      wait(NULL);
    }

    printf("witsshell> ");
  }

  free(line);
  fclose(stream);
  exit(EXIT_SUCCESS);
}
