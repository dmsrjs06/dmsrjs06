#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N 10000
#define TRIALS 100
#define MAXKEY 1000000

typedef struct { int key1, key2, id; } Data;   /* id: 원래 입력 순서 (정렬 기준 아님) */

static long long cmp_cnt, mov_cnt;

/* ---------- 정렬 알고리즘 ---------- */
void selection_sort(Data *a, int n) {
    for (int i = 0; i < n - 1; i++) {
        int m = i;
        for (int j = i + 1; j < n; j++) {
            cmp_cnt++;
            if (a[j].key1 < a[m].key1) m = j;
        }
        if (m != i) {                       /* 위치가 다를 때만 교환 */
            Data t = a[i]; mov_cnt++;
            a[i] = a[m];   mov_cnt++;
            a[m] = t;      mov_cnt++;
        }
    }
}

void bubble_sort(Data *a, int n, int early) {
    for (int i = 0; i < n - 1; i++) {
        int swapped = 0;
        for (int j = 0; j < n - 1 - i; j++) {
            cmp_cnt++;
            if (a[j].key1 > a[j + 1].key1) {
                Data t = a[j];  mov_cnt++;
                a[j] = a[j+1];  mov_cnt++;
                a[j+1] = t;     mov_cnt++;
                swapped = 1;
            }
        }
        if (early && !swapped) break;       /* 조기 종료 */
    }
}

/* strict=1: Case A (>), strict=0: Case B (>=) */
void insertion_sort(Data *a, int n, int strict) {
    for (int i = 1; i < n; i++) {
        Data cur = a[i]; mov_cnt++;         /* 임시 변수 저장 */
        int j = i - 1;
        while (j >= 0) {
            cmp_cnt++;
            int shift = strict ? (a[j].key1 > cur.key1) : (a[j].key1 >= cur.key1);
            if (!shift) break;
            a[j + 1] = a[j]; mov_cnt++;     /* 한 칸 이동 */
            j--;
        }
        a[j + 1] = cur; mov_cnt++;          /* 최종 위치 대입 */
    }
}

/* ---------- 알고리즘 목록 ---------- */
enum { SEL, BUB, BUBE, INS_A, INS_B, NALGO };
const char *names[NALGO] = {"Selection Sort", "Bubble Sort", "Bubble Sort (Early Termination)",
                            "Insertion Sort", "Insertion Sort (>=)"};

void run_algo(int k, Data *a, int n) {
    switch (k) {
        case SEL:   selection_sort(a, n); break;
        case BUB:   bubble_sort(a, n, 0); break;
        case BUBE:  bubble_sort(a, n, 1); break;
        case INS_A: insertion_sort(a, n, 1); break;
        case INS_B: insertion_sort(a, n, 0); break;
    }
}

/* ---------- 검증 ---------- */
int is_sorted(const Data *a, int n) {
    for (int i = 0; i + 1 < n; i++) if (a[i].key1 > a[i+1].key1) return 0;
    return 1;
}
/* 정렬 결과가 원본의 순열인지(데이터 유실 여부) 확인: id 합 + key 일치 */
int is_permutation(const Data *a, const Data *orig, int n) {
    char *seen = calloc(n, 1); int ok = 1;
    for (int i = 0; i < n && ok; i++) {
        int id = a[i].id;
        if (id < 0 || id >= n || seen[id] ||
            a[i].key1 != orig[id].key1 || a[i].key2 != orig[id].key2) ok = 0;
        else seen[id] = 1;
    }
    free(seen); return ok;
}
/* 안정성: key1이 같은 인접 쌍에서 id가 증가해야 함. 위반 횟수 반환 */
int stability_violations(const Data *a, int n) {
    int v = 0;
    for (int i = 0; i + 1 < n; i++)
        if (a[i].key1 == a[i+1].key1 && a[i].id > a[i+1].id) v++;
    return v;
}

/* ---------- 데이터 생성 ---------- */
void gen_random(Data *a, int n) {
    for (int i = 0; i < n; i++) { a[i].key1 = rand() % (MAXKEY+1); a[i].key2 = rand() % (MAXKEY+1); a[i].id = i; }
}
int cmp_asc(const void *x, const void *y) {   /* 시험 입력 준비용 (측정 제외) */
    const Data *p = x, *q = y;
    if (p->key1 != q->key1) return p->key1 < q->key1 ? -1 : 1;
    return p->id - q->id;
}
int cmp_desc(const void *x, const void *y) {
    const Data *p = x, *q = y;
    if (p->key1 != q->key1) return p->key1 > q->key1 ? -1 : 1;
    return p->id - q->id;
}

