#ifndef MMM_H_
#define MMM_H_

#define SEQ 1
#define PAR 2
// shared globals
extern unsigned int mode;
extern unsigned int size, num_threads;
extern double **A, **B, **SEQ_MATRIX, **PAR_MATRIX;

typedef struct thread_args {
  int begin; // first column to calculate
  long end;   // last column to calculate
} thread_args;

void mmm_init();
void mmm_reset(double **);
void mmm_freeup();
void mmm_seq();
void* mmm_par(void *);
double mmm_verify();

#endif /* MMM_H_ */
