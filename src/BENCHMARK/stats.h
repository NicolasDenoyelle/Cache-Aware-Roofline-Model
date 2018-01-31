#ifndef STATS_H
#define STATS_H

#ifndef ROOFLINE_N_SAMPLES
#define ROOFLINE_N_SAMPLES 8 /* Number of sample per benchmark */
#endif

#ifndef ROOFLINE_MIN_DUR
#define ROOFLINE_MIN_DUR 10 /* milliseconds */
#endif

unsigned roofline_PGCD(unsigned, unsigned);
unsigned roofline_PPCM(unsigned, unsigned);

long roofline_autoset_repeat(roofline_stream src, const int op_type, const void * benchmark);

#endif /* STATS_H */
