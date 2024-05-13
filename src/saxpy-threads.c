/**
 * @defgroup   SAXPY saxpy
 *
 * @brief      This file implements an iterative saxpy operation
 *
 * @param[in] <-p> {vector size}
 * @param[in] <-s> {seed}
 * @param[in] <-n> {number of threads to create}
 * @param[in] <-i> {maximum itertions}
 *
 * @author     Danny Munera
 * @date       2020
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include <bits/getopt_core.h>
#include <pthread.h>

// Global variables
double *X;
double a;
double *Y;
double *Y_avgs;
int i, it;
int p = 10000000;
int n_threads = 2;
int max_iters = 1000;

// Structure to hold thread-local data and a lock for thread safety
typedef struct
{
	double *local_Y_avgs;
	pthread_mutex_t lock;
	double Y_avg;
	int id;
} thread_data_t;

void *saxpy_parallel(void *arg)
{
	thread_data_t *data = (thread_data_t *)arg;
	int chunk_size = p / n_threads;
	int extra = p % n_threads; // Calculate the rest of the division
	int start = data->id * chunk_size + (data->id < extra ? data->id : extra);
	int end = start + chunk_size + (data->id < extra ? 1 : 0);

	for (int it = 0; it < max_iters; it++)
	{
		for (int i = start; i < end; i++)
		{
			Y[i] = Y[i] + a * X[i];
			data->local_Y_avgs[it] += Y[i];
		}
		pthread_mutex_lock(&data->lock);
		data->Y_avg += data->local_Y_avgs[it];
		pthread_mutex_unlock(&data->lock);
	}

	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	// Variables to obtain command line parameters
	unsigned int seed = 1;

	// Variables to get execution time
	struct timeval t_start, t_end;
	double exec_time;

	// Getting input values
	int opt;
	while ((opt = getopt(argc, argv, ":p:s:n:i:")) != -1)
	{
		switch (opt)
		{
		case 'p':
			printf("vector size: %s\n", optarg);
			p = strtol(optarg, NULL, 10);
			assert(p > 0 && p <= 2147483647);
			break;
		case 's':
			printf("seed: %s\n", optarg);
			seed = strtol(optarg, NULL, 10);
			break;
		case 'n':
			printf("threads number: %s\n", optarg);
			n_threads = strtol(optarg, NULL, 10);
			break;
		case 'i':
			printf("max. iterations: %s\n", optarg);
			max_iters = strtol(optarg, NULL, 10);
			break;
		case ':':
			printf("option -%c needs a value\n", optopt);
			break;
		case '?':
			fprintf(stderr, "Usage: %s [-p <vector size>] [-s <seed>] [-n <threads number>] [-i <maximum itertions>]\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}
	srand(seed);

	printf("p = %d, seed = %d, n_threads = %d, max_iters = %d\n",
		   p, seed, n_threads, max_iters);

	// initializing data
	X = (double *)malloc(sizeof(double) * p);
	Y = (double *)malloc(sizeof(double) * p);
	Y_avgs = (double *)malloc(sizeof(double) * max_iters);

	// Allocate memory and initialize thread data
	thread_data_t *thread_data = (thread_data_t *)malloc(sizeof(thread_data_t) * n_threads);
	for (int i = 0; i < n_threads; i++)
	{
		thread_data[i].local_Y_avgs = (double *)calloc(max_iters, sizeof(double));
		pthread_mutex_init(&thread_data[i].lock, NULL);
		thread_data[i].Y_avg = 0.0;
		thread_data[i].id = i;
	}

	for (i = 0; i < p; i++)
	{
		X[i] = (double)rand() / RAND_MAX;
		Y[i] = (double)rand() / RAND_MAX;
	}

	for (i = 0; i < max_iters; i++)
	{
		Y_avgs[i] = 0.0;
	}

	a = (double)rand() / RAND_MAX;

#ifdef DEBUG
	printf("vector X= [ ");
	for (i = 0; i < p - 1; i++)
	{
		printf("%f, ", X[i]);
	}
	printf("%f ]\n", X[p - 1]);

	printf("vector Y= [ ");
	for (i = 0; i < p - 1; i++)
	{
		printf("%f, ", Y[i]);
	}
	printf("%f ]\n", Y[p - 1]);

	printf("a= %f \n", a);
#endif

	/*
	 * Function to parallelize
	 */
	gettimeofday(&t_start, NULL);

	// Create threads and pass thread data
	pthread_t threads[n_threads];
	for (long i = 0; i < n_threads; i++)
	{
		pthread_create(&threads[i], NULL, saxpy_parallel, (void *)&thread_data[i]);
	}

	// Wait for threads to finish
	for (long i = 0; i < n_threads; i++)
	{
		pthread_join(threads[i], NULL);
	}

	// Finalize Y_avgs by combining thread-local Y_avg values
	for (int it = 0; it < max_iters; it++)
	{
		for (int i = 0; i < n_threads; i++)
		{
			Y_avgs[it] += thread_data[i].local_Y_avgs[it];
		}
		Y_avgs[it] /= p;
	}
	free(thread_data); // Free thread data array

	gettimeofday(&t_end, NULL);

#ifdef DEBUG
	printf("RES: final vector Y= [ ");
	for (i = 0; i < p - 1; i++)
	{
		printf("%f, ", Y[i]);
	}
	printf("%f ]\n", Y[p - 1]);

	printf("vector Y_avgs= [ ");
	for (i = 0; i < max_iters - 1; i++)
	{
		printf("%f, ", Y_avgs[i]);
	}
	printf("%f ]\n", Y_avgs[max_iters - 1]);

#endif

	// Computing execution time
	exec_time = (t_end.tv_sec - t_start.tv_sec) * 1000.0;	 // sec to ms
	exec_time += (t_end.tv_usec - t_start.tv_usec) / 1000.0; // us to ms
	printf("Execution time: %f ms \n", exec_time);
	printf("Last 3 values of Y: %f, %f, %f \n", Y[p - 3], Y[p - 2], Y[p - 1]);
	printf("Last 3 values of Y_avgs: %f, %f, %f \n", Y_avgs[max_iters - 3], Y_avgs[max_iters - 2], Y_avgs[max_iters - 1]);
	return 0;
}