#ifndef GRAWK_H
#define GRAWK_H

#include <regex.h>

typedef struct {
  int invert_match;
  int ignore_case;
  int quiet;
} grawk_opts_t;

typedef struct {
  char *pattern_str;
  regex_t rx;
} awk_pat_t;

typedef struct grawk_s {
  grawk_opts_t opts;
  awk_pat_t *pat;
} grawk_t;

// Allocation
grawk_t *grawk_init(void);
void grawk_free(grawk_t *g);

// Pattern
awk_pat_t *grawk_build_pattern(const char *pattern);
int grawk_compile_pattern(awk_pat_t *pat, const grawk_opts_t *opts);
int grawk_set_pattern(grawk_t *g, awk_pat_t *pat);

// Config
int grawk_set_options(grawk_t *g, const grawk_opts_t *opts);

// Matching
int grawk_match(grawk_t *g, const char *line);

#endif
