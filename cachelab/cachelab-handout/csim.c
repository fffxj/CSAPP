#include <stdio.h>
#include <assert.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include "cachelab.h"

/* Cache simulator states */
#define HIT 10
#define MISS 20
#define MISS_EVICTION 30
#define HIT_HIT 40
#define MISS_HIT 50
#define MISS_EVICTION_HIT 60

/* Command line parameters */
static int verbose;
static int s, E, b;
static char *tracefile;

/* Simulator valuables */
static int state;
static int hits, misses, evictions;
static int timestamp;

/* Cache and cache line structure */
typedef struct {
  int valid;                    /* valid bit */
  int tag;                      /* tag bits */
  int timestamp;                /* cache access timestamp */
} cache_line_t, *cache_line_ptr;

typedef struct {
  int S;                        /* number of sets */
  int E;                        /* number of lines per set */
  cache_line_ptr lines;
} cache_t, *cache_ptr;

/* Create SxE cache */
cache_ptr newCache(int S, int E) {
  /* Allocate cache header structure */
  cache_ptr result = (cache_ptr) malloc(sizeof(cache_t));
  cache_line_ptr lines = NULL;
  if (!result)
    return NULL;                /* Couldn't allocate storage */
  result->S = S;
  result->E = E;
  /* Allocate cache sets */
  if (S > 0 && E > 0) {
    int len = S*E;
    lines = (cache_line_ptr) calloc(len, sizeof(cache_line_t));
    if (!lines) {
      free((void *) result);
      return NULL;
    }
    for (int i = 0; i < len; i++) {
      lines[i].valid = 0;
    }
  }
  /* Sets will either be NULL or allocated array */
  result->lines = lines;
  return result;
}

/* Free cache memory */
void freeCache(cache_ptr cache) {
  if (cache)
    free((void *) cache->lines);
  free((void *) cache);
}

/* Cache data load/store */
void accessCache(cache_ptr cache, int address) {
  int tag = (address >> (s + b)); /* address's highest bit is always 0 */
  int set = (address >> b) & ((1 << s) - 1);
  int E = cache->E;

  cache_line_ptr searcher = cache->lines + E * set;
  int i;  
  /* Cache hit */
  for (i = 0; i < E; i++) {
    if (searcher[i].valid && searcher[i].tag == tag) {
      searcher[i].timestamp = timestamp++;
      state = HIT;
      hits++;
      return;
    }
  }
  /* Cache miss */
  for (i = 0; i < E; i++) {
    if (!searcher[i].valid) {
      searcher[i].valid = 1;
      searcher[i].tag = tag;
      searcher[i].timestamp = timestamp++;
      state = MISS;
      misses++;
      return;
    }
  }
  /* Cache miss, eviction */
  cache_line_ptr evictee = searcher;
  for (i = 0; i < E; i++) {
    /* assert: searcher[i].valid is true */
    if (searcher[i].timestamp < evictee->timestamp) {
      evictee = searcher + i;
    }
  }
  evictee->tag = tag;
  evictee->timestamp = timestamp++;
  state = MISS_EVICTION;
  misses++;
  evictions++;
  return;
}

/* Cache data modify */
void modifyCache(cache_ptr cache, int address) {
  accessCache(cache, address);
  hits++;
  switch (state) {
  case HIT:
    state = HIT_HIT;
    break;
  case MISS:
    state = MISS_HIT;
    break;
  case MISS_EVICTION:
    state = MISS_EVICTION_HIT;
    break;
  default:
    break;
  }
}

/* Simulator program help message */
void usage() {
  printf("  Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
  printf("Options:\n");
  printf("  -h         Print this help message.\n");
  printf("  -v         Optional verbose flag.\n");
  printf("  -s <num>   Number of set index bits.\n");
  printf("  -E <num>   Number of lines per set.\n");
  printf("  -b <num>   Number of block offset bits.\n");
  printf("  -t <file>  Trace file.\n");
  printf("Examples:\n");
  printf("  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n");
  printf("  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

/* Verbose mode message */
void verboseInfo(char operation, int address, int size) {
  printf("%c %x,%x ", operation, address, size);
  switch (state) {
  case HIT:
    printf("hit");
    break;
  case MISS:
    printf("miss");
    break;
  case MISS_EVICTION:
    printf("miss eviction");
    break;
  case HIT_HIT:
    printf("hit hit");
    break;
  case MISS_HIT:
    printf("miss hit");
    break;
  case MISS_EVICTION_HIT:
    printf("miss eviction hit");
    break;
  }
  printf("\n");
}

int main(int argc, char *argv[])
{
  /* Handle command line parameters */
  int opt;

  while ((opt = getopt(argc, argv, "h::v::s:E:b:t:")) != -1) {
    switch (opt) {    
    case 'v':
      verbose = 1;
      break;
    case 's':
      s = atoi(optarg);
      break;
    case 'E':
      E = atoi(optarg);
      break;
    case 'b':
      b = atoi(optarg);
      break;
    case 't':
      tracefile = optarg;
      break;
    case 'h':
      usage();
      exit(0);
    case '?':
    default:
      usage();
      exit(1);
    }
  }  

  /* Cache simulator main logic */
  int S = 1 << s;               /* S = pow(2, s) */
  cache_ptr cache = newCache(S, E);
  FILE *tracefile_fp = fopen(tracefile, "r");
  assert(tracefile_fp);

  char operation;
  int address, size;
  while (fscanf(tracefile_fp, " %c%x,%d\n", &operation, &address, &size) > 0) {
    switch (operation) {
    case 'L':
    case 'S':
      accessCache(cache, address);
      break;
    case 'M':
      modifyCache(cache, address);
      break;
    default:
      break;
    }
    if (verbose)
      verboseInfo(operation, address, size);
  }

  fclose(tracefile_fp);
  freeCache(cache);

  printSummary(hits, misses, evictions);
}
