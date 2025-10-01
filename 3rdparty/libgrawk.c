// libgrawk.c
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "grawk.h"

struct grawk {
  awk_pat_t *pat;
  grawk_opts_t opts;
};

awk_pat_t *grawk_build_pattern(const char *pattern) {
  if (!pattern) return NULL;
  awk_pat_t *pat = malloc(sizeof(awk_pat_t));
  if (!pat) return NULL;
  memset(pat, 0, sizeof(awk_pat_t));
  pat->pattern_str = strdup(pattern);
  if (!pat->pattern_str) {
    free(pat);
    return NULL;
  }
  return pat;
}

int grawk_compile_pattern(awk_pat_t *pat, const grawk_opts_t *opts) {
  if (!pat || !pat->pattern_str) return -1;
  int flags = REG_NOSUB | REG_NEWLINE | REG_EXTENDED;
  if (opts && opts->ignore_case) flags |= REG_ICASE;
  return regcomp(&pat->rx, pat->pattern_str, flags);
}

grawk_t *grawk_init() {
  grawk_t *g = malloc(sizeof(grawk_t));
  if (!g) return NULL;
  memset(g, 0, sizeof(grawk_t));
  return g;
}

int grawk_set_options(grawk_t *g, const grawk_opts_t *opts) {
  if (!g || !opts) return -1;
  g->opts = *opts;
  return 0;
}

int grawk_set_pattern(grawk_t *g, awk_pat_t *pat) {
  if (!g || !pat) return -1;
  if (grawk_compile_pattern(pat, &g->opts) != 0) return -1;
  g->pat = pat;
  return 0;
}

int grawk_match(grawk_t *g, const char *line) {
  if (!g || !line || !g->pat) return -1;
  int match = regexec(&g->pat->rx, line, 0, NULL, 0) == 0;
  if (g->opts.invert_match) match = !match;
  return match ? 1 : 0;
}

void grawk_free(grawk_t *g) {
  if (!g) return;
  if (g->pat) {
    regfree(&g->pat->rx);
    free(g->pat->pattern_str);
    free(g->pat);
  }
  free(g);
}
