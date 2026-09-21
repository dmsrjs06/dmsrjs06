#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define ARR_MAX 2048 // 배열 표현 최대 크기

// 연결 자료구조
typedef struct Node {
    char data;
    struct Node *left, *right;
} Node;

char arr[ARR_MAX]; // 1번 인덱스 - 루트, 빈곳 0
int  usedMaxIndex = 0; // 배열에서 사용된 최대 인덱스값
int  parseError = 0;

const char *p;

// 연걸노드생성 + 위치에 저장
Node *parseTree(int idx) {
    if (parseError) return NULL;
    if (!isupper((unsigned char)*p)) { parseError = 1; return NULL; }

    Node *node = (Node *)malloc(sizeof(Node));
    node->data = *p;
    node->left = node->right = NULL;

    if (idx >= ARR_MAX) { parseError = 1; return node; }
    arr[idx] = *p;
    if (idx > usedMaxIndex) usedMaxIndex = idx;
    p++;

    if (*p == '(') {
        p++;
        if (*p != ',' && *p != ')') // 왼쪽
            node->left = parseTree(idx * 2);
        if (*p == ',') {
            p++;
            if (*p != ')') // 오른쪽 
                node->right = parseTree(idx * 2 + 1);
        }
        if (*p != ')') { parseError = 1; return node; }
        p++;
    }
    return node;
}

// 2 .배열기반 기능
void arrPrint(int idx, int depth, int isRoot, int pendingArr[]) {
    if (idx > usedMaxIndex || arr[idx] == 0) return;

    if (isRoot) printf("%c\n", arr[idx]);
    else {
        for (int d = 1; d < depth; d++)
            printf(pendingArr[d] ? "|   " : "    ");
        printf("+---%c\n", arr[idx]);
    }
    int L = idx * 2, R = idx * 2 + 1;
    int hasL = (L <= usedMaxIndex && arr[L]);
    int hasR = (R <= usedMaxIndex && arr[R]);
    //왼쪽 출력시 오른쪽에 자식이 남아있으면 | 출력
    if (hasL) { pendingArr[depth + 1] = hasR; arrPrint(L, depth + 1, 0, pendingArr); }
    if (hasR) { pendingArr[depth + 1] = 0;    arrPrint(R, depth + 1, 0, pendingArr); }
}

// 트리 정보 배열
int arrCount(int idx) {
    if (idx > usedMaxIndex || arr[idx] == 0) return 0;
    return 1 + arrCount(idx * 2) + arrCount(idx * 2 + 1);
}
int arrChildNum(int idx) {
    int c = 0;
    if (idx * 2     <= usedMaxIndex && arr[idx * 2]) {
        c++;
    }
    if (idx * 2 + 1 <= usedMaxIndex && arr[idx * 2 + 1]) {
        c++;
    }
    return c;
}
int arrLeafCount(int idx) {
    if (idx > usedMaxIndex || arr[idx] == 0) return 0;
    if (arrChildNum(idx) == 0) return 1;
    return arrLeafCount(idx * 2) + arrLeafCount(idx * 2 + 1);
}
// 간선 기준 높이
int arrHeight(int idx) {
    if (idx > usedMaxIndex || arr[idx] == 0) return -1;
    int hl = arrHeight(idx * 2), hr = arrHeight(idx * 2 + 1);
    return 1 + (hl > hr ? hl : hr);
}
int arrDegree(int idx) {
    if (idx > usedMaxIndex || arr[idx] == 0) return 0;
    int d = arrChildNum(idx);
    int dl = arrDegree(idx * 2), dr = arrDegree(idx * 2 + 1);
    if (dl > d) d = dl;
    if (dr > d) d = dr;
    return d;
}

// 형태 배열 
int arrIsComplete(void) {
    // 완전 이진트리
    int n = arrCount(1);
    for (int i = 1; i <= n; i++)
        if (arr[i] == 0) return 0;
    return 1;
}
int arrIsFull(void) {
// 포화 이진트리 , 노드수 = 2^^(h+1) -1
    int n = arrCount(1), h = arrHeight(1);
    return arrIsComplete() && (n == (1 << (h + 1)) - 1);
}
int arrIsSkewed(void) {
    // 편향 = 모든 노두의 자식이 1개 이하
    return arrHeight(1) == arrCount(1) - 1;
}

