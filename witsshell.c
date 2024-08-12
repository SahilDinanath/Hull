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

int checkforchar(char *line, int nread) {
  for (int i = 0; i < nread; i++) {
    if (line[i] != ' ') {
      return 1;
    }
  }
  return 0;
}

char *redirectformatting(char *line, int nread) {
  // check for redirect add spaces to make processing easier later on
  char *temp = malloc(sizeof(char) * nread * 2);
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
  return temp;
}
void removearrspaces(char **des, char **src, unsigned int *argc) {
  int count = 0;
  int end = *argc;
  for (int i = 0; i < end; i++) {
    if (*src[i] == '\0') {
      *argc -= 1;
      continue;
    }
    des[count++] = src[i];
  }
}
int processredirect(char **args, unsigned int *argc) {

  for (int i = 0; i < *argc; i++) {
    if (strcmp(args[i], ">") == 0) {
      if (i + 2 != *argc) {
        errmsg();
        return 1;
      }
      int file = open(args[i + 1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

      dup2(file, 1);
      dup2(file, 2);
      close(file);
      // remove redirect from args list
      *argc = i;
      // modify the ending of args parameter list
      args[*argc] = NULL;
      break;
    }
  }
  return 0;
}
// TODO: move searching for bin into function
/* int searchforbin(char **binpath, char **path, int pathc, char **args) { */
/*   for (int i = 0; i < pathc; i++) { */
/*     strcat(*binpath, path[i]); */
/*     // Adds slash at end if path doesn't contain it. */
/*     // NOTE: using a gdb, it has been seen this doesn't work correctly, */
/*     // it adds a / regardless eg: tests//ls. However the function access */
/*     // seems to be / invariant. So the code still works.... */
/*     if (path[i][strlen(path[i])] != '/') { */
/*       strcat(*binpath, "/"); */
/*     } */
/*     strcat(*binpath, args[0]); */
/**/
/*     if (access(*binpath, F_OK) == 0) { */
/*       return 0; */
/*     } */
/*     memset(*binpath, 0, strlen(*binpath)); */
/*   } */
/*   return 1; */
/* } */
int main(int MainArgc, char *MainArgv[]) {
  setbuf(stdout, NULL);
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
      line[strcspn(line, "\n")] = '\0';
      nread--;

      char *comv[100] = {NULL};
      char *com = NULL;
      unsigned int comc = 0;
      for (comc = 0; (com = strsep(&line, "&")); comc++) {
        comv[comc] = com;
      }

      for (int c = 0; c < comc; c++) {
        line = comv[c];

        if (!checkforchar(line, strlen(line))) {
          continue;
        }

        line = redirectformatting(line, strlen(line));

        // BUILTIN EXIT
        if (strcmp(line, "exit") == 0) {
          exit(EXIT_SUCCESS);
        }

        char *argend = NULL;
        char *args[100] = {NULL};
        char *token = NULL;
        char *tempargs[100] = {NULL};

        // process args
        unsigned int argc = 0;
        for (argc = 0; (token = strsep(&line, " ")); argc++) {
          tempargs[argc] = token;
        }

        // strsep will only delete seperate 1 space between, so if extra spaces
        // are inbetween it will be added as a token.
        // remove extra whitespaces
        removearrspaces(args, tempargs, &argc);

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
          // Adds slash at end if path doesn't contain it.
          // NOTE: using a gdb, it has been seen this doesn't work correctly, it
          // adds a / regardless eg: tests//ls. However the function access
          // seems to be / invariant. So the code still works....
          if (path[i][strlen(path[i])] != '/') {
            strcat(binpath, "/");
          }
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
          if (processredirect(args, &argc) == 1) {
            return 1;
          }

          if (execv(binpath, args) == -1) {
            return 1;
          }
          return 0;
        }
      }
      // wait for all children to finish processing, wait will return -1 if
      // there's no children left to wait for
      while (wait(NULL) > 0)
        ;
    }
  };

  free(line);
  fclose(stream);
  return 0;
}
