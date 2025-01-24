#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

static const char *program_name;

static void idle_wait(long int duration_ns)
{
	const struct timespec sleep_duration = {
		.tv_sec  = static_cast<long int>(duration_ns / 1'000'000'000L),
		.tv_nsec = static_cast<long int>(duration_ns % 1'000'000'000L),
	};

	nanosleep(&sleep_duration, nullptr);
}

static void busy_wait(long int duration_ns)
{
    struct timespec start, current;

    clock_gettime(CLOCK_MONOTONIC, &start);

    long int elapsed = 0;

    do {
        clock_gettime(CLOCK_MONOTONIC, &current);
        elapsed = (current.tv_sec - start.tv_sec) * 1'000'000'000L + (current.tv_nsec - start.tv_nsec);
    } while (elapsed < duration_ns);
}

int work_on_cpu(int cpu, float busy_percent, long int duration_ns)
{
	if (busy_percent > 1.f || busy_percent < 0.f)
		return -1;

	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);

	pthread_t th = pthread_self();
	if (pthread_setaffinity_np(th, sizeof(cpu_set_t), &cpuset) != 0) {
		fprintf(stderr, "Error setting thread affinity\n");
		return -2;
	}

	constexpr unsigned long CHUNK_DURATION_NS = 1'000'000;
	const unsigned long nano_busy  = CHUNK_DURATION_NS * busy_percent;
	const unsigned long nano_sleep = CHUNK_DURATION_NS - nano_busy;

	struct timespec start, current;
	long int elapsed = 0;

	clock_gettime(CLOCK_MONOTONIC, &start);

	do {

		busy_wait(nano_busy);
		idle_wait(nano_sleep);

		clock_gettime(CLOCK_MONOTONIC, &current);
		elapsed = (current.tv_sec - start.tv_sec) * 1'000'000'000L + (current.tv_nsec - start.tv_nsec);

	} while(elapsed < duration_ns);

	return 0;
}

void usage()
{
	fprintf(
		stderr,
		"  Usage: %s CPU BUSY_PERCENT DURATION_MILLISECONDS\n"
		"Example: %s 1 75 3000 # CPU1, 75%% busy for 3 seconds\n",
		program_name, program_name
	);

	exit(1);
}

int main(int argc, char **argv)
{
	int busy_percent;
	int duration_ms;
	int cpu;

	program_name = argv[0];

	if (argc != 4)
		usage();

	if (sscanf(argv[1], "%i" , &cpu) != 1)
		usage();

	if (sscanf(argv[2], "%i" , &busy_percent) != 1)
		usage();

	if (sscanf(argv[3], "%i" , &duration_ms) != 1)
		usage();

	work_on_cpu(cpu, static_cast<float>(busy_percent) / 100.f, duration_ms * 1'000'000L);

	return 0;
}


