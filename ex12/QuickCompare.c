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
static long long cmp_cnt;

static void swap_int(int *x, int *y) { int t = *x; *x = *y; *y = t; }

/* ---------- 피벗 선택 (이 부분만 방법별로 다름) ---------- */
static int choose_pivot(const int *a, int l, int r, int method) {
    if (method == FIRST) return l;
    if (method == RANDOM) return l + RAND30() % (r - l + 1);   /* 난수 생성은 비교에 미포함 */
    /* Median-of-Three: 값 비교를 모두 카운트 */
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

/* ---------- 분할 (세 방법 공통) ---------- */
/* 피벗을 a[l]로 옮긴 뒤 좌우에서 스캔. 피벗과의 비교는 모두 카운트 */
static int partition(int *a, int l, int r, int method) {
    swap_int(&a[l], &a[choose_pivot(a, l, r, method)]);
    int pivot = a[l], i = l, j = r + 1;
    for (;;) {
        for (;;) {                       /* i: pivot 이상인 원소를 찾을 때까지 */
            i++;
            if (i > r) break;            /* 범위 검사는 카운트 제외 */
            cmp_cnt++;
            if (!(a[i] < pivot)) break;
        }
        for (;;) {                       /* j: pivot 이하인 원소를 찾을 때까지 */
            j--;
            cmp_cnt++;
            if (!(a[j] > pivot)) break;  /* a[l] == pivot 이므로 반드시 멈춤 */
        }
        if (i >= j) break;
        swap_int(&a[i], &a[j]);
    }
    swap_int(&a[l], &a[j]);
    return j;
}

/* ---------- 재귀적 퀵 정렬 ---------- */
static void quick_rec(int *a, int l, int r, int method) {
    if (l >= r) return;
    int p = partition(a, l, r, method);
    quick_rec(a, l, p - 1, method);
    quick_rec(a, p + 1, r, method);
}

/* ---------- 반복적 퀵 정렬 (명시적 스택) ---------- */
static int stk[2 * (MAXN + 10)];
static void quick_iter(int *a, int n, int method) {
    int top = 0;
    stk[top++] = 0; stk[top++] = n - 1;
    while (top > 0) {
        int r = stk[--top], l = stk[--top];
        if (l >= r) continue;
        int p = partition(a, l, r, method);
        stk[top++] = p + 1; stk[top++] = r;      /* 오른쪽을 먼저 push → 왼쪽이 먼저 처리 (재귀와 같은 순서) */
        stk[top++] = l;     stk[top++] = p - 1;
    }
}

/* ---------- 검증 ---------- */
static int is_sorted(const int *a, int n) {
    for (int i = 0; i + 1 < n; i++) if (a[i] > a[i + 1]) return 0;
    return 1;
}
static int cmp_int(const void *x, const void *y) { int p = *(const int *)x, q = *(const int *)y; return (p > q) - (p < q); }
static int same_multiset(const int *sorted, const int *orig, int n) {
    static int t[MAXN];
    memcpy(t, orig, n * sizeof(int));
    qsort(t, n, sizeof(int), cmp_int);      /* 검증용 (측정 제외) */
    return memcmp(t, sorted, n * sizeof(int)) == 0;
}

/* ---------- 타이머 ---------- */
static double now_ms(void) {
#ifdef _WIN32
    LARGE_INTEGER f, c; QueryPerformanceFrequency(&f); QueryPerformanceCounter(&c);
    return (double)c.QuadPart * 1000.0 / (double)f.QuadPart;
#else
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
#endif
}

static const char *LABEL[3] = {"Quick Sort (First)   ", "Quick Sort (Random)  ", "Quick Sort (Median-3)"};

int main(void) {
    int sizes[] = {1000, 2000, 4000, 8000, 16000, 20000};
    int ns = 6, TRIALS = 20;
    static int orig[MAXN], work[MAXN];
    long long single[6][3];
    double avg[6][3] = {{0}};
    int all_ok = 1;
    srand(20240601);

    for (int s = 0; s < ns; s++) {
        int n = sizes[s];
        for (int t = 0; t < TRIALS; t++) {
            for (int i = 0; i < n; i++) orig[i] = RAND_VAL();
            for (int m = 0; m < 3; m++) {
                memcpy(work, orig, n * sizeof(int));
                cmp_cnt = 0;
                quick_rec(work, 0, n - 1, m);
                if (!is_sorted(work, n) || !same_multiset(work, orig, n)) all_ok = 0;
                if (t == 0) single[s][m] = cmp_cnt;
                avg[s][m] += (double)cmp_cnt / TRIALS;
            }
        }
    }

    printf("=== Main experiment (one random data set per size) ===\n\n");
    for (int s = 0; s < ns; s++) {
        printf("Data Size : %d\n\n", sizes[s]);
        for (int m = 0; m < 3; m++) printf("%s : %lld comparisons\n", LABEL[m], single[s][m]);
        printf("\n");
    }
    printf("All results verified (ascending + same elements): %s\n\n", all_ok ? "YES" : "NO");

    printf("=== Supplementary: average of %d trials ===\n\n", TRIALS);
    printf("%-6s %12s %12s %12s\n", "n", "First", "Random", "Median-3");
    for (int s = 0; s < ns; s++)
        printf("%-6d %12.0f %12.0f %12.0f\n", sizes[s], avg[s][0], avg[s][1], avg[s][2]);

    printf("\nRatio to n*log2(n) (average):\n");
    for (int s = 0; s < ns; s++) {
        double nl = sizes[s] * (log(sizes[s]) / log(2.0));
        printf("n=%-6d First %.3f  Random %.3f  Median-3 %.3f\n", sizes[s], avg[s][0]/nl, avg[s][1]/nl, avg[s][2]/nl);
    }
    printf("\nGrowth ratio when n doubles (average):\n");
    for (int s = 1; s < 5; s++)
        printf("%5d -> %5d : First x%.2f  Random x%.2f  Median-3 x%.2f\n", sizes[s-1], sizes[s],
               avg[s][0]/avg[s-1][0], avg[s][1]/avg[s-1][1], avg[s][2]/avg[s-1][2]);
    printf("Median-3 vs First: ");
    for (int s = 0; s < ns; s++) printf("%.1f%% ", 100.0*(1.0 - avg[s][2]/avg[s][0]));
    printf("(reduction)\n");

    /* ---- 보충: 이미 정렬된 입력 (n=5000, 재귀 깊이 제한 때문) ---- */
    {
        int n = 5000;
        for (int i = 0; i < n; i++) orig[i] = i * 2;     /* 정렬된 입력 */
        printf("\n=== Supplementary: already sorted input, n=%d ===\n", n);
        for (int m = 0; m < 3; m++) {
            memcpy(work, orig, n * sizeof(int));
            cmp_cnt = 0; quick_rec(work, 0, n - 1, m);
            printf("%s : %lld comparisons (sorted: %s)\n", LABEL[m], cmp_cnt, is_sorted(work, n) ? "OK" : "FAIL");
        }
    }

    /* ---- 보너스: 재귀 vs 반복 ---- */
    {
        int n = 10000, REPS = 300;
        double t_rec = 0, t_it = 0;
        long long c_rec = 0, c_it = 0;
        int same = 1;
        for (int i = 0; i < n; i++) orig[i] = RAND_VAL();
        /* 워밍업 */
        memcpy(work, orig, n * sizeof(int)); quick_rec(work, 0, n - 1, MEDIAN3);
        memcpy(work, orig, n * sizeof(int)); quick_iter(work, n, MEDIAN3);
        for (int r = 0; r < REPS; r++) {
            static int w1[MAXN], w2[MAXN];
            double t0, t1;
            memcpy(w1, orig, n * sizeof(int));
            memcpy(w2, orig, n * sizeof(int));
            for (int k = 0; k < 2; k++) {                       /* 실행 순서 편향 제거를 위해 번갈아 측정 */
                if ((r + k) % 2 == 0) {
                    cmp_cnt = 0; t0 = now_ms(); quick_rec(w1, 0, n - 1, MEDIAN3); t1 = now_ms();
                    t_rec += t1 - t0; c_rec = cmp_cnt;
                } else {
                    cmp_cnt = 0; t0 = now_ms(); quick_iter(w2, n, MEDIAN3); t1 = now_ms();
                    t_it += t1 - t0; c_it = cmp_cnt;
                }
            }
            if (!is_sorted(w1, n) || !is_sorted(w2, n) || memcmp(w1, w2, n * sizeof(int)) != 0) same = 0;
        }
        printf("\n=== Bonus: recursive vs iterative (average of %d runs) ===\n\n", REPS);
        printf("Data Size : %d\nPivot     : Median-of-Three\n\n", n);
        printf("Recursive Quick Sort : %.4f ms\n", t_rec / REPS);
        printf("Iterative Quick Sort : %.4f ms\n", t_it / REPS);
        printf("\nComparisons recursive=%lld, iterative=%lld, identical results: %s\n", c_rec, c_it, same ? "YES" : "NO");
    }
    return 0;
}