#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAXV 1000000
/* Windows rand() returns at most 32767, so call it twice to build a 30-bit random number */
#define RAND30() (((rand() & 0x7FFF) << 15) | (rand() & 0x7FFF))
#define RAND_VAL() (RAND30() % (MAXV + 1))
#define MAXN 20000
#define MAXOPS 4000
#define CAP (MAXN + MAXOPS + 10)

static long long cmp_cnt, mov_cnt;        /* value comparisons / data movements */
static long long phase_cmp, phase_mov;    /* snapshot after the sorting phase */
static int ins_done, del_done, del_miss;  /* operation statistics */

/* ---- stack usage: record the lowest local-variable address seen in recursive calls ---- */
static int track_stack = 0;
static char *stack_top, *stack_min;
#define ENTER() char probe_; if (track_stack && (char *)&probe_ < stack_min) stack_min = (char *)&probe_

/* one three-way comparison of data values counts as one comparison */
static int cmp3(int x, int y) { cmp_cnt++; return (x > y) - (x < y); }

/* ================= Quick sort (Median-of-Three pivot) ================= */
static void swap3(int *x, int *y) { int t = *x; *x = *y; *y = t; mov_cnt += 3; }

static int median_index(const int *a, int l, int r) {
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
static int partition(int *a, int l, int r) {
    int p = median_index(a, l, r);
    if (p != l) swap3(&a[l], &a[p]);
    int pivot = a[l], i = l, j = r + 1;
    for (;;) {
        for (;;) { i++; if (i > r) break; cmp_cnt++; if (!(a[i] < pivot)) break; }
        for (;;) { j--; cmp_cnt++; if (!(a[j] > pivot)) break; }
        if (i >= j) break;
        swap3(&a[i], &a[j]);
    }
    if (l != j) swap3(&a[l], &a[j]);
    return j;
}
static void quick_rec(int *a, int l, int r) {
    ENTER();
    if (l < r) {
        int p = partition(a, l, r);
        quick_rec(a, l, p - 1);
        quick_rec(a, p + 1, r);
    }
}

/* ================= Sorted array: insert / delete ================= */
static int arr[CAP];
static int arr_size, arr_peak;

/* linear search for the first index whose value >= v; *eq tells whether it equals v */
static int arr_find(int v, int *eq) {
    int i = 0, c = -1;
    for (; i < arr_size; i++) {
        c = cmp3(arr[i], v);
        if (c >= 0) break;
    }
    *eq = (i < arr_size && c == 0);
    return i;
}
static void arr_insert(int v) {
    int eq, pos = arr_find(v, &eq);
    for (int j = arr_size; j > pos; j--) { arr[j] = arr[j - 1]; mov_cnt++; }
    arr[pos] = v; mov_cnt++;
    arr_size++;
    if (arr_size > arr_peak) arr_peak = arr_size;
}
static int arr_delete(int v) {
    int eq, pos = arr_find(v, &eq);
    if (!eq) return 0;
    for (int j = pos; j < arr_size - 1; j++) { arr[j] = arr[j + 1]; mov_cnt++; }
    arr_size--;
    return 1;
}

/* ================= AVL tree (duplicates stored as a count in the node) ================= */
typedef struct Node { int key, cnt, h; struct Node *l, *r; } Node;
static Node *root;
static long long live_nodes, peak_nodes, tree_total;

static int hgt(const Node *t) { return t ? t->h : 0; }
static void upd(Node *t) { int a = hgt(t->l), b = hgt(t->r); t->h = 1 + (a > b ? a : b); }
static Node *rot_right(Node *y) { Node *x = y->l; y->l = x->r; x->r = y; upd(y); upd(x); return x; }
static Node *rot_left(Node *x)  { Node *y = x->r; x->r = y->l; y->l = x; upd(x); upd(y); return y; }
static Node *rebalance(Node *t) {      /* control comparisons only: not counted */
    upd(t);
    int bf = hgt(t->l) - hgt(t->r);
    if (bf > 1) {
        if (hgt(t->l->l) < hgt(t->l->r)) t->l = rot_left(t->l);
        return rot_right(t);
    }
    if (bf < -1) {
        if (hgt(t->r->r) < hgt(t->r->l)) t->r = rot_right(t->r);
        return rot_left(t);
    }
    return t;
}
static Node *avl_insert(Node *t, int v) {
    ENTER();
    if (!t) {
        Node *n = (Node *)malloc(sizeof(Node));
        n->key = v; mov_cnt++;                 /* storing the value in the node */
        n->cnt = 1; n->h = 1; n->l = n->r = NULL;
        live_nodes++;
        if (live_nodes > peak_nodes) peak_nodes = live_nodes;
        return n;
    }
    int c = cmp3(v, t->key);
    if (c < 0) t->l = avl_insert(t->l, v);
    else if (c > 0) t->r = avl_insert(t->r, v);
    else { t->cnt++; return t; }               /* duplicate: only the counter changes */
    return rebalance(t);
}
/* unlink the leftmost node of subtree t; it is returned through *out */
static Node *detach_min(Node *t, Node **out) {
    ENTER();
    if (!t->l) { *out = t; return t->r; }
    t->l = detach_min(t->l, out);
    return rebalance(t);
}
static Node *avl_delete(Node *t, int v, int *removed) {
    ENTER();
    if (!t) return NULL;
    int c = cmp3(v, t->key);
    if (c < 0) t->l = avl_delete(t->l, v, removed);
    else if (c > 0) t->r = avl_delete(t->r, v, removed);
    else {
        *removed = 1;
        if (t->cnt > 1) { t->cnt--; return t; }
        if (!t->l || !t->r) {
            Node *child = t->l ? t->l : t->r;
            free(t); live_nodes--;
            return child;
        }
        Node *s;
        t->r = detach_min(t->r, &s);           /* in-order successor */
        t->key = s->key; mov_cnt++;            /* copy successor value */
        t->cnt = s->cnt; mov_cnt++;            /* copy successor duplicate count */
        free(s); live_nodes--;
    }
    return rebalance(t);
}
static void free_tree(Node *t) { if (t) { free_tree(t->l); free_tree(t->r); free(t); } }

/* ================= Operations (shared by both structures) ================= */
typedef struct { int is_insert, value; } Op;
static Op ops[MAXOPS];

static void array_experiment(int n, int nops) {
    arr_size = n; arr_peak = n;
    cmp_cnt = mov_cnt = 0;
    quick_rec(arr, 0, n - 1);
    phase_cmp = cmp_cnt; phase_mov = mov_cnt;
    ins_done = del_done = del_miss = 0;
    for (int i = 0; i < nops; i++) {
        if (ops[i].is_insert) { arr_insert(ops[i].value); ins_done++; }
        else if (arr_delete(ops[i].value)) del_done++;
        else del_miss++;
    }
}
static void avl_experiment(const int *orig, int n, int nops) {
    root = NULL; live_nodes = peak_nodes = tree_total = 0;
    cmp_cnt = mov_cnt = 0;
    for (int i = 0; i < n; i++) { root = avl_insert(root, orig[i]); tree_total++; }
    phase_cmp = cmp_cnt; phase_mov = mov_cnt;
    ins_done = del_done = del_miss = 0;
    for (int i = 0; i < nops; i++) {
        if (ops[i].is_insert) { root = avl_insert(root, ops[i].value); tree_total++; ins_done++; }
        else {
            int removed = 0;
            root = avl_delete(root, ops[i].value, &removed);
            if (removed) { tree_total--; del_done++; } else del_miss++;
        }
    }
}

/* ================= Verification (never timed or counted) ================= */
static int is_sorted(const int *a, int n) { for (int i = 0; i + 1 < n; i++) if (a[i] > a[i + 1]) return 0; return 1; }
static void inorder_fill(const Node *t, int *out, int *k) {
    if (!t) return;
    inorder_fill(t->l, out, k);
    for (int c = 0; c < t->cnt; c++) out[(*k)++] = t->key;
    inorder_fill(t->r, out, k);
}
static int avl_valid(const Node *t) {   /* returns height, or -1 if the AVL property / height field is broken */
    if (!t) return 0;
    int a = avl_valid(t->l), b = avl_valid(t->r);
    if (a < 0 || b < 0) return -1;
    int d = a - b;
    if (d > 1 || d < -1) return -1;
    int h = 1 + (a > b ? a : b);
    return h == t->h ? h : -1;
}
static int cmp_int(const void *x, const void *y) { int p = *(const int *)x, q = *(const int *)y; return (p > q) - (p < q); }

static double now_ms(void) {
    static long long base_sec = -1;
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    if (base_sec < 0) base_sec = (long long)ts.tv_sec;
    return (double)((long long)ts.tv_sec - base_sec) * 1000.0 + (double)ts.tv_nsec / 1e6;
}

typedef struct {
    double time_ms;
    long long cmp, mov, sort_cmp, sort_mov, mem, final_count;
    int ins, del, miss, ok;
} Result;

/* mode 0: delete value = new random number (assignment rule); mode 1: delete value taken from existing data */
static void make_ops(const int *orig, int n, int nops, int mode) {
    for (int i = 0; i < nops; i++) {
        int selector = RAND30();
        ops[i].is_insert = (selector % 2 == 0);
        if (!ops[i].is_insert && mode == 1) ops[i].value = orig[RAND30() % n];
        else ops[i].value = RAND_VAL();
    }
}

static void measure(const int *orig, int n, int nops, Result *qs, Result *av) {
    static int tmp[CAP], ref[MAXN];
    int REPS = 20;
    memset(qs, 0, sizeof *qs); memset(av, 0, sizeof *av);

    /* ---- Quick sort + sorted array ---- */
    double total = 0;
    for (int r = 0; r < REPS; r++) {
        memcpy(arr, orig, n * sizeof(int));          /* copy: excluded from timing */
        double t0 = now_ms();
        array_experiment(n, nops);
        total += now_ms() - t0;
        if (r == 0) {
            qs->cmp = cmp_cnt; qs->mov = mov_cnt; qs->sort_cmp = phase_cmp; qs->sort_mov = phase_mov;
            qs->ins = ins_done; qs->del = del_done; qs->miss = del_miss; qs->final_count = arr_size;
        }
    }
    qs->time_ms = total / REPS;
    { /* memory: peak array size + measured peak stack */
        char base_;
        memcpy(arr, orig, n * sizeof(int));
        track_stack = 1; stack_top = &base_; stack_min = &base_;
        array_experiment(n, nops);
        track_stack = 0;
        qs->mem = (long long)arr_peak * (long long)sizeof(int) + (long long)(stack_top - stack_min);
    }
    int arr_final[CAP], arr_final_n = arr_size;       /* kept for the content comparison */
    memcpy(arr_final, arr, arr_size * sizeof(int));

    /* ---- AVL tree ---- */
    total = 0;
    for (int r = 0; r < REPS; r++) {
        double t0 = now_ms();
        avl_experiment(orig, n, nops);
        total += now_ms() - t0;
        if (r == 0) {
            av->cmp = cmp_cnt; av->mov = mov_cnt; av->sort_cmp = phase_cmp; av->sort_mov = phase_mov;
            av->ins = ins_done; av->del = del_done; av->miss = del_miss; av->final_count = tree_total;
        }
        free_tree(root); root = NULL;                /* cleanup is not timed */
    }
    av->time_ms = total / REPS;
    {
        char base_;
        track_stack = 1; stack_top = &base_; stack_min = &base_;
        avl_experiment(orig, n, nops);
        track_stack = 0;
        av->mem = peak_nodes * (long long)sizeof(Node) + (long long)(stack_top - stack_min);
    }

    /* ---- verification ---- */
    int k = 0;
    inorder_fill(root, tmp, &k);
    int same = (k == arr_final_n) && memcmp(tmp, arr_final, k * sizeof(int)) == 0;
    qs->ok = is_sorted(arr_final, arr_final_n);
    av->ok = same && is_sorted(tmp, k) && avl_valid(root) >= 0 && tree_total == k;
    free_tree(root); root = NULL;
    /* after the build phase only: the in-order result must equal the sorted original */
    memcpy(ref, orig, n * sizeof(int)); qsort(ref, n, sizeof(int), cmp_int);
    { int saved = nops; nops = 0; avl_experiment(orig, n, nops); nops = saved; }
    k = 0; inorder_fill(root, tmp, &k);
    if (k != n || memcmp(tmp, ref, n * sizeof(int)) != 0 || avl_valid(root) < 0) av->ok = 0;
    free_tree(root); root = NULL;
    memcpy(arr, orig, n * sizeof(int)); arr_size = n; arr_peak = n;
    quick_rec(arr, 0, n - 1);
    if (memcmp(arr, ref, n * sizeof(int)) != 0) qs->ok = 0;
}

int main(void) {
    int sizes[] = {1000, 2000, 4000, 8000, 16000, 20000};
    int ns = 6;
    static int orig[MAXN];
    static Result Q[2][6], A[2][6];
    int ok_all = 1;
    srand(20240601);

    for (int s = 0; s < ns; s++) {
        int n = sizes[s], nops = n / 5;
        for (int i = 0; i < n; i++) orig[i] = RAND_VAL();
        for (int mode = 0; mode < 2; mode++) {
            make_ops(orig, n, nops, mode);
            measure(orig, n, nops, &Q[mode][s], &A[mode][s]);
            if (!Q[mode][s].ok || !A[mode][s].ok || Q[mode][s].final_count != A[mode][s].final_count) ok_all = 0;
        }
    }

    printf("=== Main experiment (assignment rule: insert/delete values are new random numbers) ===\n\n");
    for (int s = 0; s < ns; s++) {
        Result *q = &Q[0][s], *a = &A[0][s];
        printf("Data Size : %d\nOperations: %d\n\n", sizes[s], sizes[s] / 5);
        printf("                  Quick Sort      AVL Tree\n");
        printf("------------------------------------------------\n");
        printf("Total Time        %-9.4f ms    %-9.4f ms\n", q->time_ms, a->time_ms);
        printf("Comparisons       %-15lld %lld\n", q->cmp, a->cmp);
        printf("Data Movements    %-15lld %lld\n", q->mov, a->mov);
        printf("Memory Usage      %-9lld Bytes  %-9lld Bytes\n\n", q->mem, a->mem);
        printf("Final Data Count  %-15lld %lld\n", q->final_count, a->final_count);
        printf("(inserts=%d, deletes done=%d, deletes missed=%d)\n\n", q->ins, q->del, q->miss);
    }
    printf("All checks passed (sorted, AVL valid, array == AVL in-order, same counts): %s\n\n", ok_all ? "YES" : "NO");

    printf("=== Phase breakdown of the main experiment: sorting phase / insert-delete phase ===\n");
    printf("%-6s | %-26s | %-26s\n", "n", "Quick: cmp sort / ops, mov sort / ops", "AVL: cmp sort / ops, mov sort / ops");
    for (int s = 0; s < ns; s++) {
        Result *q = &Q[0][s], *a = &A[0][s];
        printf("%-6d | cmp %lld / %lld, mov %lld / %lld | cmp %lld / %lld, mov %lld / %lld\n", sizes[s],
               q->sort_cmp, q->cmp - q->sort_cmp, q->sort_mov, q->mov - q->sort_mov,
               a->sort_cmp, a->cmp - a->sort_cmp, a->sort_mov, a->mov - a->sort_mov);
    }

    printf("\n=== Supplementary: delete values are taken from existing data (deletions really happen) ===\n\n");
    for (int s = 0; s < ns; s++) {
        Result *q = &Q[1][s], *a = &A[1][s];
        printf("n=%d ops=%d (inserts=%d, deletes done=%d, missed=%d)\n", sizes[s], sizes[s] / 5, q->ins, q->del, q->miss);
        printf("  Quick+Array: time %.4f ms, cmp %lld, mov %lld, mem %lld B, final %lld\n", q->time_ms, q->cmp, q->mov, q->mem, q->final_count);
        printf("  AVL Tree   : time %.4f ms, cmp %lld, mov %lld, mem %lld B, final %lld\n", a->time_ms, a->cmp, a->mov, a->mem, a->final_count);
    }
    printf("\nsizeof(Node) = %d bytes, sizeof(int) = %d bytes\n", (int)sizeof(Node), (int)sizeof(int));
    return 0;
}