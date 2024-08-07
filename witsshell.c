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
  ssize_t nread = 0;
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

  while (1) {
    if (interactive) {
      printf("witsshell> ");
    }

    if ((nread = getline(&line, &len, stream)) != -1) {
      // remove newline
      line[strcspn(line, "\n")] = 0;

      if (strcmp(line, "exit") == 0) {
        exit(EXIT_SUCCESS);
      }
      /*
       * NixOS doesn't have a regular filesystem, so I have to execute bins
       * through env MAKE SURE TO MAKE IT WORK WITH REGULAR PATHS /bin/
       * /usr/bin/ etc etc etc TEST ON DISTROBOX
       * */
      char bin[1000] = {""};
      // TODO: when searching different paths probably need to make a for loop
      // appending bin to the end of each path and attempting to run that, if
      // none run then err. strcat(bin, path);
      char *argend = NULL;
      char *args[100] = {NULL};
      char *token = NULL;

      // process args
      unsigned int j = 0;
      for (j = 0; (token = strsep(&line, " ")); j++) {
        args[j] = token;
      }

      // TODO: you need to implement directory switching as a builtin
      // you need to process the arguments,
      // so your next goal is to extract the line parsing code
      // so you can call it in this if statement
      if (strcmp(args[0], "cd") == 0) {
        if (j > 2) {
          errmsg();
          exit(EXIT_FAILURE);
        }
        if (chdir(args[1]) == -1) {
          errmsg();
          exit(EXIT_FAILURE);
        }
        continue;
      }

      // preping data before fork
      // remember to initialize dest with null value before trying to strcat.
      strcat(bin, path);
      strcat(bin, args[0]);

      // args array suplied to execv needs to be terminated by null pointer
      args[j] = argend;
      // need to exclude first arg as that is the command
      args[0] = "";
      // fwrite(line, nread, 1, stdout);
      int pid = fork();
      if (pid == 0) {
        if (execv(bin, args) == -1) {
          errmsg();
          exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
      } else {
        wait(NULL);
      }
    }
  };

  free(line);
  fclose(stream);
  exit(EXIT_SUCCESS);
}
