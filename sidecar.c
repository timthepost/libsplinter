#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <time.h>

#define HISTORY_HEIGHT 15
#define REFRESH 500000
#define HISTORY_DIVISOR 4
#define MAXW 512
#define MAX_DEBUG_LINES 512

typedef struct {
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
} cpu_stats;

static int term_cols = 80;
static int term_rows = 24;
static int graph_width = 50;

static double hist_cpu[MAXW] = {0};
static double hist_mem[MAXW] = {0};

static double last_io_pct = 0.0;
static volatile sig_atomic_t resize_pending = 0;

// debug log buffer
static char *debug_lines[MAX_DEBUG_LINES];
static int debug_line_count = 0;

// file handle for tail mode
static FILE *dbg_fp = NULL;

void handle_winch(int sig) {
    (void)sig;
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        term_cols  = ws.ws_col;
        term_rows  = ws.ws_row;
        graph_width = term_cols - 12;
        if (graph_width > MAXW) graph_width = MAXW;
        if (graph_width < 20)   graph_width = 20;
    }
    resize_pending = 1;
}

void install_winch_handler() {
    struct sigaction sa;
    sa.sa_handler = handle_winch;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGWINCH, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
}

void debug_log_append(const char *line) {
    if (debug_line_count >= MAX_DEBUG_LINES) {
        free(debug_lines[0]);
        memmove(&debug_lines[0], &debug_lines[1],
                (MAX_DEBUG_LINES-1)*sizeof(char*));
        debug_line_count--;
    }
    debug_lines[debug_line_count++] = strdup(line);
}

// --- File tail integration ---
int init_debug_file(const char *path) {
    dbg_fp = fopen(path, "r");
    if (!dbg_fp) {
        perror("fopen debug file");
        return -1;
    }
    setvbuf(dbg_fp, NULL, _IONBF, 0);
    fseek(dbg_fp, 0, SEEK_END); // jump to end
    return 0;
}

void read_debug_file() {
    if (!dbg_fp) return;
    char line[1024];
    while (fgets(line, sizeof(line), dbg_fp)) {
        line[strcspn(line, "\r\n")] = 0; // strip newline
        debug_log_append(line);
    }
}

// --- System stats code ---
void get_cpu_stats(cpu_stats *s) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) exit(1);
    fscanf(f, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
           &s->user, &s->nice, &s->system, &s->idle,
           &s->iowait, &s->irq, &s->softirq, &s->steal);
    fclose(f);
}

double get_cpu_usage(cpu_stats *prev) {
    cpu_stats cur;
    get_cpu_stats(&cur);
    unsigned long long prev_idle = prev->idle + prev->iowait;
    unsigned long long idle = cur.idle + cur.iowait;
    unsigned long long prev_non = (prev->user + prev->nice + prev->system +
                                   prev->irq + prev->softirq + prev->steal);
    unsigned long long non = (cur.user + cur.nice + cur.system +
                              cur.irq + cur.softirq + cur.steal);
    unsigned long long prev_total = prev_idle + prev_non;
    unsigned long long total = idle + non;
    unsigned long long diff_total = total - prev_total;
    unsigned long long diff_idle = idle - prev_idle;
    unsigned long long diff_iowait = cur.iowait - prev->iowait;

    *prev = cur;

    if (diff_total > 0) {
        last_io_pct = (double)diff_iowait / diff_total * 100.0;
        return (double)(diff_total - diff_idle) / diff_total * 100.0;
    } else {
        last_io_pct = 0.0;
        return 0.0;
    }
}

double get_mem_usage(double *swap_pct) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) exit(1);
    char key[32]; unsigned long val; char unit[8];
    unsigned long mem_total=1, mem_free=0, buffers=0, cached=0;
    unsigned long swap_total=1, swap_free=0;
    while (fscanf(f, "%31s %lu %7s\n", key, &val, unit) == 3) {
        if (!strcmp(key, "MemTotal:")) mem_total = val;
        else if (!strcmp(key, "MemFree:")) mem_free = val;
        else if (!strcmp(key, "Buffers:")) buffers = val;
        else if (!strcmp(key, "Cached:")) cached = val;
        else if (!strcmp(key, "SwapTotal:")) swap_total = val;
        else if (!strcmp(key, "SwapFree:")) swap_free = val;
    }
    fclose(f);
    unsigned long used = mem_total - mem_free - buffers - cached;
    *swap_pct = (swap_total > 0) ?
                (double)(swap_total - swap_free) / swap_total * 100.0 : 0.0;
    return (double)used / mem_total * 100.0;
}

void draw_bar(const char *label, double percent) {
    int filled = (int)(percent / 100.0 * graph_width);
    printf("%-4s [", label);
    for (int i = 0; i < graph_width; i++) {
        if (i < filled) printf("■");
        else printf(" ");
    }
    printf("]\n    -> %5.1f%%\n", percent);
}

int main(int argc, char **argv) {
    if (argc > 1) {
        if (init_debug_file(argv[1]) != 0) {
            dbg_fp = NULL; // disable debug if file can't be opened
        }
    }

    install_winch_handler();
    handle_winch(0);

    cpu_stats prev; get_cpu_stats(&prev);
    int hist_counter = 0;

    printf("\033[2J");
    while (1) {
        double swap_pct;
        double cpu = get_cpu_usage(&prev);
        double mem = get_mem_usage(&swap_pct);

        if (dbg_fp) {
            read_debug_file();
        }

        if (hist_counter == 0) {
            memmove(hist_cpu, hist_cpu+1, (MAXW-1)*sizeof(double));
            memmove(hist_mem, hist_mem+1, (MAXW-1)*sizeof(double));
            hist_cpu[MAXW-1] = cpu;
            hist_mem[MAXW-1] = mem;
        }
        hist_counter = (hist_counter + 1) % HISTORY_DIVISOR;

        if (resize_pending) {
            printf("\033[2J");
            resize_pending = 0;
        }
        printf("\033[H");

        printf("History (CPU=█, RAM=░)\n");
        for (int row=HISTORY_HEIGHT; row>=0; row--) {
            for (int col=MAXW-graph_width; col<MAXW; col++) {
                int c = (int)(hist_cpu[col] / 100.0 * HISTORY_HEIGHT);
                int m = (int)(hist_mem[col] / 100.0 * HISTORY_HEIGHT);
                if (c >= row && m >= row) printf("▓");
                else if (c >= row) printf("█");
                else if (m >= row) printf("░");
                else printf(" ");
            }
            printf("\n");
        }

        printf("\n");
        draw_bar("CPU", cpu);
        draw_bar("RAM", mem);

        if (dbg_fp) {
            int used_above_debug = 1 + HISTORY_HEIGHT + 1 + (2*2) + 1;
            int used_below_debug = 2;
            int available = term_rows - used_above_debug - used_below_debug;
            int max_debug_rows = (available > 0 ? available : 0);

            printf("\n--- Debug ---\n");
            int start = debug_line_count > max_debug_rows ?
                        debug_line_count - max_debug_rows : 0;
            for (int i = start; i < debug_line_count; i++) {
                printf("%.*s\n", term_cols > 1 ? term_cols - 1 : 1, debug_lines[i]);
            }
        }

        printf("\n(w=%d, s=%0.1f%%, i=%0.1f%%)\n",
               graph_width, swap_pct, last_io_pct);

        fflush(stdout);
        struct timespec ts = {0, REFRESH * 1000};
        nanosleep(&ts, NULL);
    }
}