// 특정노드의 부모 , 자식, 형제 배열
void arrRelatives(char target) {
    int idx = -1;
    for (int i = 1; i <= usedMaxIndex; i++)
        if (arr[i] == target) { idx = i; break; }
    if (idx < 0) { printf("  Node %c is not in the tree.\n", target); return; }

    printf("  [Array] Node %c (Index %d)\n", target, idx);
    if (idx == 1) printf("    Parent: None (Root)\n");
    else          printf("    Parent: %c (Index %d)\n", arr[idx / 2], idx / 2);

    int L = idx * 2, R = idx * 2 + 1;
    printf("    Child(ren): ");
    int any = 0;
    if (L <= usedMaxIndex && arr[L]) { printf("%c ", arr[L]); any = 1; }
    if (R <= usedMaxIndex && arr[R]) { printf("%c ", arr[R]); any = 1; }
    printf(any ? "\n" : "None\n");

    if (idx == 1) printf("    Sibling: None (Root)\n");
    else {
        int sib = (idx % 2 == 0) ? idx + 1 : idx - 1;
        if (sib <= usedMaxIndex && arr[sib]) printf("    Sibling: %c\n", arr[sib]);
        else printf("    Sibling: None\n");
    }
}

// 연결 자료구조 기반 배열

// 1. 눕힌 형태 출력
void linkPrint(Node *t, int depth, int isRoot, int pendingArr[]) {
    if (!t) return;
    if (isRoot) printf("%c\n", t->data);
    else {
        for (int d = 1; d < depth; d++)
            printf(pendingArr[d] ? "|   " : "    ");
        printf("+---%c\n", t->data);
    }
    if (t->left)  { pendingArr[depth + 1] = (t->right != NULL); linkPrint(t->left,  depth + 1, 0, pendingArr); }
    if (t->right) { pendingArr[depth + 1] = 0;                  linkPrint(t->right, depth + 1, 0, pendingArr); }
}

// 2. 트리 정보
int linkCount(Node *t)  { return t ? 1 + linkCount(t->left) + linkCount(t->right) : 0; }
int linkLeaf(Node *t) {
    if (!t) return 0;
    if (!t->left && !t->right) return 1;
    return linkLeaf(t->left) + linkLeaf(t->right);
}
int linkHeight(Node *t) {
    if (!t) return -1;
    int hl = linkHeight(t->left), hr = linkHeight(t->right);
    return 1 + (hl > hr ? hl : hr);
}
int linkDegree(Node *t) {
    if (!t) return 0;
    int d = (t->left != NULL) + (t->right != NULL);
    int dl = linkDegree(t->left), dr = linkDegree(t->right);
    if (dl > d) d = dl;
    if (dr > d) d = dr;
    return d;
}

// 3. 형태 판별
int linkCompleteChk(Node *t, int idx, int n) {
// 각 노드에 번호를 부여하고 그 수가 n을 넘으면 완전 트리가 아님.
    if (!t) return 1;
    if (idx > n) return 0;
    return linkCompleteChk(t->left, idx * 2, n) &&
           linkCompleteChk(t->right, idx * 2 + 1, n);
}
int linkIsComplete(Node *t) { return linkCompleteChk(t, 1, linkCount(t)); }
int linkIsFull(Node *t) {
    int n = linkCount(t), h = linkHeight(t);
    return n == (1 << (h + 1)) - 1 && linkIsComplete(t);
}
int linkIsSkewed(Node *t) { return linkHeight(t) == linkCount(t) - 1; }

