#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include "rtclock.h"
#include "mmm.h"

// shared  globals
unsigned int mode;
unsigned int size, num_threads;
double **A, **B, **SEQ_MATRIX, **PAR_MATRIX;

int main(int argc, char *argv[])
{

	// We always need at least 3 arguments, so check that
	if (argc < 3)
	{
		printf("Usage: ./mmmSol S <size>\nUsage: ./mmmSol P <num threads> <size>\n");
		return 1;
	}

	// Check and set the mode
	char *modeStr = argv[1];
	if (strlen(modeStr) != 1)
	{
		printf("Error: mode must be either S (sequential) or P (parallel)\n");
		return 1;
	}

	if (*modeStr == 'S')
	{
		mode = SEQ;
	}
	else if (*modeStr == 'P')
	{
		mode = PAR;
	}
	else
	{

		printf("Error: mode must be either S (sequential) or P (parallel)\n");
		return 1;
	}

	// SEQ takes only the size
	if (mode == SEQ)
	{
		sscanf(argv[2], "%d", &size);
		num_threads = 1;
	}
	else
	{
		// PAR needs both size and number of threads
		if (argc < 4)
		{
			printf("Usage: ./mmmSol S <size>\nUsage: ./mmmSol P <num threads> <size>\n");
			return 1;
		}

		sscanf(argv[2], "%d", &num_threads);
		sscanf(argv[3], "%d", &size);

		if (num_threads > size)
		{
			printf("Error: Number of threads must be less than or equal to size.\n");
			return 1;
		}
	}

	// initialize matrices
	mmm_init();

	// These hold the times for each run
	double seqTimes[3] = {0};
	double parTimes[3] = {0};
	double clockstart, clockend;

	for (int i = 0; i < 4; i++)
	{
		// Reset all the matrices before each run
		mmm_reset_rand(A);
		mmm_reset_rand(B);
		mmm_reset(SEQ_MATRIX);
		mmm_reset(PAR_MATRIX);

		clockstart = rtclock(); // start the clock

		// Run the sequential version
		mmm_seq();

		clockend = rtclock(); // stop the clock

		// Don't time the first run
		if (i > 0)
			seqTimes[i - 1] = clockend - clockstart;

		// Only do the parallel version if in parallel mode
		if (mode != PAR)
			continue;

		clockstart = rtclock(); // start the clock

		// Split up the work by columns
		thread_args *args = (thread_args *)malloc(num_threads * sizeof(thread_args));
		pthread_t *threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
		int workPerThread = (int)floor(size / num_threads);
		for (int j = 0; j < num_threads; j++)
		{
			args[j].begin = j * workPerThread;
			if (j == num_threads - 1)
				// Make sure we calculate all the columns, even if num_threads doesn't divide size evenly
				args[j].end = size;
			else
				args[j].end = (j + 1) * workPerThread;
		}

		// Create the threads
		for (int j = 0; j < num_threads; j++)
		{
			pthread_create(threads + j, NULL, mmm_par, args + j);
		}

		// Create the threads
		for (int j = 0; j < num_threads; j++)
		{
			pthread_join(threads[j], NULL);
		}

		free(args);
		free(threads);

		clockend = rtclock(); // stop the clock

		// Don't time the first run
		if (i > 0)
			parTimes[i - 1] = clockend - clockstart;
	}

	// Print the info
	printf("========\nmode: %s\nthread count: %d\nsize: %d\n========\n", mode == SEQ ? "sequential" : "parallel", num_threads, size);

	// Print the sequential time
	double seqTime = (seqTimes[0] + seqTimes[1] + seqTimes[2]) / 3;
	printf("Sequential Time (avg of 3 runs): %f sec\n", seqTime);

	if (mode == PAR)
	{
		// If in parallel mode, print the sequential time, compare, and verify
		double parTime = (parTimes[0] + parTimes[1] + parTimes[2]) / 3;
		printf("Parallel Time (avg of 3 runs): %f sec\n", parTime);
		printf("Speedup: %f\n", seqTime / parTime);
		printf("Verifying...");
		printf("largest error between parallel and sequential matrix: %f\n", mmm_verify());
	}

	// Free the matrices
	mmm_freeup();

	return 0;
}
