/*
 * Copyright(C) 2015 Pedro H. Penna <pedrohenriquepenna@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <assert.h>
#include <omp.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#include <util.h>
#include <benchmark.h>

/* Definied at LaPeSD LibGOMP. */
extern void omp_set_workload(unsigned *, unsigned);

/*============================================================================*
 *                                   Kernel                                   *
 *============================================================================*/

/**
 * @brief CPU intensive kernel.
 * 
 * @param n    Number of operations.
 * @param load Load of an operation.
 * 
 * @returns A dummy result.
 */
static long kernel_cpu(unsigned n, long load)
{
	long sum = 0;
 
	for (unsigned i = 0; i < n; i++)
	{
		for (long j = 0; j < load; j++)
			sum += i + j;
	}

	return (sum);
}

#if defined(_CACHE_BENCHMARK_)

/**
 * @brief Cache intensive kernel.
 *
 * @param a    Shared array.
 * @param off  Array offset.
 * @param n    Number of operations.
 * @param load Load of an operation.
 */
static void kernel_cache(unsigned *a, unsigned off, unsigned n, long load)
{
	for (unsigned i = 0; i < n; i++)
	{
		for (unsigned j = 0; j < load; j++)
			a[off]++;
	}
}

#endif

/*============================================================================*
 *                                 Benchmark                                  *
 *============================================================================*/

/**
 * @brief Dumps benchmark statistics.
 *
 * @param respvar  Response variable.
 * @param nthreads Number of threads.
 * @param prefix   Prefix for response variable.
 */
static void benchmark_dump(const double *respvar, int nthreads, const char *prefix)
{
	double min, max, total;
	double mean, stddev;

	min = UINT_MAX; max = 0;
	total = 0; mean = 0.0; stddev = 0.0;

	/* Compute min, max, total. */
	for (int i = 0; i < nthreads; i++)
	{
		double wtotal;

		wtotal = respvar[i];

		if (min > wtotal)
			min = wtotal;
		if (max < wtotal)
			max = wtotal;

		total += wtotal;
	}

	/* Compute mean. */
	mean = ((double) total)/nthreads;

	/* Compute stddev. */
	for (int i = 0; i < nthreads; i++)
		stddev += pow(respvar[i] - mean, 2);
	stddev = sqrt(stddev/(nthreads));

	/* Print statistics. */
	printf("%s_max: %lf\n", prefix, max);
	printf("%s_total: %lf\n", prefix, total);
	printf("%s_cov: %lf\n", prefix, stddev/mean);
	printf("%s_slowdown: %lf\n", prefix, max/((double) min));
	printf("%s_cost: %lf\n", prefix, nthreads*max);
}

/**
 * @brief CPU intensive synthetic benchmark.
 * 
 * @param tasks    Tasks.
 * @param ntasks   Number of tasks.
 * @param nthreads Number of threads.
 * @param load     Kernel load.
 */
static void benchmark_cpu(const unsigned *tasks, unsigned ntasks, int nthreads, long load)
{
	long sum;
	unsigned *_tasks;
	double loads[nthreads];
	double times[nthreads];

	_tasks = smalloc(ntasks*sizeof(unsigned));

	memset(times, 0, nthreads*sizeof(double));
	memset(loads, 0, nthreads*sizeof(unsigned));
	
	/* Workload prediction. */
	memcpy(_tasks, tasks, ntasks*sizeof(unsigned));
	omp_set_workload(_tasks, ntasks);

	#pragma omp parallel num_threads(nthreads)
	{
		int tid;
		double start;
		double private_load, private_time;

		sum = 0;
		private_load = 0.0;
		private_time = 0.0;

		tid = omp_get_thread_num();
		
		#pragma omp for schedule(runtime) reduction(+:sum)
		for (unsigned i = 0; i < ntasks; i++)
		{
			int work;

			private_load += work = tasks[i];
			
			start = omp_get_wtime();
			sum += kernel_cpu(work, load);
			private_time += omp_get_wtime() - start;
		}

		loads[tid] = private_load;
		times[tid] = private_time;
	}
	
	benchmark_dump(loads, nthreads, "cpu_load");
	benchmark_dump(times, nthreads, "cpu_time");

	/* House keeping. */
	free(_tasks);
}

#if defined(_CACHE_BENCHMARK_)

#define CACHE_SIZE (256*1024)

/**
 * @brief Cache intensive synthetic benchmark.
 * 
 * @param tasks    Tasks.
 * @param ntasks   Number of tasks.
 * @param nthreads Number of threads.
 * @param load     Kernel load.
 */
static void benchmark_cache(const unsigned *tasks, unsigned ntasks, int nthreads, long load)
{
	unsigned *_tasks, *array;
	double loads[nthreads];
	double times[nthreads];

	_tasks = smalloc(ntasks*sizeof(unsigned));
	array = smalloc(ntasks*sizeof(unsigned));

	memset(times, 0, nthreads*sizeof(double));
	memset(loads, 0, nthreads*sizeof(unsigned));

	#pragma omp parallel for schedule(static)
	for (unsigned i = 0; i < ntasks; i++)
		array[i] = tasks[i];
	
	/* Workload prediction. */
	memcpy(_tasks, tasks, ntasks*sizeof(unsigned));
	omp_set_workload(_tasks, ntasks);

	#pragma omp parallel num_threads(nthreads)
	{
		int tid;
		double start;
		double private_load, private_time;

		private_load = 0.0;
		private_time = 0.0;

		tid = omp_get_thread_num();
		
		#pragma omp for schedule(runtime)
		for (unsigned i = 0; i < ntasks; i++)
		{
			int work;

			private_load += work = array[i];
			
			start = omp_get_wtime();
			kernel_cache(array, i, work, load);
			private_time += omp_get_wtime() - start;
		}

		loads[tid] = private_load;
		times[tid] = private_time;
	}
	
	benchmark_dump(loads, nthreads, "cache_load");
	benchmark_dump(times, nthreads, "cache_time");

	/* House keeping. */
	free(array);
	free(_tasks);
}

#endif

/**
 * @brief Synthetic benchmark.
 * 
 * @param tasks    Tasks.
 * @param ntasks   Number of tasks.
 * @param nthreads Number of threads.
 * @param load     Load for constant kernel.
 */
void benchmark(const unsigned *tasks, unsigned ntasks, int nthreads, long load)
{
	/* Sanity check. */
	assert(tasks != NULL);
	assert(ntasks > 0);
	assert(nthreads > 0);
	assert(load > 0);

	benchmark_cpu(tasks, ntasks, nthreads, load);

#if defined(_CACHE_BENCHMARK_)
	benchmark_cache(tasks, ntasks, nthreads, load);
#endif
}
