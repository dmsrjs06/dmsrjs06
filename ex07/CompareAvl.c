#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 100      /* 생성할 난수 개수 (중복 포함) */
#define M 50       /* 탐색 대상 개수 */
#define MAXV 1000  /* 값의 범위는 [0, MAXV] */

/* ----- 이진 탐색 트리(BST) 노드 ----- */
typedef struct BNode {
    int data;
    struct BNode *left;
    struct BNode *right;
} BNode;

/* ----- AVL 트리 노드 (균형 유지를 위해 height 필드를 둔다) ----- */
typedef struct ANode {
    int data;
    struct ANode *left;
    struct ANode *right;
    int height; /* 이 노드를 루트로 하는 서브트리의 높이 (노드 수 기준, 리프 = 1) */
} ANode;

static BNode *bnew(int v) {
    BNode *n = malloc(sizeof(BNode));
    if (!n) { fprintf(stderr, "Out of memory\n"); exit(1); }
    n->data = v;
    n->left = n->right = NULL;
    return n;
}

static ANode *anew(int v) {
    ANode *n = malloc(sizeof(ANode));
    if (!n) { fprintf(stderr, "Out of memory\n"); exit(1); }
    n->data = v;
    n->left = n->right = NULL;
    n->height = 1; /* 새로 만든 노드는 그 자체로 높이 1인 트리 */
    return n;
}

static int aheight(ANode *n) { return n ? n->height : 0; }
static int amax(int a, int b) { return a > b ? a : b; }
static int abalance(ANode *n) { return n ? aheight(n->left) - aheight(n->right) : 0; }

/* 오른쪽 회전 (LL 상황 교정) */
static ANode *right_rotate(ANode *y) {
    ANode *x = y->left;
    ANode *t2 = x->right;
    x->right = y;
    y->left = t2;
    y->height = 1 + amax(aheight(y->left), aheight(y->right));
    x->height = 1 + amax(aheight(x->left), aheight(x->right));
    return x;
}

/* 왼쪽 회전 (RR 상황 교정) */
static ANode *left_rotate(ANode *x) {
    ANode *y = x->right;
    ANode *t2 = y->left;
    y->left = x;
    x->right = t2;
    x->height = 1 + amax(aheight(x->left), aheight(x->right));
    y->height = 1 + amax(aheight(y->left), aheight(y->right));
    return y;
}

/* BST 삽입 (재귀). v와 기존 노드 값을 비교할 때마다 *cmp를 1 증가시킨다.
 * 삽입에 성공하면 *inserted에 1, 이미 존재하는 값이라 삽입하지 않으면 0을
 * 저장한다. (갱신된) 서브트리의 루트를 반환한다. */
static BNode *bst_insert(BNode *node, int v, long *cmp, int *inserted) {
    if (!node) { *inserted = 1; return bnew(v); }
    (*cmp)++;
    if (v < node->data) {
        node->left = bst_insert(node->left, v, cmp, inserted);
    } else if (v > node->data) {
        node->right = bst_insert(node->right, v, cmp, inserted);
    } else {
        *inserted = 0; /* 이미 존재하는 값: 중복, 삽입하지 않음 */
    }
    return node;
}

/* AVL 삽입 (재귀). 값 비교 카운트 방식은 bst_insert와 동일하다.
 * 삽입 후 높이를 갱신하고 balance factor를 확인하여 필요하면 회전한다.
 * 회전 여부를 결정하는 데 쓰이는 비교는 *cmp에 포함하지 않는다. */
static ANode *avl_insert(ANode *node, int v, long *cmp, int *inserted) {
    if (!node) { *inserted = 1; return anew(v); }
    (*cmp)++;
    if (v < node->data) {
        node->left = avl_insert(node->left, v, cmp, inserted);
    } else if (v > node->data) {
        node->right = avl_insert(node->right, v, cmp, inserted);
    } else {
        *inserted = 0; /* 이미 존재하는 값: 중복, 삽입하지 않음 */
        return node;   /* 구조 변화가 없으므로 균형도 그대로 유지 */
    }

    node->height = 1 + amax(aheight(node->left), aheight(node->right));
    int balance = abalance(node);

    if (balance > 1 && abalance(node->left) >= 0)  return right_rotate(node);            /* LL */
    if (balance > 1 && abalance(node->left) < 0)  { node->left  = left_rotate(node->left);  return right_rotate(node); } /* LR */
    if (balance < -1 && abalance(node->right) <= 0) return left_rotate(node);             /* RR */
    if (balance < -1 && abalance(node->right) > 0) { node->right = right_rotate(node->right); return left_rotate(node); } /* RL */

    return node;
}

