#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXV 1000000
/* Windows의 rand()는 최대 32767이므로 두 번 호출해 30비트 난수를 만든다 */
#define RAND_VAL() ((((rand() & 0x7FFF) << 15) | (rand() & 0x7FFF)) % (MAXV + 1))

static long long cmp_cnt;

/* [1] 삽입 정렬 */
void insertion_sort(int *a, int n) {
    for (int i = 1; i < n; i++) {
        int cur = a[i], j = i - 1;
        while (j >= 0) {
            cmp_cnt++;                       /* 데이터 값 비교 1회 */
            if (a[j] > cur) { a[j + 1] = a[j]; j--; }
            else break;
        }
        a[j + 1] = cur;
    }
}

/* 간격 gap으로 하는 삽입 정렬 (쉘 정렬의 한 단계) */
static void gap_insertion(int *a, int n, int gap) {
    for (int i = gap; i < n; i++) {
        int cur = a[i], j = i - gap;
        while (j >= 0) {
            cmp_cnt++;
            if (a[j] > cur) { a[j + gap] = a[j]; j -= gap; }
            else break;
        }
        a[j + gap] = cur;
    }
}

/* [2] 쉘 정렬 - 기본 간격: n/2, n/4, ..., 1 */
void shell_original(int *a, int n) {
    for (int gap = n / 2; gap >= 1; gap /= 2) gap_insertion(a, n, gap);
}

/* [3] 쉘 정렬 - Knuth: 1, 4, 13, 40, ... (n보다 작은 가장 큰 값부터 역순) */
void shell_knuth(int *a, int n) {
    int h = 1;
    while (h * 3 + 1 < n) h = h * 3 + 1;
    for (; h >= 1; h /= 3) gap_insertion(a, n, h);   /* (h-1)/3 == h/3 (정수 나눗셈) */
}

int is_sorted(const int *a, int n) {
    for (int i = 0; i + 1 < n; i++) if (a[i] > a[i + 1]) return 0;
    return 1;
}

/* 원본과 동일한 원소 집합인지 확인 (정렬 후 원본을 qsort한 것과 비교; 검증용이므로 측정 제외) */
static int cmp_int(const void *x, const void *y) { int p = *(int*)x, q = *(int*)y; return (p > q) - (p < q); }
int same_multiset(const int *sorted, const int *orig, int n) {
    int *t = malloc(n * sizeof(int));
    memcpy(t, orig, n * sizeof(int));
    qsort(t, n, sizeof(int), cmp_int);
    int ok = memcmp(t, sorted, n * sizeof(int)) == 0;
    free(t); return ok;
}

typedef void (*SortFn)(int *, int);
static const char *LABEL[3] = {"Insertion Sort       ", "Shell Sort (Original)", "Shell Sort (Knuth)   "};
static SortFn FN[3] = {insertion_sort, shell_original, shell_knuth};

int main(void) {
    int sizes[] = {1000, 2000, 4000, 8000, 16000, 20000};
    int ns = sizeof sizes / sizeof sizes[0];
    int TRIALS = 20;
    srand(20240601);

    static int orig[20000], work[20000];
    long long single[6][3];
    double avg[6][3] = {{0}};
    int all_ok = 1;

    for (int s = 0; s < ns; s++) {
        int n = sizes[s];
        for (int t = 0; t < TRIALS; t++) {
            for (int i = 0; i < n; i++) orig[i] = RAND_VAL();
            for (int k = 0; k < 3; k++) {
                memcpy(work, orig, n * sizeof(int));
                cmp_cnt = 0;
                FN[k](work, n);
                if (!is_sorted(work, n) || !same_multiset(work, orig, n)) all_ok = 0;
                if (t == 0) single[s][k] = cmp_cnt;     /* 1회차 = 본 실험 */
                avg[s][k] += (double)cmp_cnt / TRIALS;
            }
        }
    }

    printf("=== Main experiment (one random data set per size) ===\n\n");
    for (int s = 0; s < ns; s++) {
        printf("Data Size : %d\n\n", sizes[s]);
        for (int k = 0; k < 3; k++) printf("%s : %lld comparisons\n", LABEL[k], single[s][k]);
        printf("\n");
    }
    printf("All results verified (ascending + same elements): %s\n\n", all_ok ? "YES" : "NO");

    printf("=== Supplementary: average of %d trials per size ===\n\n", TRIALS);
    printf("%-6s %16s %16s %16s\n", "n", "Insertion", "Shell(Original)", "Shell(Knuth)");
    for (int s = 0; s < ns; s++)
        printf("%-6d %16.0f %16.0f %16.0f\n", sizes[s], avg[s][0], avg[s][1], avg[s][2]);

    printf("\n=== Growth ratio when n doubles (average) ===\n");
    for (int s = 1; s < 5; s++)
        printf("%5d -> %5d : Insertion x%.2f, Original x%.2f, Knuth x%.2f\n", sizes[s-1], sizes[s],
               avg[s][0]/avg[s-1][0], avg[s][1]/avg[s-1][1], avg[s][2]/avg[s-1][2]);

    printf("\n=== Ratio vs Insertion (average) ===\n");
    for (int s = 0; s < ns; s++)
        printf("n=%-6d Original: %.1f%%  Knuth: %.1f%%\n", sizes[s], 100*avg[s][1]/avg[s][0], 100*avg[s][2]/avg[s][0]);

    /* 사용된 간격 수열 예시 */
    printf("\n=== Gap sequences for n=20000 ===\nOriginal: ");
    for (int g = 20000/2; g >= 1; g /= 2) printf("%d ", g);
    printf("\nKnuth   : ");
    { int h = 1; while (h*3+1 < 20000) h = h*3+1; for (; h >= 1; h /= 3) printf("%d ", h); }
    printf("\n");
    return 0;
}