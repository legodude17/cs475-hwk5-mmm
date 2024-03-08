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

	if (argc < 3)
	{
		printf("Usage: ./mmmSol S <size>\nUsage: ./mmmSol P <num threads> <size>\n");
		return 1;
	}

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

	if (mode == SEQ)
	{
		sscanf(argv[2], "%d", &size);
		num_threads = 1;
	}
	else
	{
		if (argc < 4)
		{
			printf("Usage: ./mmmSol S <size>\nUsage: ./mmmSol P <num threads> <size>\n");
			return 1;
		}

		sscanf(argv[2], "%d", &num_threads);
		sscanf(argv[3], "%d", &size);

		if (num_threads > size)
		{
			printf("mmm: Number of threads must be less than size.\n");
			return 1;
		}
	}

	// initialize my matrices
	mmm_init();

	double seqTimes[3] = {0};
	double parTimes[3] = {0};
	double clockstart, clockend;
	double error = 0;

	for (int i = 0; i < 4; i++)
	{
		clockstart = rtclock(); // start the clock

		mmm_seq();

		clockend = rtclock(); // stop the clock

		// printf("Sequential run %d took %f seconds\n", i, clockend - clockstart);
		if (i > 0)
			seqTimes[i - 1] = clockend - clockstart;

		if (mode != PAR)
			continue;

		clockstart = rtclock(); // start the clock

		thread_args *args = (thread_args *)malloc(num_threads * sizeof(thread_args));
		pthread_t *threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
		int workPerThread = (int)floor(size / num_threads);
		for (int j = 0; j < num_threads; j++)
		{
			args[j].begin = j * workPerThread;
			if (j == num_threads - 1)
				args[j].end = size;
			else
				args[j].end = (j + 1) * workPerThread;
		}

		for (int j = 0; j < num_threads; j++)
		{
			pthread_create(threads + j, NULL, mmm_par, args + j);
		}

		for (int j = 0; j < num_threads; j++)
		{
			pthread_join(threads[j], NULL);
		}

		clockend = rtclock(); // stop the clock

		// printf("Parallel run %d took %f seconds\n", i, clockend - clockstart);
		if (i > 0)
			parTimes[i - 1] = clockend - clockstart;

		double newError = mmm_verify();

		if (newError > error)
			error = newError;
	}

	printf("========\nmode: %s\nthread count: %d\nsize: %d\n========\n", mode == SEQ ? "sequential" : "parallel", num_threads, size);

	double seqTime = (seqTimes[0] + seqTimes[1] + seqTimes[2]) / 3;
	printf("Sequential Time (avg of 3 runs): %f sec\n", seqTime);

	if (mode == PAR)
	{
		double parTime = (parTimes[0] + parTimes[1] + parTimes[2]) / 3;
		printf("Parallel Time (avg of 3 runs): %f sec\n", parTime);
		printf("Speedup: %f\n", seqTime / parTime);
		printf("Verifying... largest error between parallel and sequential matrix: %f\n", error);
	}

	mmm_freeup();

	return 0;
}
