#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#define MAXV 1000000
/* Windows의 rand()는 최대 32767 → 두 번 호출해 30비트 난수 생성 */
#define RAND30() (((rand() & 0x7FFF) << 15) | (rand() & 0x7FFF))
#define RAND_VAL() (RAND30() % (MAXV + 1))
#define MAXN 20000

enum { FIRST, RANDOM, MEDIAN3 };
enum { MERGE, QFIRST, QRAND, QMED3, NALGO };

static long long cmp_cnt;

/* ---- 스택 사용량 측정: 재귀 함수 안에서 지역변수 주소를 기록해 최저 주소를 추적 ---- */
static int track_stack = 0;
static char *stack_top = NULL, *stack_min = NULL;
static int cur_depth = 0, max_depth = 0;
#define ENTER() char probe_; \
    if (track_stack) { if ((char *)&probe_ < stack_min) stack_min = (char *)&probe_; \
                       if (++cur_depth > max_depth) max_depth = cur_depth; }
#define LEAVE() do { if (track_stack) cur_depth--; } while (0)

static void swap_int(int *x, int *y) { int t = *x; *x = *y; *y = t; }

/* ================= 퀵 정렬 (과제 12와 동일) ================= */
static int choose_pivot(const int *a, int l, int r, int method) {
    if (method == FIRST) return l;
    if (method == RANDOM) return l + RAND30() % (r - l + 1);
    int m = l + (r - l) / 2;
    int x = a[l], y = a[m], z = a[r];
    cmp_cnt++;
    if (x < y) {
        cmp_cnt++;
        if (y < z) return m;
        cmp_cnt++;
        return (x < z) ? r : l;
    } else {
        cmp_cnt++;
        if (x < z) return l;
        cmp_cnt++;
        return (y < z) ? r : m;
    }
}
static int partition(int *a, int l, int r, int method) {
    swap_int(&a[l], &a[choose_pivot(a, l, r, method)]);
    int pivot = a[l], i = l, j = r + 1;
    for (;;) {
        for (;;) { i++; if (i > r) break; cmp_cnt++; if (!(a[i] < pivot)) break; }
        for (;;) { j--; cmp_cnt++; if (!(a[j] > pivot)) break; }
        if (i >= j) break;
        swap_int(&a[i], &a[j]);
    }
    swap_int(&a[l], &a[j]);
    return j;
}
static void quick_rec(int *a, int l, int r, int method) {
    ENTER();
    if (l < r) {
        int p = partition(a, l, r, method);
        quick_rec(a, l, p - 1, method);
        quick_rec(a, p + 1, r, method);
    }
    LEAVE();
}

/* ================= 병합 정렬 (하향식, 임시 배열 1개) ================= */
static int *tmp_buf;
static void merge(int *a, int l, int m, int r) {
    int i = l, j = m + 1, k = l;
    while (i <= m && j <= r) {
        cmp_cnt++;                                   /* 값 비교 1회 */
        if (a[i] <= a[j]) tmp_buf[k++] = a[i++];     /* <= : 안정 정렬 */
        else              tmp_buf[k++] = a[j++];
    }
    while (i <= m) tmp_buf[k++] = a[i++];
    while (j <= r) tmp_buf[k++] = a[j++];
    for (k = l; k <= r; k++) a[k] = tmp_buf[k];
}
static void merge_rec(int *a, int l, int r) {
    ENTER();
    if (l < r) {
        int m = l + (r - l) / 2;
        merge_rec(a, l, m);
        merge_rec(a, m + 1, r);
        merge(a, l, m, r);
    }
    LEAVE();
}

/* 알고리즘 실행. 임시 배열은 정렬 시작 시 할당하고 끝나면 해제(할당 시간도 정렬 시간에 포함) */
static void run_algo(int k, int *a, int n) {
    if (k == MERGE) {
        tmp_buf = (int *)malloc(n * sizeof(int));
        merge_rec(a, 0, n - 1);
        free(tmp_buf);
    } else {
        quick_rec(a, 0, n - 1, k == QFIRST ? FIRST : (k == QRAND ? RANDOM : MEDIAN3));
    }
}

/* ---- 메모리: 추가 힙 + 측정된 최대 스택 ---- */
static long long heap_bytes(int k, int n) { return k == MERGE ? (long long)n * sizeof(int) : 0; }
static long long measure_stack(int k, const int *orig, int n, int *depth_out) {
    static int w[MAXN];
    char base_;
    memcpy(w, orig, n * sizeof(int));
    track_stack = 1; cur_depth = max_depth = 0;
    stack_top = &base_; stack_min = &base_;
    run_algo(k, w, n);
    track_stack = 0;
    *depth_out = max_depth;
    return (long long)(stack_top - stack_min);
}

/* ---- 검증/타이머 ---- */
static int is_sorted(const int *a, int n) { for (int i = 0; i + 1 < n; i++) if (a[i] > a[i + 1]) return 0; return 1; }
static int cmp_int(const void *x, const void *y) { int p = *(const int *)x, q = *(const int *)y; return (p > q) - (p < q); }
static int same_multiset(const int *sorted, const int *orig, int n) {
    static int t[MAXN];
    memcpy(t, orig, n * sizeof(int)); qsort(t, n, sizeof(int), cmp_int);
    return memcmp(t, sorted, n * sizeof(int)) == 0;
}
static double now_ms(void) {
#ifdef _WIN32
    LARGE_INTEGER f, c; QueryPerformanceFrequency(&f); QueryPerformanceCounter(&c);
    return (double)c.QuadPart * 1000.0 / (double)f.QuadPart;
#else
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
#endif
}