// 3. 특정 노드의 부모, 자식, 형제 구조 파악 (연결일때)
Node *findNode(Node *t, char c) {
    if (!t) return NULL;
    if (t->data == c) return t;
    Node *f = findNode(t->left, c);
    return f ? f : findNode(t->right, c);
}
Node *findParent(Node *t, char c) {
    if (!t) return NULL;
    if ((t->left && t->left->data == c) || (t->right && t->right->data == c))
        return t;
    Node *f = findParent(t->left, c);
    return f ? f : findParent(t->right, c);
}
void linkRelatives(Node *root, char target) {
    Node *nd = findNode(root, target);
    if (!nd) { printf("  Node %c is not in the tree.\n", target); return; }

    printf("  [Linked] Node %c\n", target);
    Node *par = findParent(root, target);
    if (par) printf("    Parent: %c\n", par->data);
    else     printf("    Parent: None (Root)\n");

    printf("    Child(ren): ");
    if (nd->left || nd->right) {
        if (nd->left)  printf("%c ", nd->left->data);
        if (nd->right) printf("%c ", nd->right->data);
        printf("\n");
    } else printf("None\n");

    if (par) {
        Node *sib = (par->left == nd) ? par->right : par->left;
        if (sib) printf("    Sibling: %c\n", sib->data);
        else     printf("    Sibling: None\n");
    } else printf("    Sibling: None (Root)\n");
}

void freeTree(Node *t) {
    if (!t) return;
    freeTree(t->left); freeTree(t->right); free(t);
}

// 메인함수
int main(void) {
    char s[1024];
    printf("Enter a binary tree in parenthesis notation: ");
    if (scanf("%1023s", s) != 1) return 0;

    memset(arr, 0, sizeof(arr));
    p = s;
    Node *root = parseTree(1);
    if (parseError || *p != '\0') {
        printf("Error: Invalid binary tree parenthesis notation.\n");
        return 1;
    }

    int pend[64] = {0};

// 1. 배열 출력
    printf("\n========== 1. Array-based Binary Tree ==========\n");
    printf("\n[1] Print Binary Tree\n");
    arrPrint(1, 0, 1, pend);

    int aTotal = arrCount(1), aLeaf = arrLeafCount(1);
    printf("\n[2] Tree Information\n");
    printf("  Total nodes      : %d\n", aTotal);
    printf("  Leaf nodes       : %d\n", aLeaf);
    printf("  Non-leaf nodes   : %d\n", aTotal - aLeaf);
    printf("  Tree height      : %d\n", arrHeight(1));
    printf("  Tree degree      : %d\n", arrDegree(1));

    printf("\n[3] Determine Tree Shape\n");
    printf("  Complete binary tree : %s\n", arrIsComplete() ? "Yes" : "No");
    printf("  Full binary tree     : %s\n", arrIsFull()     ? "Yes" : "No");
    printf("  Skewed binary tree   : %s\n", arrIsSkewed()   ? "Yes" : "No");

// 2. 연결자료구조 출력
    printf("\n========== 2. Linked List-based Binary Tree ==========\n");
    printf("\n[1] Print Binary Tree\n");
    memset(pend, 0, sizeof(pend));
    linkPrint(root, 0, 1, pend);

    int lTotal = linkCount(root), lLeaf = linkLeaf(root);
    printf("\n[2] Tree Information\n");
    printf("  Total nodes      : %d\n", lTotal);
    printf("  Leaf nodes       : %d\n", lLeaf);
    printf("  Non-leaf nodes   : %d\n", lTotal - lLeaf);
    printf("  Tree height      : %d\n", linkHeight(root));
    printf("  Tree degree      : %d\n", linkDegree(root));

    printf("\n[3] Determine Tree Shape\n");
    printf("  Complete binary tree : %s\n", linkIsComplete(root) ? "Yes" : "No");
    printf("  Full binary tree     : %s\n", linkIsFull(root)     ? "Yes" : "No");
    printf("  Skewed binary tree   : %s\n", linkIsSkewed(root)   ? "Yes" : "No");

// 3. 두 구현의 비교
    printf("\n========== 3. Comparison of Array and Linked Implementations ==========\n");
    printf("\n[1] Memory Usage\n");
    printf("  Array implementation  : Max index used %d -> %d bytes (%d slots x %zu bytes)\n",
           usedMaxIndex, (int)((usedMaxIndex + 1) * sizeof(char)),
           usedMaxIndex + 1, sizeof(char));
    printf("  Linked implementation : %d nodes x %zu bytes = %d bytes\n",
           lTotal, sizeof(Node), (int)(lTotal * sizeof(Node)));

    printf("\n[2] Print Parent/Child/Sibling of a Specific Node\n");
    char target;
    printf("  Enter node to query: ");
    scanf(" %c", &target);
    arrRelatives(target);
    linkRelatives(root, target);

    freeTree(root);
    return 0;
}