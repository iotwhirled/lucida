#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>

#include "../../timer/timer.h"
#include "slre.h"

#define EXPRESSIONS 100
#define QUESTIONS 400
#define MAXCAPS 60000

/* Data */
char *exps[256];
struct slre *slre[512];
char *bufs[MAXCAPS];
int temp[256], buf_len[512];
struct cap caps[MAXCAPS];

int iterations;
int NTHREADS;
int numQs;
int numExps;

void *slre_thread(void *tid) {
  int start, end, *mytid;
  mytid = (int *)tid;
  start = (*mytid * iterations);
  end = start + iterations;

  for (int i = start; i < end; ++i) {
    for (int j = 0; j < numQs; ++j) {
      slre_match(slre[i], bufs[j], buf_len[j], caps);
    }
  }
}

int fill(FILE *f, char **toFill, int *bufLen, int len) {
  int i = 0;

  while (i < len) {
    int ch = getc(f);
    if (ch == EOF) return i;
    bufLen[i] = 0;
    char *s = (char *)malloc(5000 + 1);
    while (1) {
      s[bufLen[i]] = ch;
      ++bufLen[i];
      ch = getc(f);
      if (ch == '\n') {
        s[bufLen[i]] = 0;
        toFill[i] = s;
        ++i;
        break;
      }
    }
  }
  return i;
}

int main(int argc, char *argv[]) {
  if (argc < 4) {
    fprintf(stderr, "[ERROR] Invalid arguments provided.\n\n");
    fprintf(stderr,
            "Usage: %s [NUMBER OF THREADS] [LIST FILE] [QUESTION FILE]\n\n",
            argv[0]);
    exit(0);
  }
  /* Timing */
  /* Timing */
  STATS_INIT ("kernel", "regular_expression");
  PRINT_STAT_STRING ("abrv", "pthread_regex");

  NTHREADS = atoi(argv[1]);
  PRINT_STAT_INT ("threads", NTHREADS);

  FILE *f = fopen(argv[2], "r");
  if (f == 0) {
    fprintf(stderr, "File %s not found\n", argv[1]);
    exit(1);
  }
  numExps = fill(f, exps, temp, EXPRESSIONS);
  PRINT_STAT_INT ("expressions", numExps);

  FILE *f1 = fopen(argv[3], "r");
  if (f1 == 0) {
    fprintf(stderr, "File %s not found\n", argv[2]);
    exit(1);
  }
  numQs = fill(f1, bufs, buf_len, QUESTIONS);
  PRINT_STAT_INT ("questions", numQs);

  tic ();
  for (int i = 0; i < numExps; ++i) {
    slre[i] = (struct slre *)malloc(sizeof(slre));
    if (!slre_compile(slre[i], exps[i])) {
      printf("error compiling: %s\n", exps[i]);
    }
  }
  PRINT_STAT_DOUBLE ("regex_compile", toc ());

  tic ();
  int tids[NTHREADS];
  pthread_t threads[NTHREADS];
  pthread_attr_t attr;
  iterations = numExps / NTHREADS;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  for (int i = 0; i < NTHREADS; i++) {
    tids[i] = i;
    pthread_create(&threads[i], &attr, slre_thread, (void *)&tids[i]);
  }

  for (int i = 0; i < NTHREADS; i++) pthread_join(threads[i], NULL);

  PRINT_STAT_DOUBLE ("pthread_regex", toc ());

  STATS_END ();

  return 0;
}
