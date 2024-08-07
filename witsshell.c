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
  int pathc = 1;
  char *path[100] = {"/bin/"};
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

      // BUILTIN EXIT
      if (strcmp(line, "exit") == 0) {
        exit(EXIT_SUCCESS);
      }
      /*
       * NixOS doesn't have a regular filesystem, so I have to execute bins
       * through env MAKE SURE TO MAKE IT WORK WITH REGULAR PATHS /bin/
       * /usr/bin/ etc etc etc TEST ON DISTROBOX
       * */
      // TODO: when searching different paths probably need to make a for loop
      // appending bin to the end of each path and attempting to run that, if
      // none run then err. strcat(bin, path);
      char *argend = NULL;
      char *args[100] = {NULL};
      char *token = NULL;

      // process args
      unsigned int argc = 0;
      for (argc = 0; (token = strsep(&line, " ")); argc++) {
        args[argc] = token;
      }

      // BUILTIN CD
      if (strcmp(args[0], "cd") == 0) {
        if (argc > 2 || chdir(args[1]) == -1) {
          errmsg();
          return 0;
        }
        continue;
      }

      // BUILTIN PATH

      if (strcmp(args[0], "path") == 0) {
        // exclude the first arg which is the bin
        // set path count to 0
        pathc = 0;
        for (int i = 1; i < argc; i++) {
          path[i - 1] = args[i];
          pathc++;
        }
        continue;
      }
      // preping data before fork
      int success = 0;
      for (int i = 0; i < pathc; i++) {
        // remember to initialize dest with null value before trying to
        // strcat.
        char bin[100] = {""};
        strcat(bin, path[i]);
        strcat(bin, args[0]);

        // args array suplied to execv needs to be terminated by null pointer
        args[argc] = argend;
        int pid = fork();
        if (pid == 0) {
          if (execv(bin, args) == -1) {
            errmsg();
            return 1;
          }
          return 0;
        } else {
          wait(&success);
          if (WEXITSTATUS(success)) {
            break;
          }
        }
      }
    }
  };

  free(line);
  fclose(stream);
  return 0;
}
