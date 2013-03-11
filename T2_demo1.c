#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

/*
 * ECE 453/653 SE 465 CS 447/647
 * Demo 1 for Tutorial 2
 * 
 * This demo is intended to show how to generate call graph using opt from
 * inside your C program. This program spawns "opt" in a separate process via
 * fork(). It captures the call graph from "opt"'s stderr and prints onto
 * stdin.
 * Run this demo on ecelinux only.
 *
 * Instructions:
 * 1. Compile this program
 * 2. Run this program, pass the path to any valid bitcode file as the first argument
 * 3. Observe the call graph is printed onto stdin
 *
 * */

enum 
{
  PIPE_READ = 0,
  PIPE_WRITE,
};

int main(int argc, char *argv[]) {
  /* pipe to connect opt's stderr and our stdin */
  int pipe_callgraph[2];

  /* pid of opt */
  int opt_pid;

  /* arguments */
  char *bc_file;
  int support;
  double confidence;

  /* check arguments */
  /* !!!parse arguments yourself!!! */
  bc_file = argv[1];

  /* create pipe and check if pipe succeeded */
  if (pipe(pipe_callgraph) < 0) {
    perror("pipe");
    return 1;
  }

  /* create child process */
  opt_pid = fork();
  if (!opt_pid) { /* child process, to spawn opt */

    /* close the read end, since opt only write */
    close(pipe_callgraph[PIPE_READ]);  

    /* bind pipe to stderr, and check */
    if (dup2(pipe_callgraph[PIPE_WRITE], STDERR_FILENO) < 0) {
      perror("dup2 pipe_callgraph");
      return 1;
    }

    /* print something to stderr */
    fprintf(stderr, "This is child, just before spawning opt with %s.\n", bc_file);

    /* spawn opt */
    if (execl("/usr/local/bin/opt", "opt", "-print-callgraph", bc_file, (char *)NULL) < 0) {
      perror("execl opt");
      return 1;
    }

    /* unreachable code */
    return 0;
  }

  /* parent process */
  

  /* close the write end, since we only read */
  close(pipe_callgraph[PIPE_WRITE]);

  /* since we don't need stdin, we simply replace stdin with the pipe */
  if (dup2(pipe_callgraph[PIPE_READ], STDIN_FILENO) < 0) {
    perror("dup2 pipe_callgraph");
    return 1;
  }

  /* we print w/e read from the pipe */
  char c = '\0';
  while (scanf("%c", &c) >= 1) {
    printf("%c", c);
  }

  /* "That's all folks." */
  return 0;
}






