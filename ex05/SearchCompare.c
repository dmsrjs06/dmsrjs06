#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 100 /* 저장할 정수 개수 */
#define M 50 /* 탐색 대상 개수 */
#define MAXV 1000 /* 값의 범위는 [0, MAXV] */

typedef struct Node {
    int data;
    struct Node *left;
    struct Node *right;
} Node;

static Node *new_node(int v) {
    Node *n = malloc(sizeof(Node));
    if (!n) { fprintf(stderr, "Out of memory\n"); exit(1); }
    n->data = v;
    n->left = n->right = NULL;
    return n;
}

/* v를 root를 루트로 하는 BST에 삽입한다 (반복적). 기존 노드의 값과 v를
 * 비교할 때마다 *cmp를 1씩 증가시킨다. (갱신된) 루트를 반환한다.
 * 삽입되는 값은 기존 값들과 모두 다르다고 가정한다. */
static Node *bst_insert(Node *root, int v, long *cmp) {
    if (!root) return new_node(v);
    Node *cur = root;
    for (;;) {
        (*cmp)++;
        if (v < cur->data) {
            if (!cur->left) { cur->left = new_node(v); return root; }
            cur = cur->left;
        } else { /* v > cur->data (값은 모두 서로 다름이 보장됨) */
            if (!cur->right) { cur->right = new_node(v); return root; }
            cur = cur->right;
        }
    }
}

/* root를 루트로 하는 BST에서 key를 탐색한다 (반복적). key와 노드의 값을
 * 비교할 때마다 *cmp를 1씩 증가시킨다. 찾으면 1, 못 찾으면 0을 반환한다. */
static int bst_search(Node *root, int key, long *cmp) {
    Node *cur = root;
    while (cur) {
        (*cmp)++;
        if (key == cur->data) return 1;
        cur = (key < cur->data) ? cur->left : cur->right;
    }
    return 0;
}

/* BST 구조 통계를 반복적으로 계산한다: 높이(루트 깊이=0, 리프 깊이=height)와
 * 평균 노드 깊이. 깊이 정보를 함께 담은 명시적 스택을 사용한다. */
static void bst_shape_stats(Node *root, int n, int *out_height, double *out_avg_depth) {
    if (!root) { *out_height = -1; *out_avg_depth = 0.0; return; }
    Node *nstack[N + 1];
    int dstack[N + 1];
    int top = 0;
    nstack[top] = root; dstack[top] = 0; top++;
    int max_depth = 0;
    long depth_sum = 0;
    while (top > 0) {
        top--;
        Node *cur = nstack[top];
        int d = dstack[top];
        depth_sum += d;
        if (d > max_depth) max_depth = d;
        if (cur->left)  { nstack[top] = cur->left;  dstack[top] = d + 1; top++; }
        if (cur->right) { nstack[top] = cur->right; dstack[top] = d + 1; top++; }
    }
    *out_height = max_depth;
    *out_avg_depth = (double)depth_sum / n;
}

static void free_tree(Node *root) {
    if (!root) return;
    /* 배열 기반 스택을 이용해 모든 노드를 반복적으로 해제 */
    Node *stack[N + 1];
    int top = 0;
    stack[top++] = root;
    while (top > 0) {
        Node *n = stack[--top];
        if (n->left)  stack[top++] = n->left;
        if (n->right) stack[top++] = n->right;
        free(n);
    }
}

/* arr[0..n-1]에서 순차 탐색을 수행한다. key와 원소를 비교할 때마다
 * *cmp를 1씩 증가시킨다. 찾으면 인덱스를, 못 찾으면 -1을 반환한다. */
static int sequential_search(const int *arr, int n, int key, long *cmp) {
    for (int i = 0; i < n; i++) {
        (*cmp)++;
        if (arr[i] == key) return i;
    }
    return -1;
}

/* [0, MAXV] 범위에서 서로 다른 정수 n개를 생성하여 out[]에 저장한다. */
static void generate_distinct(int *out, int n) {
    static int used[MAXV + 1]; /* static이므로 0으로 초기화됨 */
    int count = 0;
    while (count < n) {
        int v = rand() % (MAXV + 1);
        if (!used[v]) { used[v] = 1; out[count++] = v; }
    }
}