/* BST의 높이를 노드 수 기준으로 재귀 계산한다 (빈 트리 = 0, 리프 = 1). */
static int bst_height(BNode *node) {
    if (!node) return 0;
    int lh = bst_height(node->left);
    int rh = bst_height(node->right);
    return 1 + (lh > rh ? lh : rh);
}

/* BST에서 key를 탐색한다 (반복적). 노드를 방문할 때마다 *cmp를 1 증가시킨다. */
static int bst_search(BNode *root, int key, long *cmp) {
    BNode *cur = root;
    while (cur) {
        (*cmp)++;
        if (key == cur->data) return 1;
        cur = (key < cur->data) ? cur->left : cur->right;
    }
    return 0;
}

/* AVL 트리에서 key를 탐색한다. 탐색 방법 자체는 BST 탐색과 동일하다. */
static int avl_search(ANode *root, int key, long *cmp) {
    ANode *cur = root;
    while (cur) {
        (*cmp)++;
        if (key == cur->data) return 1;
        cur = (key < cur->data) ? cur->left : cur->right;
    }
    return 0;
}

/* 배열에서 key를 순차 탐색한다. 원소와 비교할 때마다 *cmp를 1 증가시킨다. */
static int sequential_search(const int *arr, int n, int key, long *cmp) {
    for (int i = 0; i < n; i++) {
        (*cmp)++;
        if (arr[i] == key) return 1;
    }
    return 0;
}

/* 배열에 값을 "중복이 아닐 때만" 추가한다. 중복 확인을 위한 순차 탐색
 * 비교 횟수는 *cmp에 누적한다. 삽입되면 1, 중복이면 0을 반환한다. */
static int array_insert_unique(int *arr, int *len, int v, long *cmp) {
    for (int i = 0; i < *len; i++) {
        (*cmp)++;
        if (arr[i] == v) return 0; /* 이미 존재: 중복 */
    }
    arr[(*len)++] = v;
    return 1;
}

/* 배열 기반 스택을 이용해 BST의 모든 노드를 반복적으로 해제한다. */
static void free_bst(BNode *root) {
    if (!root) return;
    BNode *stack[N + 1];
    int top = 0;
    stack[top++] = root;
    while (top > 0) {
        BNode *n = stack[--top];
        if (n->left)  stack[top++] = n->left;
        if (n->right) stack[top++] = n->right;
        free(n);
    }
}

/* 배열 기반 스택을 이용해 AVL 트리의 모든 노드를 반복적으로 해제한다. */
static void free_avl(ANode *root) {
    if (!root) return;
    ANode *stack[N + 1];
    int top = 0;
    stack[top++] = root;
    while (top > 0) {
        ANode *n = stack[--top];
        if (n->left)  stack[top++] = n->left;
        if (n->right) stack[top++] = n->right;
        free(n);
    }
}