#define PARTIAL_SWAPS 200   /* 정렬 데이터에서 무작위 위치 쌍 200번(=2%의 원소) 교환 */

int main(void) {
    srand(20240601);
    static Data orig[N], work[N];
    static long long tc[NALGO], tm[NALGO];
    int all_ok = 1;
    static Data first_base[N];

    for (int t = 0; t < TRIALS; t++) {
        gen_random(orig, N);
        if (t == 0) memcpy(first_base, orig, sizeof orig);   /* 입력 상태별 실험의 원본 */
        for (int k = 0; k < NALGO; k++) {
            memcpy(work, orig, sizeof orig);                 /* 동일한 복사본 */
            cmp_cnt = mov_cnt = 0;
            run_algo(k, work, N);
            if (!is_sorted(work, N) || !is_permutation(work, orig, N)) all_ok = 0;
            tc[k] += cmp_cnt; tm[k] += mov_cnt;
        }
    }

    printf("Data Count : %d\n\n", N);
    printf("Random Data\nNumber of Trials : %d\n", TRIALS);
    printf("(All results verified sorted & permutation-intact: %s)\n\n", all_ok ? "YES" : "NO");
    for (int k = 0; k < NALGO; k++) {
        if (k == INS_B) continue;
        printf("%s\n", names[k]);
        printf("Total Comparisons   : %lld\n", tc[k]);
        printf("Average Comparisons : %.2f\n", (double)tc[k] / TRIALS);
        printf("Average Movements   : %.2f\n\n", (double)tm[k] / TRIALS);
    }
    printf("Insertion Sort (>=) [reference]\n");
    printf("Total Comparisons   : %lld\n", tc[INS_B]);
    printf("Average Comparisons : %.2f\n", (double)tc[INS_B] / TRIALS);
    printf("Average Movements   : %.2f\n\n", (double)tm[INS_B] / TRIALS);

    /* ----- 입력 상태별 ----- */
    static Data sorted[N], rev[N], part[N];
    memcpy(sorted, first_base, sizeof sorted); qsort(sorted, N, sizeof(Data), cmp_asc);
    memcpy(rev, first_base, sizeof rev);       qsort(rev, N, sizeof(Data), cmp_desc);
    memcpy(part, sorted, sizeof part);
    for (int s = 0; s < PARTIAL_SWAPS; s++) {
        int i = rand() % N, j = rand() % N;
        Data t = part[i]; part[i] = part[j]; part[j] = t;
    }
    Data *inputs[3] = {sorted, rev, part};
    const char *iname[3] = {"Sorted Data", "Reverse-Sorted Data", "Partially Sorted Data"};
    for (int s = 0; s < 3; s++) {
        printf("------------------------------------\n\n%s\n\n", iname[s]);
        for (int k = 0; k < NALGO; k++) {
            memcpy(work, inputs[s], sizeof work);
            cmp_cnt = mov_cnt = 0;
            run_algo(k, work, N);
            printf("%s\nComparisons : %lld\nMovements   : %lld\n", names[k], cmp_cnt, mov_cnt);
            printf("Sorted check: %s\n\n", is_sorted(work, N) && is_permutation(work, first_base, N) ? "OK" : "FAIL");
        }
    }

    /* ----- 안정성 검증 ----- */
    printf("------------------------------------\n\nStability Verification\n\n");
    /* (1) 과제 예시 4개 데이터 */
    Data ex[4] = {{527,100,0},{527,200,1},{125,300,2},{892,400,3}};
    printf("[Example data] original: ");
    for (int i = 0; i < 4; i++) printf("(%d,%d) ", ex[i].key1, ex[i].key2);
    printf("\n");
    int algo_order[4] = {SEL, BUB, INS_A, INS_B};
    const char *alabel[4] = {"Selection Sort", "Bubble Sort", "Insertion (>)", "Insertion (>=)"};
    for (int q = 0; q < 4; q++) {
        Data tmp[4]; memcpy(tmp, ex, sizeof tmp);
        cmp_cnt = mov_cnt = 0; run_algo(algo_order[q], tmp, 4);
        printf("  %-16s -> ", alabel[q]);
        for (int i = 0; i < 4; i++) printf("(%d,%d) ", tmp[i].key1, tmp[i].key2);
        printf("\n");
    }
    printf("\n");

    /* (2) 중복이 많은 시험 데이터: 2000개, key1 in [0,99] (평균 값당 20개) + 랜덤 10000개 */
    static Data dup[2000], dup_w[2000];
    for (int i = 0; i < 2000; i++) { dup[i].key1 = rand() % 100; dup[i].key2 = rand() % (MAXKEY+1); dup[i].id = i; }
    printf("[Duplicate-heavy data] 2000 items, key1 in [0,99]\n");
    for (int q = 0; q < 4; q++) {
        memcpy(dup_w, dup, sizeof dup);
        cmp_cnt = mov_cnt = 0; run_algo(algo_order[q], dup_w, 2000);
        int v = stability_violations(dup_w, 2000);
        printf("  %-16s : sorted=%s, order violations=%d -> %s\n", alabel[q],
               is_sorted(dup_w, 2000) ? "OK" : "FAIL", v, v == 0 ? "STABLE" : "UNSTABLE");
    }
    printf("\n");

    /* (3) 무작위 10000개 데이터 (자연 발생 중복) */
    int pairs = 0;
    { static Data s[N]; memcpy(s, first_base, sizeof s); qsort(s, N, sizeof(Data), cmp_asc);
      for (int i = 0; i + 1 < N; i++) if (s[i].key1 == s[i+1].key1) pairs++; }
    printf("[Random 10000 data] adjacent equal-key pairs after sorting: %d\n", pairs);
    for (int q = 0; q < 4; q++) {
        memcpy(work, first_base, sizeof work);
        cmp_cnt = mov_cnt = 0; run_algo(algo_order[q], work, N);
        int v = stability_violations(work, N);
        printf("  %-16s : order violations=%d -> %s\n", alabel[q], v, v == 0 ? "STABLE" : "UNSTABLE");
    }
    printf("\n");

    /* 최종 요약 */
    int res[4];
    for (int q = 0; q < 4; q++) {
        memcpy(dup_w, dup, sizeof dup);
        run_algo(algo_order[q], dup_w, 2000);
        res[q] = stability_violations(dup_w, 2000) == 0;
    }
    printf("Selection Sort       : %s\n", res[0] ? "Stable" : "Unstable");
    printf("Bubble Sort          : %s\n", res[1] ? "Stable" : "Unstable");
    printf("Insertion Sort (>)   : %s\n", res[2] ? "Stable" : "Unstable");
    printf("Insertion Sort (>=)  : %s\n", res[3] ? "Stable" : "Unstable");
    return 0;
=======
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N 10000
#define TRIALS 100
#define MAXKEY 1000000

typedef struct { int key1, key2, id; } Data;   /* id: 원래 입력 순서 (정렬 기준 아님) */

static long long cmp_cnt, mov_cnt;

/* ---------- 정렬 알고리즘 ---------- */
void selection_sort(Data *a, int n) {
    for (int i = 0; i < n - 1; i++) {
        int m = i;
        for (int j = i + 1; j < n; j++) {
            cmp_cnt++;
            if (a[j].key1 < a[m].key1) m = j;
        }
        if (m != i) {                       /* 위치가 다를 때만 교환 */
            Data t = a[i]; mov_cnt++;
            a[i] = a[m];   mov_cnt++;
            a[m] = t;      mov_cnt++;
        }
    }
}

void bubble_sort(Data *a, int n, int early) {
    for (int i = 0; i < n - 1; i++) {
        int swapped = 0;
        for (int j = 0; j < n - 1 - i; j++) {
            cmp_cnt++;
            if (a[j].key1 > a[j + 1].key1) {
                Data t = a[j];  mov_cnt++;
                a[j] = a[j+1];  mov_cnt++;
                a[j+1] = t;     mov_cnt++;
                swapped = 1;
            }
        }
        if (early && !swapped) break;       /* 조기 종료 */
    }
}

/* strict=1: Case A (>), strict=0: Case B (>=) */
void insertion_sort(Data *a, int n, int strict) {
    for (int i = 1; i < n; i++) {
        Data cur = a[i]; mov_cnt++;         /* 임시 변수 저장 */
        int j = i - 1;
        while (j >= 0) {
            cmp_cnt++;
            int shift = strict ? (a[j].key1 > cur.key1) : (a[j].key1 >= cur.key1);
            if (!shift) break;
            a[j + 1] = a[j]; mov_cnt++;     /* 한 칸 이동 */
            j--;
        }
        a[j + 1] = cur; mov_cnt++;          /* 최종 위치 대입 */
    }
}

/* ---------- 알고리즘 목록 ---------- */
enum { SEL, BUB, BUBE, INS_A, INS_B, NALGO };
const char *names[NALGO] = {"Selection Sort", "Bubble Sort", "Bubble Sort (Early Termination)",
                            "Insertion Sort", "Insertion Sort (>=)"};

void run_algo(int k, Data *a, int n) {
    switch (k) {
        case SEL:   selection_sort(a, n); break;
        case BUB:   bubble_sort(a, n, 0); break;
        case BUBE:  bubble_sort(a, n, 1); break;
        case INS_A: insertion_sort(a, n, 1); break;
        case INS_B: insertion_sort(a, n, 0); break;
    }
}

/* ---------- 검증 ---------- */
int is_sorted(const Data *a, int n) {
    for (int i = 0; i + 1 < n; i++) if (a[i].key1 > a[i+1].key1) return 0;
    return 1;
}
/* 정렬 결과가 원본의 순열인지(데이터 유실 여부) 확인: id 합 + key 일치 */
int is_permutation(const Data *a, const Data *orig, int n) {
    char *seen = calloc(n, 1); int ok = 1;
    for (int i = 0; i < n && ok; i++) {
        int id = a[i].id;
        if (id < 0 || id >= n || seen[id] ||
            a[i].key1 != orig[id].key1 || a[i].key2 != orig[id].key2) ok = 0;
        else seen[id] = 1;
    }
    free(seen); return ok;
}
/* 안정성: key1이 같은 인접 쌍에서 id가 증가해야 함. 위반 횟수 반환 */
int stability_violations(const Data *a, int n) {
    int v = 0;
    for (int i = 0; i + 1 < n; i++)
        if (a[i].key1 == a[i+1].key1 && a[i].id > a[i+1].id) v++;
    return v;
}

/* ---------- 데이터 생성 ---------- */
void gen_random(Data *a, int n) {
    for (int i = 0; i < n; i++) { a[i].key1 = rand() % (MAXKEY+1); a[i].key2 = rand() % (MAXKEY+1); a[i].id = i; }
}
int cmp_asc(const void *x, const void *y) {   /* 시험 입력 준비용 (측정 제외) */
    const Data *p = x, *q = y;
    if (p->key1 != q->key1) return p->key1 < q->key1 ? -1 : 1;
    return p->id - q->id;
}
int cmp_desc(const void *x, const void *y) {
    const Data *p = x, *q = y;
    if (p->key1 != q->key1) return p->key1 > q->key1 ? -1 : 1;
    return p->id - q->id;
}

#define PARTIAL_SWAPS 200   /* 정렬 데이터에서 무작위 위치 쌍 200번(=2%의 원소) 교환 */

int main(void) {
    srand(20240601);
    static Data orig[N], work[N];
    static long long tc[NALGO], tm[NALGO];
    int all_ok = 1;
    static Data first_base[N];

    for (int t = 0; t < TRIALS; t++) {
        gen_random(orig, N);
        if (t == 0) memcpy(first_base, orig, sizeof orig);   /* 입력 상태별 실험의 원본 */
        for (int k = 0; k < NALGO; k++) {
            memcpy(work, orig, sizeof orig);                 /* 동일한 복사본 */
            cmp_cnt = mov_cnt = 0;
            run_algo(k, work, N);
            if (!is_sorted(work, N) || !is_permutation(work, orig, N)) all_ok = 0;
            tc[k] += cmp_cnt; tm[k] += mov_cnt;
        }
    }

    printf("Data Count : %d\n\n", N);
    printf("Random Data\nNumber of Trials : %d\n", TRIALS);
    printf("(All results verified sorted & permutation-intact: %s)\n\n", all_ok ? "YES" : "NO");
    for (int k = 0; k < NALGO; k++) {
        if (k == INS_B) continue;
        printf("%s\n", names[k]);
        printf("Total Comparisons   : %lld\n", tc[k]);
        printf("Average Comparisons : %.2f\n", (double)tc[k] / TRIALS);
        printf("Average Movements   : %.2f\n\n", (double)tm[k] / TRIALS);
    }
    printf("Insertion Sort (>=) [reference]\n");
    printf("Total Comparisons   : %lld\n", tc[INS_B]);
    printf("Average Comparisons : %.2f\n", (double)tc[INS_B] / TRIALS);
    printf("Average Movements   : %.2f\n\n", (double)tm[INS_B] / TRIALS);

    /* ----- 입력 상태별 ----- */
    static Data sorted[N], rev[N], part[N];
    memcpy(sorted, first_base, sizeof sorted); qsort(sorted, N, sizeof(Data), cmp_asc);
    memcpy(rev, first_base, sizeof rev);       qsort(rev, N, sizeof(Data), cmp_desc);
    memcpy(part, sorted, sizeof part);
    for (int s = 0; s < PARTIAL_SWAPS; s++) {
        int i = rand() % N, j = rand() % N;
        Data t = part[i]; part[i] = part[j]; part[j] = t;
    }
    Data *inputs[3] = {sorted, rev, part};
    const char *iname[3] = {"Sorted Data", "Reverse-Sorted Data", "Partially Sorted Data"};
    for (int s = 0; s < 3; s++) {
        printf("------------------------------------\n\n%s\n\n", iname[s]);
        for (int k = 0; k < NALGO; k++) {
            memcpy(work, inputs[s], sizeof work);
            cmp_cnt = mov_cnt = 0;
            run_algo(k, work, N);
            printf("%s\nComparisons : %lld\nMovements   : %lld\n", names[k], cmp_cnt, mov_cnt);
            printf("Sorted check: %s\n\n", is_sorted(work, N) && is_permutation(work, first_base, N) ? "OK" : "FAIL");
        }
    }

    /* ----- 안정성 검증 ----- */
    printf("------------------------------------\n\nStability Verification\n\n");
    /* (1) 과제 예시 4개 데이터 */
    Data ex[4] = {{527,100,0},{527,200,1},{125,300,2},{892,400,3}};
    printf("[Example data] original: ");
    for (int i = 0; i < 4; i++) printf("(%d,%d) ", ex[i].key1, ex[i].key2);
    printf("\n");
    int algo_order[4] = {SEL, BUB, INS_A, INS_B};
    const char *alabel[4] = {"Selection Sort", "Bubble Sort", "Insertion (>)", "Insertion (>=)"};
    for (int q = 0; q < 4; q++) {
        Data tmp[4]; memcpy(tmp, ex, sizeof tmp);
        cmp_cnt = mov_cnt = 0; run_algo(algo_order[q], tmp, 4);
        printf("  %-16s -> ", alabel[q]);
        for (int i = 0; i < 4; i++) printf("(%d,%d) ", tmp[i].key1, tmp[i].key2);
        printf("\n");
    }
    printf("\n");

    /* (2) 중복이 많은 시험 데이터: 2000개, key1 in [0,99] (평균 값당 20개) + 랜덤 10000개 */
    static Data dup[2000], dup_w[2000];
    for (int i = 0; i < 2000; i++) { dup[i].key1 = rand() % 100; dup[i].key2 = rand() % (MAXKEY+1); dup[i].id = i; }
    printf("[Duplicate-heavy data] 2000 items, key1 in [0,99]\n");
    for (int q = 0; q < 4; q++) {
        memcpy(dup_w, dup, sizeof dup);
        cmp_cnt = mov_cnt = 0; run_algo(algo_order[q], dup_w, 2000);
        int v = stability_violations(dup_w, 2000);
        printf("  %-16s : sorted=%s, order violations=%d -> %s\n", alabel[q],
               is_sorted(dup_w, 2000) ? "OK" : "FAIL", v, v == 0 ? "STABLE" : "UNSTABLE");
    }
    printf("\n");

    /* (3) 무작위 10000개 데이터 (자연 발생 중복) */
    int pairs = 0;
    { static Data s[N]; memcpy(s, first_base, sizeof s); qsort(s, N, sizeof(Data), cmp_asc);
      for (int i = 0; i + 1 < N; i++) if (s[i].key1 == s[i+1].key1) pairs++; }
    printf("[Random 10000 data] adjacent equal-key pairs after sorting: %d\n", pairs);
    for (int q = 0; q < 4; q++) {
        memcpy(work, first_base, sizeof work);
        cmp_cnt = mov_cnt = 0; run_algo(algo_order[q], work, N);
        int v = stability_violations(work, N);
        printf("  %-16s : order violations=%d -> %s\n", alabel[q], v, v == 0 ? "STABLE" : "UNSTABLE");
    }
    printf("\n");

    /* 최종 요약 */
    int res[4];
    for (int q = 0; q < 4; q++) {
        memcpy(dup_w, dup, sizeof dup);
        run_algo(algo_order[q], dup_w, 2000);
        res[q] = stability_violations(dup_w, 2000) == 0;
    }
    printf("Selection Sort       : %s\n", res[0] ? "Stable" : "Unstable");
    printf("Bubble Sort          : %s\n", res[1] ? "Stable" : "Unstable");
    printf("Insertion Sort (>)   : %s\n", res[2] ? "Stable" : "Unstable");
    printf("Insertion Sort (>=)  : %s\n", res[3] ? "Stable" : "Unstable");
    return 0;
>>>>>>> 5c8c084329635ab3497deaf0f3e98ed1dcb17655
}