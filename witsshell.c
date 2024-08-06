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
  char path[] = "/bin/";
  int interactive = 1;

  if (MainArgc > 2) {
    errmsg();
    exit(EXIT_FAILURE);
  }

  if (MainArgc == 2) {
    interactive = 0;
    if ((stream = fopen(MainArgv[1], "r")) == NULL) {
      errmsg();
      exit(EXIT_FAILURE);
    }
  }

  do {
    if (interactive) {
      printf("witsshell> ");
    }

    if ((nread = getline(&line, &len, stream)) != -1) {
      // remove newline
      line[strcspn(line, "\n")] = '\0';

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
        char bin[1000];
        // TODO: when searching different paths probably need to make a for loop
        // appending bin to the end of each path and attempting to run that, if
        // none run then err. strcat(bin, path);
        char *argend = NULL;
        char *args[100];
        char *token;

        unsigned int j = 0;
        for (j = 0; (token = strsep(&line, " ")); j++) {
          args[j] = token;
        }
        strcat(bin, path);
        strcat(bin, args[0]);

        // args array suplied to execv needs to be terminated by null pointer
        args[j++] = argend;

        if (execv(bin, args) == -1) {
          errmsg();
          exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
      } else {
        wait(NULL);
      }
    }

  } while (1);

  free(line);
  fclose(stream);
  exit(EXIT_SUCCESS);
}
