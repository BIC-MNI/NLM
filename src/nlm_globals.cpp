#include "nlm_globals.h"

/* Defaults match what mincnlm's argument table has always started from, so
 * behaviour is unchanged for callers that never touch them. */
int verbose   = 0;
int debug     = 0;
int nb_thread = 4;
int testmean  = 1;
int testvar   = 1;
int block     = 1;
