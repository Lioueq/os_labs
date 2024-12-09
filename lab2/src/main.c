#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define TOTAL_PTS 1000000

typedef struct {
    double radius;
    long pts_thread;
} thread_attrs;

pthread_mutex_t mutex;
long total_in_circle = 0;

void* monte_carlo(void* arg) {
    thread_attrs* attrs = (thread_attrs*)arg;
    long in_circle = 0;
    unsigned int seed = rand();
    for (long i = 0; i < attrs->pts_thread; i++) {
        double x = (double)rand_r(&seed) / RAND_MAX * attrs->radius;
        double y = (double)rand_r(&seed) / RAND_MAX * attrs->radius;
        if (x * x + y * y <= attrs->radius * attrs->radius) {
            in_circle++;
        }
    }

    pthread_mutex_lock(&mutex);
    total_in_circle += in_circle;
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    if (argc != 3) {
        printf("Using: %s <radius> <thread_count>\n", argv[0]);
        return 1;
    }
    double radius = atof(argv[1]);
    if (radius < 0) {
        printf("Negative radius\n");
        return -1;
    }

    int threads_count = atoi(argv[2]);

    srand(time(NULL));
    pthread_t threads[threads_count];
    long long pts_thread = TOTAL_PTS / threads_count;
    thread_attrs attrs;
    attrs.radius = radius;
    attrs.pts_thread = pts_thread;

    pthread_mutex_init(&mutex, NULL);

    for (int i = 0; i < threads_count; ++i) {
        pthread_create(&threads[i], NULL, monte_carlo, &attrs);
    }

    for (int i = 0; i < threads_count; ++i) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);

    double area = 4 * radius * radius * ((double)total_in_circle / (double)TOTAL_PTS);
    printf("Circle area with radius %.2f is %.5f\n", radius, area);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Execution time: %.10f seconds\n", elapsed);

    return 0;
}