/**
 * Tool to deliberately break the implied MRSW contract by writing
 * an indeterminate number of times instead of reading. This is expected
 * to fail, somewhat, with some data corruption, but not a full-out segmentation
 * fault. 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdatomic.h>
#include <sys/time.h>
#include <errno.h>
#include <stdbool.h>

#include "splinter.h"

#define MAX_THREADS 1024
#define MAX_KEY_LEN 128
#define MAX_VALUE_LEN 2048

typedef struct {
    int thread_id;
    int operations_per_thread;
    double write_ratio;
    atomic_int *total_ops;
    atomic_int *successful_reads;
    atomic_int *successful_writes;
    atomic_int *failed_reads;
    atomic_int *failed_writes;
    atomic_int *contention_failures;
    struct timespec *start_time;
    int test_duration_ms;
    volatile int *running;
} thread_data_t;

typedef struct {
    int num_threads;
    int operations_per_thread;
    int test_duration_ms;
    char store_name[256];
    int slots;
    int max_value_size;
    double write_ratio;
} test_config_t;

// Predefined configurations
test_config_t configs[] = {
    // light
    {4, 10000, 10000, "torture_light", 1000, 2048, 0.3},
    // moderate 
    {16, 50000, 30000, "torture_moderate", 10000, 4096, 0.5},
    // heavy
    {64, 100000, 60000, "torture_heavy", 50000, 8192, 0.7},
    // facebook levels of ouch
    {256, 1000000, 120000, "torture_facebook", 100000, 8192, 0.6},
    // anthropic / openai levels of ouch
    // this could potentially lock up your VM if run as root
    {512, 2000000, 300000, "torture_anthropic", 500000, 16384, 0.4}
};

static inline uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static inline double time_diff_ms(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) * 1000.0 + 
           (end->tv_nsec - start->tv_nsec) / 1000000.0;
}

void generate_random_string(char *str, int length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < length - 1; i++) {
        str[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    str[length - 1] = '\0';
}

void *worker_thread(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    
    // Seed random number generator per thread
    srand(time(NULL) + data->thread_id);
    
    int operations = 0;
    char key[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
    size_t value_size;
    
    printf("🧵 Thread %d starting...\n", data->thread_id);
    
    while (*(data->running) && operations < data->operations_per_thread) {
        // Check if we've exceeded time limit
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        double elapsed = time_diff_ms(data->start_time, &now);
        if (elapsed > data->test_duration_ms) {
            break;
        }
        
        // Generate key - mix of thread-specific and shared keys for contention
        int key_id = rand() % (data->thread_id * 1000 + 2000);
        snprintf(key, sizeof(key), "thread_%d_key_%08d", data->thread_id, key_id);
        
        bool is_write = (rand() / (double)RAND_MAX) < data->write_ratio;
        
        if (is_write) {
            // Generate random value
            int val_len = 100 + rand() % 400; // 100-500 chars
            snprintf(value, sizeof(value), 
                     "{\"thread\":%d,\"op\":%d,\"timestamp\":%ld,\"data\":\"",
                     data->thread_id, operations, time(NULL));
            
            int base_len = strlen(value);
            generate_random_string(value + base_len, val_len - base_len - 10);
            strcat(value, "\"}");
            
            // Perform write
            int result = splinter_set(key, value, strlen(value));
            if (result == 0) {
                atomic_fetch_add(data->successful_writes, 1);
            } else {
                atomic_fetch_add(data->failed_writes, 1);
                atomic_fetch_add(data->contention_failures, 1);
            }
        } else {
            // Perform read
            int result = splinter_get(key, value, sizeof(value), &value_size);
            if (result == 0) {
                atomic_fetch_add(data->successful_reads, 1);
                // Verify data integrity (basic check)
                if (value_size > 0 && value[0] == '{') {
                    // Looks like valid JSON
                } else {
                    printf("⚠️  Data corruption detected in thread %d (length of returned val is %lu)\n", data->thread_id, strlen(value));
                }
            } else {
                atomic_fetch_add(data->failed_reads, 1);
            }
        }
        
        atomic_fetch_add(data->total_ops, 1);
        operations++;
        
        // Occasional yield to prevent thread starvation
        if (operations % 1000 == 0) {
            sched_yield();
        }
    }
    
    printf("🧵 Thread %d completed %d operations\n", data->thread_id, operations);
    return NULL;
}

void prepopulate_store(int slots, int max_value_size) {
    printf("📝 Pre-populating store with test data...\n");
    
    int initial_keys = slots / 4; // Fill 25% of slots
    char key[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
    
    for (int i = 0; i < initial_keys; i++) {
        snprintf(key, sizeof(key), "initial_key_%08d", i);
        snprintf(value, sizeof(value), 
                 "initial_value_%d-(%d*%s)", i, max_value_size, "x");
        splinter_set(key, value, strlen(value));
        if (i % 10000 == 0) {
            printf("  Populated %d/%d keys...\n", i, initial_keys);
        }
    }
    
    printf("✅ Pre-populated with %d keys\n", initial_keys);
}

void print_results(test_config_t *config, struct timespec *start_time, struct timespec *end_time,
                   atomic_int *total_ops, atomic_int *successful_reads, atomic_int *successful_writes,
                   atomic_int *failed_reads, atomic_int *failed_writes, atomic_int *contention_failures) {
    
    double elapsed_ms = time_diff_ms(start_time, end_time);
    int total = atomic_load(total_ops);
    int s_reads = atomic_load(successful_reads);
    int s_writes = atomic_load(successful_writes);
    int f_reads = atomic_load(failed_reads);
    int f_writes = atomic_load(failed_writes);
    int contentions = atomic_load(contention_failures);
    
    double ops_per_sec = (total / elapsed_ms) * 1000.0;
    double success_rate = ((s_reads + s_writes) / (double)total) * 100.0;
    
    printf("\n🏁 TORTURE TEST RESULTS 🏁\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("📊 PERFORMANCE:\n");
    printf("   Total Operations:    %s\n", 
           total >= 1000000 ? 
           (total >= 1000000000 ? 
            (snprintf((char[32]){0}, 32, "%.1fB", total/1e9), (char[32]){0}) :
            (snprintf((char[32]){0}, 32, "%.1fM", total/1e6), (char[32]){0})) :
           (snprintf((char[32]){0}, 32, "%d", total), (char[32]){0}));
    printf("   Operations/Second:   %s\n", 
           ops_per_sec >= 1000000 ? 
           (snprintf((char[32]){0}, 32, "%.1fM", ops_per_sec/1e6), (char[32]){0}) :
           (snprintf((char[32]){0}, 32, "%.0f", ops_per_sec), (char[32]){0}));
    printf("   Test Duration:       %.2fms\n", elapsed_ms);
    printf("   Threads Used:        %d\n", config->num_threads);
    printf("\n");
    printf("✅ SUCCESS RATES:\n");
    printf("   Successful Reads:    %d\n", s_reads);
    printf("   Successful Writes:   %d\n", s_writes);
    printf("   Success Rate:        %.2f%%\n", success_rate);
    printf("\n");
    printf("❌ FAILURE ANALYSIS:\n");
    printf("   Failed Reads:        %d\n", f_reads);
    printf("   Failed Writes:       %d\n", f_writes);
    printf("   Contention Failures: %d\n", contentions);
    printf("\n");
    
    // Performance assessment
    const char* assessment;
    if (ops_per_sec > 1000000) assessment = "🚀 LUDICROUS SPEED";
    else if (ops_per_sec > 500000) assessment = "🔥 BLAZING FAST";
    else if (ops_per_sec > 100000) assessment = "⚡ VERY FAST";
    else if (ops_per_sec > 50000) assessment = "✨ FAST";
    else if (ops_per_sec > 10000) assessment = "👍 GOOD";
    else assessment = "🤔 NEEDS TUNING";
    
    printf("🎯 VERDICT: %s\n", assessment);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

int run_torture_test(test_config_t *config) {
    printf("🔥 SPLINTER TORTURE TEST 🔥\n");
    printf("Threads: %d\n", config->num_threads);
    printf("Operations per thread: %d\n", config->operations_per_thread);
    printf("Write ratio: %.1f%%\n", config->write_ratio * 100);
    printf("Test duration: %dms\n", config->test_duration_ms);
    printf("Store: %s (%d slots, %dB max)\n", 
           config->store_name, config->slots, config->max_value_size);
    
    // Initialize store
    int result = splinter_create_or_open(config->store_name, config->slots, config->max_value_size);
    if (result != 0) {
        printf("❌ Failed to create/open store: %s\n", strerror(errno));
        return 1;
    }
    printf("✅ Store initialized\n");
    
    // Pre-populate
    prepopulate_store(config->slots, config->max_value_size);
    
    // Initialize shared counters
    atomic_int total_ops = ATOMIC_VAR_INIT(0);
    atomic_int successful_reads = ATOMIC_VAR_INIT(0);
    atomic_int successful_writes = ATOMIC_VAR_INIT(0);
    atomic_int failed_reads = ATOMIC_VAR_INIT(0);
    atomic_int failed_writes = ATOMIC_VAR_INIT(0);
    atomic_int contention_failures = ATOMIC_VAR_INIT(0);
    
    // Setup threads
    pthread_t threads[MAX_THREADS];
    thread_data_t thread_data[MAX_THREADS];
    volatile int running = 1;
    
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    printf("\n🚀 Launching %d concurrent threads...\n", config->num_threads);
    
    // Create threads
    for (int i = 0; i < config->num_threads; i++) {
        thread_data[i] = (thread_data_t){
            .thread_id = i,
            .operations_per_thread = config->operations_per_thread,
            .write_ratio = config->write_ratio,
            .total_ops = &total_ops,
            .successful_reads = &successful_reads,
            .successful_writes = &successful_writes,
            .failed_reads = &failed_reads,
            .failed_writes = &failed_writes,
            .contention_failures = &contention_failures,
            .start_time = &start_time,
            .test_duration_ms = config->test_duration_ms,
            .running = &running
        };
        
        if (pthread_create(&threads[i], NULL, worker_thread, &thread_data[i]) != 0) {
            printf("❌ Failed to create thread %d\n", i);
            running = 0;
            break;
        }
    }
    
    // Sleep for test duration, then signal stop
    usleep(config->test_duration_ms * 1000);
    running = 0;
    
    // Wait for all threads to complete
    for (int i = 0; i < config->num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    // Print results
    print_results(config, &start_time, &end_time,
                  &total_ops, &successful_reads, &successful_writes,
                  &failed_reads, &failed_writes, &contention_failures);
    
    // Close store
    splinter_close();
    
    // Return success/failure based on results
    double success_rate = ((atomic_load(&successful_reads) + atomic_load(&successful_writes)) / 
                          (double)atomic_load(&total_ops)) * 100.0;
    
    if (success_rate < 95.0) {
        printf("❌ Test failed: Success rate %.2f%% < 95%%\n", success_rate);
        return 1;
    } else {
        printf("✅ Test passed: Success rate %.2f%%\n", success_rate);
        return 0;
    }
}

int main(int argc, char *argv[]) {
    const char *test_name = argc > 1 ? argv[1] : "moderate";
    
    int config_index = -1;
    const char *config_names[] = {"light", "moderate", "heavy", "facebook", "anthropic"};
    int num_configs = sizeof(configs) / sizeof(configs[0]);
    
    for (int i = 0; i < num_configs; i++) {
        if (strcmp(test_name, config_names[i]) == 0) {
            config_index = i;
            break;
        }
    }
    
    if (config_index == -1) {
        printf("❌ Unknown test: %s\n", test_name);
        printf("Available tests: ");
        for (int i = 0; i < num_configs; i++) {
            printf("%s%s", config_names[i], i < num_configs - 1 ? ", " : "\n");
        }
        return 1;
    }
    
    return run_torture_test(&configs[config_index]);
}