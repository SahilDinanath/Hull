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
  int pathc = 2;
  char *path[100] = {"", "/bin/"};
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
      // line preprocessing
      // remove newline
      line[strcspn(line, "\n")] = 0;

      // check for redirect add spaces to make processing easier later on
      char *temp = malloc(sizeof(char) * 100);
      int count = 0;
      for (int i = 0; i < nread; i++) {
        if (line[i] == '>') {
          temp[count++] = ' ';
          temp[count++] = '>';
          temp[count++] = ' ';
          continue;
        }
        temp[count++] = line[i];
      }
      temp[count] = '\0';
      free(line);

      line = temp;

      // BUILTIN EXIT
      if (strcmp(line, "exit") == 0) {
        exit(EXIT_SUCCESS);
      }
      // TODO: when searching different paths probably need to make a for loop
      // appending bin to the end of each path and attempting to run that, if
      // none run then err. strcat(bin, path);
      char *argend = NULL;
      char *args[100] = {NULL};
      char *token = NULL;

      // process args
      char *tempargs[100] = {NULL};
      unsigned int argc = 0;
      for (argc = 0; (token = strsep(&line, " ")); argc++) {
        tempargs[argc] = token;
      }

      // remove extra whitespaces
      count = 0;
      int end = argc;
      for (int i = 0; i < end; i++) {
        if (*tempargs[i] == '\0') {
          argc--;
          continue;
        }
        args[count++] = tempargs[i];
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
        // reset pathc to base possible path ie: the empty string
        pathc = 1;
        path[0] = "";
        for (int i = 1; i < argc; i++) {
          path[i] = args[i];
          pathc++;
        }
        continue;
      }

      // search for executable in path
      int success = 0;
      // remember to initialize dest with null value before trying to
      // strcat.
      char binpath[100] = {""};
      for (int i = 0; i < pathc; i++) {
        strcat(binpath, path[i]);
        strcat(binpath, args[0]);

        if (access(binpath, F_OK) == 0) {
          success = 1;
          break;
        }
        memset(binpath, 0, strlen(binpath));
      }

      // if no path succeeds then continue
      if (success == 0) {
        errmsg();
        continue;
      }

      // EXECUTE COMMAND
      // args array suplied to execv needs to be terminated by null pointer
      args[argc] = argend;
      int pid = fork();
      if (pid == 0) {

        // BUILTIN redirect
        // redirect runs here due to changing of file descriptors
        // change file descriptor if redirect is used
        for (int i = 0; i < argc; i++) {
          if (strcmp(args[i], ">") == 0) {
            if (i + 2 != argc) {
              errmsg();
              return 1;
            }
            //
            // TODO: you need to basically get files fixed or smth
            int file = open(args[i + 1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

            dup2(file, 1);
            dup2(file, 2);
            close(file);
            // remove redirect from args list
            argc = i - 1;
            // modify the ending of args parameter list
            args[argc] = argend;
            break;
          }
        }

        if (execv(binpath, args) == -1) {
          return 1;
        }
        return 0;
      } else {
        wait(NULL);
      }
    }
  };

  free(line);
  fclose(stream);
  return 0;
}
