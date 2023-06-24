
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h> // for inbuilt chdir(),fork(),exec(),pid_t functions
#include <stdlib.h> // for malloc(),realloc(),free(),exit(),execvp(),EXIT_SUCCESS, EXIT_FAILURE
#include <stdio.h>
#include <string.h> //strcmp(),strtok()
#define ShellGaurd_TOK_BUFSIZE 64
#define ShellGaurd_TOK_DELIM " \t\r\n\a"

// initial declarations
int ShellGaurd_cd(char **args);
int ShellGaurd_help(char **args);
int ShellGaurd_exit(char **args);
int sh_bg(char **args);
int ShellGaurd_history(char **args);

// builtins
char *builtin_str[] = {
    "cd",
    "help",
    "exit",
    "bg",
    "history"};
// pointers to respective functions
int (*builtin_func[])(char **) = {
    &ShellGaurd_cd,
    &ShellGaurd_help,
    &ShellGaurd_exit,
    &sh_bg,
    &ShellGaurd_history};

int ShellGaurd_num_builtins()
{
  return sizeof(builtin_str) / sizeof(char *);
}
// implementing cd builtin
int ShellGaurd_cd(char **args)
{
  if (args[1] == NULL)
  {
    fprintf(stderr, "ShellGaurd: expected argument to \"cd\"\n");
  }
  else
  {
    if (chdir(args[1]) != 0)
    {
      perror("ShellGaurd");
    }
  }
  return 1;
}
// gives list of available builtins and how to operate
int ShellGaurd_help(char **args)
{
  int i;
  printf("ACM's very own shell\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < ShellGaurd_num_builtins(); i++)
  {
    printf("  %s\n", builtin_str[i]);
  }

  return 1;
}

// a struct for history's linked list:
struct Node
{
  char *str;
  struct Node *next;
};
struct Node *head = NULL;
struct Node *cur = NULL;
// implementing utility functions for history:
char *strAppend(char *str1, char *str2)
{
  char *str3 = (char *)malloc(sizeof(char *) * (strlen(str1) + strlen(str2)));
  strcpy(str3, str1);
  strcat(str3, str2);
  return str3;
}
void add_to_hist(char **args)
{
  if (head == NULL)
  {
    head = (struct Node *)malloc(sizeof(struct Node));
    head->str = (char *)malloc(0x1000);
    char *str1 = " ";
    if (args[1] == NULL)
      strcpy(head->str, strAppend(args[0], str1));
    else
    {
      strcpy(head->str, strAppend(args[0], str1));
      strcpy(head->str, strAppend(head->str, args[1]));
    }
    head->next = NULL;
    cur = head;
  }
  else
  {
    struct Node *ptr = (struct Node *)malloc(sizeof(struct Node));
    cur->next = ptr;
    ptr->str = (char *)malloc(0x1000);
    char *str1 = " ";
    if (args[1] == NULL)
      strcpy(ptr->str, strAppend(args[0], str1));
    else
    {
      strcpy(ptr->str, strAppend(args[0], str1));
      strcpy(ptr->str, strAppend(ptr->str, args[1]));
    }
    ptr->next = NULL;
    cur = ptr;
  }
}
// creating a function to display history:

int ShellGaurd_history(char **args)
{
  struct Node *ptr = head;
  int i = 1;
  while (ptr != NULL)
  {
    printf(" %d %s\n", i++, ptr->str);
    ptr = ptr->next;
  }
  return 1;
}

// exit ShellGaurd
int ShellGaurd_exit(char **args)
{
  return 0;
}
// bg command
int sh_bg(char **args)
{
  ++args;
  char *firstCmd = args[0]; // echo
  int childpid = fork();
  if (childpid >= 0)
  {
    if (childpid == 0)
    {
      if (execvp(firstCmd, args) < 0)
      {
        perror("Error on execvp\n");
        exit(0);
      }
    }
  }
  else
  {
    perror("fork() error");
  }
  return 1;
}

// some processes such as all the builtins just change the properties of
// spawned child and not of the parent process.For them we have builtins and for
// rest, we have following:
int ShellGaurd_launch(char **args)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0)
  {
    // Child process
    if (execvp(args[0], args) == -1)
    {
      perror("ShellGaurd");
    }
    exit(EXIT_FAILURE);
  }
  else if (pid < 0)
  {
    // Error forking
    perror("ShellGaurd");
  }
  else
  {
    // Parent process
    do
    {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

// executing the builtins and calling launc() for rest.
int ShellGaurd_execute(char **args)
{

  int i;

  if (args[0] == NULL)
  {
    // An empty command was entered.
    return 1;
  }
  // search for builtins
  for (i = 0; i < ShellGaurd_num_builtins(); i++)
  {
    if (strcmp(args[0], builtin_str[i]) == 0)
    {
      return (*builtin_func[i])(args);
    }
  }

  return ShellGaurd_launch(args);
}
// reading the input via standard procedure of allocation-reallocation
char *ShellGaurd_read_line(void)
{
#define ShellGaurd_RL_BUFSIZE 1024
  int bufsize = ShellGaurd_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize); // contains input
  int c;

  if (!buffer)
  {
    fprintf(stderr, "ShellGaurd: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1)
  {
    // Read a character
    c = getchar();

    if (c == EOF)
    {
      exit(EXIT_SUCCESS);
    }
    else if (c == '\n')
    {
      buffer[position] = '\0';
      return buffer;
    }
    else
    {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize)
    {
      bufsize += ShellGaurd_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer)
      {
        fprintf(stderr, "ShellGaurd: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

// parsing the input and separating via separator(in this case a blanck space)
char **ShellGaurd_split_line(char *line)
{
  int bufsize = ShellGaurd_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char *)); // strores the different parts of command
  char *token, **tokens_backup;

  if (!tokens)
  {
    fprintf(stderr, "ShellGaurd: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, ShellGaurd_TOK_DELIM);
  while (token != NULL)
  {
    tokens[position] = token;
    position++;

    if (position >= bufsize)
    {
      bufsize += ShellGaurd_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char *));
      if (!tokens)
      {
        free(tokens_backup);
        fprintf(stderr, "ShellGaurd: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, ShellGaurd_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}
// loop till termination
void ShellGaurd_loop(void)
{
  char *line;
  char **args;
  int status;

  do
  {
    printf("> ");
    line = ShellGaurd_read_line();
    args = ShellGaurd_split_line(line);
    add_to_hist(args);
    status = ShellGaurd_execute(args);

    free(line);
    free(args);
  } while (status);
}

int main(int argc, char **argv)
{
  // currcomm=0;
  ShellGaurd_loop();
  return EXIT_SUCCESS;
}