int main(void) {
    srand((unsigned)time(NULL));

    /* 1. 0~1000 사이의 정수 100개를 생성한다 (중복 발생 가능, 발생 순서 유지) */
    int raw[N];
    for (int i = 0; i < N; i++) raw[i] = rand() % (MAXV + 1);

    printf("=== 1. Generated %d integers (duplicates allowed, insertion order) ===\n", N);
    for (int i = 0; i < N; i++) {
        printf("%4d%s", raw[i], (i + 1) % 10 == 0 ? "\n" : " ");
    }
    if (N % 10 != 0) printf("\n");

    /* 2. 같은 순서로 배열 / BST / AVL 트리에 삽입을 시도한다 */
    int arr[N];
    int arr_len = 0;
    long arr_build_cmp = 0;

    BNode *broot = NULL;
    long bst_build_cmp = 0;

    ANode *aroot = NULL;
    long avl_build_cmp = 0;

    int stored_count = 0, dup_count = 0;

    for (int i = 0; i < N; i++) {
        int v = raw[i];
        int ins_a = array_insert_unique(arr, &arr_len, v, &arr_build_cmp);

        int ins_b = 0;
        broot = bst_insert(broot, v, &bst_build_cmp, &ins_b);

        int ins_c = 0;
        aroot = avl_insert(aroot, v, &avl_build_cmp, &ins_c);

        if (ins_a) stored_count++; else dup_count++;

        /* 세 자료구조는 항상 같은 집합을 유지해야 하므로 삽입 결과가
         * 서로 어긋나면 안 된다. 어긋나면 구현 오류이므로 경고를 출력한다. */
        if (ins_a != ins_b || ins_a != ins_c) {
            fprintf(stderr, "Warning: insertion result mismatch for value %d (array=%d, bst=%d, avl=%d)\n",
                    v, ins_a, ins_b, ins_c);
        }
    }

    printf("\n=== 2. Construction ===\n");
    printf("Distinct values stored      : %d\n", stored_count);
    printf("Duplicate values skipped    : %d\n", dup_count);
    printf("Array  construction comparisons : %ld\n", arr_build_cmp);
    printf("BST    construction comparisons : %ld\n", bst_build_cmp);
    printf("AVL    construction comparisons : %ld\n", avl_build_cmp);

    int bst_h = bst_height(broot);
    int avl_h = aheight(aroot);

    printf("\n=== 3. Structure ===\n");
    printf("Array length : %d\n", arr_len);
    printf("BST height   : %d\n", bst_h);
    printf("AVL height   : %d\n", avl_h);

    /* 4. 탐색 대상 50개 생성 */
    int keys[M];
    for (int i = 0; i < M; i++) keys[i] = rand() % (MAXV + 1);

    printf("\n=== 4. Search keys (%d) ===\n", M);
    for (int i = 0; i < M; i++) {
        printf("%4d%s", keys[i], (i + 1) % 10 == 0 ? "\n" : " ");
    }
    if (M % 10 != 0) printf("\n");

    /* 5. 세 가지 방법으로 각각 탐색하고 비교 횟수를 측정 */
    printf("\n=== 5. Per-key search results ===\n");
    printf("%-8s %-8s %-10s %-10s %-10s\n", "Key", "Result", "Seq.Cmp", "BST.Cmp", "AVL.Cmp");

    long seq_total = 0, bst_total = 0, avl_total = 0;
    int seq_found = 0, bst_found = 0, avl_found = 0;

    for (int i = 0; i < M; i++) {
        long seq_cmp = 0, bst_cmp = 0, avl_cmp = 0;
        int f_seq = sequential_search(arr, arr_len, keys[i], &seq_cmp);
        int f_bst = bst_search(broot, keys[i], &bst_cmp);
        int f_avl = avl_search(aroot, keys[i], &avl_cmp);

        seq_total += seq_cmp; bst_total += bst_cmp; avl_total += avl_cmp;
        if (f_seq) seq_found++;
        if (f_bst) bst_found++;
        if (f_avl) avl_found++;

        printf("%-8d %-8s %-10ld %-10ld %-10ld\n",
               keys[i], f_seq ? "Found" : "NotFnd", seq_cmp, bst_cmp, avl_cmp);

        /* 세 탐색은 같은 데이터 집합을 대상으로 하므로 성공/실패 여부가
         * 항상 일치해야 한다. */
        if (f_seq != f_bst || f_seq != f_avl) {
            fprintf(stderr, "Warning: search result mismatch for key %d (seq=%d, bst=%d, avl=%d)\n",
                    keys[i], f_seq, f_bst, f_avl);
        }
    }

    printf("\n=== 6. Summary ===\n");
    printf("Number of searches : %d\n", M);
    printf("Sequential - found : %d, not found: %d\n", seq_found, M - seq_found);
    printf("BST        - found : %d, not found: %d\n", bst_found, M - bst_found);
    printf("AVL        - found : %d, not found: %d\n", avl_found, M - avl_found);

    printf("\nSequential Search\n");
    printf("  Total comparisons   : %ld\n", seq_total);
    printf("  Average comparisons : %.2f\n", (double)seq_total / M);
    printf("\nBST Search\n");
    printf("  Total comparisons   : %ld\n", bst_total);
    printf("  Average comparisons : %.2f\n", (double)bst_total / M);
    printf("\nAVL Search\n");
    printf("  Total comparisons   : %ld\n", avl_total);
    printf("  Average comparisons : %.2f\n", (double)avl_total / M);

    printf("\n=== 7. Combined cost (construction + %d searches) ===\n", M);
    printf("Array (construction + sequential search) : %ld\n", arr_build_cmp + seq_total);
    printf("BST   (construction + BST search)         : %ld\n", bst_build_cmp + bst_total);
    printf("AVL   (construction + AVL search)         : %ld\n", avl_build_cmp + avl_total);

    free_bst(broot);
    free_avl(aroot);
    return 0;
}