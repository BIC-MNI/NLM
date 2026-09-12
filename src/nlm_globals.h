/* Legacy process-wide state read by nl_means.cpp, nl_means_block.cpp and
 * nl_means_utils.cpp.
 *
 * These predate any notion of this code being a library: the algorithm
 * routines consult them directly rather than taking them as parameters, so
 * whoever links those objects has to supply the definitions.  They now live in
 * the library itself (nlm_globals.cpp), which is what lets a caller link `nlm`
 * without hand-rolling its own copies.
 *
 * New code should use nlm::denoise_params (nlm_denoise.h) instead; nlm::denoise
 * sets these from the parameter struct before dispatching.  They remain
 * separately visible because mincnlm's ParseArgv table writes into their
 * addresses.
 */

#ifndef NLM_GLOBALS_H
#define NLM_GLOBALS_H

extern int verbose;    /* chatter on stdout                                  */
extern int debug;      /* extra chatter, intermediate volumes in the drivers */
extern int nb_thread;  /* pthread count used by denoise_mt/denoise_block_mt  */
extern int testmean;   /* enable the mean-ratio patch preselection test      */
extern int testvar;    /* enable the variance-ratio patch preselection test  */
extern int block;      /* 1 = block-wise NL-means, 0 = voxel-wise            */

#endif /* NLM_GLOBALS_H */