static const char *NAME[NALGO] = {"Merge Sort      ", "Quick (First)   ", "Quick (Random)  ", "Quick (Median3) "};

int main(void) {
    int sizes[] = {1000, 2000, 4000, 8000, 16000, 20000};
    int ns = 6, TRIALS = 20, REPS = 200;
    static int orig[MAXN], work[MAXN];
    long long comps[6][NALGO], mem[6][NALGO], stk[6][NALGO];
    int depth[6][NALGO];
    double tms[6][NALGO], avgc[6][NALGO] = {{0}};
    int all_ok = 1;
    srand(20240601);

    for (int s = 0; s < ns; s++) {
        int n = sizes[s];
        /* (a) 20회 반복: 비교 횟수 평균 (보조 분석용) + 정렬 검증 */
        for (int t = 0; t < TRIALS; t++) {
            for (int i = 0; i < n; i++) orig[i] = RAND_VAL();
            for (int k = 0; k < NALGO; k++) {
                memcpy(work, orig, n * sizeof(int));
                cmp_cnt = 0; run_algo(k, work, n);
                if (!is_sorted(work, n) || !same_multiset(work, orig, n)) all_ok = 0;
                avgc[s][k] += (double)cmp_cnt / TRIALS;
            }
        }
        /* (b) 본 실험: 새 원본 1개 */
        for (int i = 0; i < n; i++) orig[i] = RAND_VAL();
        for (int k = 0; k < NALGO; k++) {
            memcpy(work, orig, n * sizeof(int));
            cmp_cnt = 0; run_algo(k, work, n);
            if (!is_sorted(work, n) || !same_multiset(work, orig, n)) all_ok = 0;
            comps[s][k] = cmp_cnt;
            stk[s][k] = measure_stack(k, orig, n, &depth[s][k]);
            mem[s][k] = heap_bytes(k, n) + stk[s][k];
            /* 실행시간: 같은 원본으로 REPS회, 복사는 시간 제외 */
            double total = 0;
            for (int r = 0; r < REPS; r++) {
                memcpy(work, orig, n * sizeof(int));
                double t0 = now_ms();
                run_algo(k, work, n);
                total += now_ms() - t0;
            }
            tms[s][k] = total / REPS;
        }
    }

    for (int s = 0; s < ns; s++) {
        printf("Data Size : %d\n\n", sizes[s]);
        printf("Algorithm       Comparisons    Time(ms)    Memory(Bytes)\n");
        printf("--------------------------------------------------------\n");
        for (int k = 0; k < NALGO; k++)
            printf("%s %-14lld %-11.4f %lld\n", NAME[k], comps[s][k], tms[s][k], mem[s][k]);
        printf("\n");
    }
    printf("All results verified (ascending + same elements): %s\n\n", all_ok ? "YES" : "NO");

    printf("=== Memory breakdown (bytes): heap(temp array) + measured stack, max recursion depth ===\n");
    for (int s = 0; s < ns; s++) {
        printf("n=%-6d", sizes[s]);
        for (int k = 0; k < NALGO; k++)
            printf(" | %s heap=%lld stack=%lld depth=%d", k == MERGE ? "Merge" : (k == QFIRST ? "QFirst" : (k == QRAND ? "QRand" : "QMed3")),
                   heap_bytes(k, sizes[s]), stk[s][k], depth[s][k]);
        printf("\n");
    }

    printf("\n=== Average comparisons over %d data sets ===\n", TRIALS);
    printf("%-6s %12s %12s %12s %12s %10s\n", "n", "Merge", "QFirst", "QRandom", "QMedian3", "n*log2(n)");
    for (int s = 0; s < ns; s++)
        printf("%-6d %12.0f %12.0f %12.0f %12.0f %10.0f\n", sizes[s], avgc[s][0], avgc[s][1], avgc[s][2], avgc[s][3],
               sizes[s] * log((double)sizes[s]) / log(2.0));
    printf("\nGrowth ratio when n doubles (time, ms):\n");
    for (int s = 1; s < 5; s++)
        printf("%5d -> %5d : Merge x%.2f QFirst x%.2f QRand x%.2f QMed3 x%.2f\n", sizes[s-1], sizes[s],
               tms[s][0]/tms[s-1][0], tms[s][1]/tms[s-1][1], tms[s][2]/tms[s-1][2], tms[s][3]/tms[s-1][3]);
    printf("Time ratio Merge/QMed3: ");
    for (int s = 0; s < ns; s++) printf("%.2f ", tms[s][0] / tms[s][3]);
    printf("\n");

    /* 보충: 정렬된 입력 n=5000에서의 스택 메모리 (재귀 깊이) */
    {
        int n = 5000;
        for (int i = 0; i < n; i++) orig[i] = i * 2;
        printf("\n=== Supplementary: sorted input, n=%d ===\n", n);
        for (int k = 0; k < NALGO; k++) {
            int d; long long st = measure_stack(k, orig, n, &d);
            memcpy(work, orig, n * sizeof(int));
            cmp_cnt = 0; run_algo(k, work, n);
            printf("%s comps=%-10lld heap=%-6lld stack=%-8lld depth=%d sorted=%s\n", NAME[k], cmp_cnt, heap_bytes(k, n), st, d,
                   is_sorted(work, n) ? "OK" : "FAIL");
        }
    }
    return 0;
}