int main(void) {
    srand((unsigned)time(NULL));

    int arr[N];
    generate_distinct(arr, N);

    printf("=== 1. Generated %d distinct integers (insertion order) ===\n", N);
    for (int i = 0; i < N; i++) {
        printf("%4d%s", arr[i], (i + 1) % 10 == 0 ? "\n" : " ");
    }
    if (N % 10 != 0) printf("\n");

    /* 배열 값을 같은 순서로 삽입해 BST를 만들면서, 삽입 위치를 찾는 데
     * 든 비교 횟수를 누적한다. */
    Node *root = NULL;
    long build_cmp = 0;
    for (int i = 0; i < N; i++) root = bst_insert(root, arr[i], &build_cmp);

    printf("\n=== 2. BST construction cost ===\n");
    printf("Total comparisons to build the BST from %d values: %ld\n", N, build_cmp);

    int height; double avg_depth;
    bst_shape_stats(root, N, &height, &avg_depth);
    double ideal_height = 0;
    { double v = N; while (v > 1) { v /= 2; ideal_height += 1; } }
    printf("\n=== 2b. BST shape ===\n");
    printf("Tree height (root depth = 0)      : %d\n", height);
    printf("Average node depth                : %.2f\n", avg_depth);
    printf("Ideal (perfectly balanced) height for %d nodes : about %.1f (log2(%d))\n", N, ideal_height, N);

    /* 탐색 대상 50개를 생성한다. 탐색 대상끼리 중복될 수 있고, 각각은
     * 저장된 값들 중에 있을 수도 없을 수도 있다. */
    int keys[M];
    for (int i = 0; i < M; i++) keys[i] = rand() % (MAXV + 1);

    printf("\n=== 3. Search keys (%d) ===\n", M);
    for (int i = 0; i < M; i++) {
        printf("%4d%s", keys[i], (i + 1) % 10 == 0 ? "\n" : " ");
    }
    if (M % 10 != 0) printf("\n");

    printf("\n=== 4. Per-key search results ===\n");
    printf("%-8s %-8s %-10s %-10s\n", "Key", "Result", "Seq.Cmp", "BST.Cmp");
    long seq_total = 0, bst_total = 0;
    int seq_found_cnt = 0, bst_found_cnt = 0;
    for (int i = 0; i < M; i++) {
        long seq_cmp = 0, bst_cmp = 0;
        int seq_idx = sequential_search(arr, N, keys[i], &seq_cmp);
        int bst_ok  = bst_search(root, keys[i], &bst_cmp);
        seq_total += seq_cmp;
        bst_total += bst_cmp;
        if (seq_idx >= 0) seq_found_cnt++;
        if (bst_ok)       bst_found_cnt++;
        printf("%-8d %-8s %-10ld %-10ld\n",
               keys[i], seq_idx >= 0 ? "Found" : "NotFnd", seq_cmp, bst_cmp);
        /* 같은 데이터 집합을 탐색하므로 순차 탐색과 BST 탐색의
         * 성공/실패 여부는 항상 서로 일치해야 한다. */
        if ((seq_idx >= 0) != bst_ok) {
            fprintf(stderr, "Warning: search result mismatch for key %d\n", keys[i]);
        }
    }

    printf("\n=== 5. Summary ===\n");
    printf("Number of searches : %d\n", M);
    printf("Sequential search - found : %d, not found: %d\n", seq_found_cnt, M - seq_found_cnt);
    printf("BST search - found : %d, not found: %d\n", bst_found_cnt, M - bst_found_cnt);
    printf("\nSequential Search\n");
    printf("  Total comparisons : %ld\n", seq_total);
    printf("  Average comparisons : %.2f\n", (double)seq_total / M);
    printf("\nBST Search\n");
    printf("  Total comparisons : %ld\n", bst_total);
    printf("  Average comparisons : %.2f\n", (double)bst_total / M);
    printf("\nBST build cost (one-time)\n");
    printf("  Total comparisons : %ld\n", build_cmp);
    printf("  Average per insert : %.2f\n", (double)build_cmp / N);

    printf("\n=== 6. Combined cost comparison ===\n");
    printf("Sequential search total (over %d searches) : %ld\n", M, seq_total);
    printf("BST search total (over %d searches) : %ld\n", M, bst_total);
    printf("BST search total + one-time BST build cost : %ld\n", bst_total + build_cmp);
    printf("Break-even point reached after this many searches (approx) : ");
    if (seq_total <= bst_total) {
        printf("N/A (sequential search already cheaper over %d searches)\n", M);
    } else {
        double per_seq = (double)seq_total / M;
        double per_bst = (double)bst_total / M;
        if (per_seq <= per_bst) {
            printf("N/A (per-search cost not lower for BST)\n");
        } else {
            double k = build_cmp / (per_seq - per_bst);
            printf("%.1f searches\n", k);
        }
    }

    free_tree(root);
    return 0;
}