#include <stdio.h>

/*
 * ECE 453/653 SE 465 CS 447/647
 * Demo 2 for Tutorial 2
 *
 * This demo is intended to show the output format that veriy.sh expects
 * 
 * Instructions:
 * 1. Compile this file, name the binary "pipair"
 * 2. Put the compiled binary into the directory where "verify.sh" is
 * 3. Run verify.sh
 * 4. Observe one test cases pass.
 *
 * */

int main(int argc, char *argv[]) {
  printf("bug: A in scope2 pair: (A B) support: 3 confidence: 75.00%\n");
  printf("bug: A in scope3 pair: (A D) support: 3 confidence: 75.00%\n");
  printf("bug: B in scope3 pair: (B D) support: 4 confidence: 80.00%\n");
  printf("bug: D in scope2 pair: (B D) support: 4 confidence: 80.00%\n");

  return 0;
}
