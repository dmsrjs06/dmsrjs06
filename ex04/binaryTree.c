#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_DATA 32
#define MAX_LINE 4096

typedef struct Node {
    char data[MAX_DATA];
    struct Node *left;
    struct Node *right;
} Node;

/* 스택 (동적 배열), aux: 파서에서는 방향 표시, 출력에서는 깊이 */
typedef struct {
    Node **items;
    int *aux;
    int top, cap;
} Stack;

static void stack_init(Stack *s) {
    s->cap = 64; s->top = 0;
    s->items = malloc(sizeof(Node *) * s->cap);
    s->aux = malloc(sizeof(int) * s->cap);
    if (!s->items || !s->aux) { fprintf(stderr, "Out of memory\n"); exit(1); }
}

static void stack_free(Stack *s) { free(s->items); free(s->aux); }
static int  stack_empty(const Stack *s) { return s->top == 0; }
static void push(Stack *s, Node *n, int aux) {
    if (s->top == s->cap) { /* 가득 차면 용량 2배 확장 */
        s->cap *= 2;
        s->items = realloc(s->items, sizeof(Node *) * s->cap);
        s->aux = realloc(s->aux, sizeof(int) * s->cap);
        if (!s->items || !s->aux) { fprintf(stderr, "Out of memory\n"); exit(1); }
    }
    s->items[s->top] = n; s->aux[s->top] = aux; s->top++;
}

static Node *pop(Stack *s) { s->top--; return s->items[s->top]; }
static Node *peek(const Stack *s) { return s->items[s->top - 1]; }
static Node *new_node(const char *data) {
    Node *n = malloc(sizeof(Node));
    if (!n) { fprintf(stderr, "Out of memory\n"); exit(1); }
    strcpy(n->data, data);
    n->left = n->right = NULL;
    return n;
}

/* 스택을 이용해 모든 노드를 반복적으로 해제 */
static void free_tree(Node *root) {
    Stack s; stack_init(&s);
    if (root) push(&s, root, 0);
    while (!stack_empty(&s)) {
        Node *n = pop(&s);
        if (n->left)  push(&s, n->left, 0);
        if (n->right) push(&s, n->right, 0);
        free(n);
    }
    stack_free(&s);
}
/* 괄호 표기법 파서 (반복적). 스택에는 아직 안 닫힌 부모 노드를 보관 
    aux: 0=왼쪽 자식 작성 중, 1=오른쪽 자식 작성 중, -1=방금 만든 노드('(' 미확인)
    성공 시 루트 반환, 실패 시 errmsg를 채우고 NULL 반환 */
static Node *parse_tree(const char *str, char *errmsg, size_t errsz) {
    Stack st; stack_init(&st);
    Node *root = NULL;
    int can_data = 1; /* 현재 위치에 노드 데이터가 올 수 있는가 */
    int can_open = 0; /* '(' 가 올 수 있는가 (노드 생성 직후) */
    size_t i = 0, len = strlen(str);
/* 오류 시 메시지 저장 + 자원 정리 후 NULL 반환 */
#define FAIL(...) do { snprintf(errmsg, errsz, __VA_ARGS__); \
                       free_tree(root); stack_free(&st); return NULL; } while (0)
    while (i < len) {
        unsigned char c = (unsigned char)str[i];
        if (isspace(c)) { i++; continue; } /* 공백 무시 */
        if (isalnum(c)) { /* 노드 데이터 */
            char buf[MAX_DATA]; int k = 0;
            size_t start = i;
            if (!can_data) FAIL("Position %zu: data '%c' must be preceded by a separator ('(' or ',').", start + 1, c);
            while (i < len && isalnum((unsigned char)str[i])) {
                if (k >= MAX_DATA - 1) FAIL("Position %zu: data is too long (max %d characters).", start + 1, MAX_DATA - 1);
                buf[k++] = str[i++];
            }
            buf[k] = '\0';
            Node *n = new_node(buf);
            if (stack_empty(&st)) { /* 스택이 비었으면 루트 */
                if (root) { free(n); FAIL("Position %zu: more than one root.", start + 1); }
                root = n;
            } else { /* 현재 부모의 왼쪽/오른쪽 슬롯에 연결 */
                Node *p = peek(&st);
                if (st.aux[st.top - 1] == 0) p->left = n; else p->right = n;
            }
            push(&st, n, -1); /* 표식: 방금 만든 노드 */
            can_data = 0; can_open = 1;
            continue;
        }
        if (c == '(') {
            if (!can_open || stack_empty(&st) || st.aux[st.top - 1] != -1)
                FAIL("Position %zu: '(' may only follow a node's data.", i + 1);
            st.aux[st.top - 1] = 0; /* 부모로 확정, 왼쪽 자식 작성 시작 */
            can_data = 1; can_open = 0;
            i++; continue;
        }
        /* ',' 또는 ')' 처리 전에 "방금 만든 노드" 표식 제거 (리프 노드) */
        if (!stack_empty(&st) && st.aux[st.top - 1] == -1) { pop(&st); can_open = 0; }
        if (c == ',') {
            if (stack_empty(&st)) FAIL("Position %zu: ',' outside of parentheses.", i + 1);
            if (st.aux[st.top - 1] != 0) FAIL("Position %zu: a node has at most 2 children (extra comma).", i + 1);
            st.aux[st.top - 1] = 1; /* 이제 오른쪽 자식 작성 */
            can_data = 1;
            i++; continue;
        }
        if (c == ')') {
            if (stack_empty(&st)) FAIL("Position %zu: unmatched ')'.", i + 1);
            if (st.aux[st.top - 1] != 1) FAIL("Position %zu: parentheses must contain a comma in the form 'left,right'.", i + 1);
            pop(&st); /* 부모 노드 완성 */
            can_data = 0; can_open = 0;
            i++; continue;
        }
        FAIL("Position %zu: invalid character '%c'.", i + 1, c);
    }
    if (!stack_empty(&st) && st.aux[st.top - 1] == -1) pop(&st);
    if (!stack_empty(&st)) FAIL("There is an unclosed '('.");
    if (!root) FAIL("Empty input. There is no tree.");
    stack_free(&st);
#undef FAIL
    return root;
}

/* 트리 구조 출력 (반복적): 역중위(오른쪽->루트->왼쪽)로 90도 눕힌 모양, aux에 깊이 저장 */
static void print_tree(Node *root) {
    Stack s; stack_init(&s);
    Node *cur = root; int depth = 0;
    while (cur || !stack_empty(&s)) {
        while (cur) { push(&s, cur, depth); cur = cur->right; depth++; }
        depth = s.aux[s.top - 1];
        cur = pop(&s);
        for (int i = 0; i < depth; i++) printf("    "); /* 깊이만큼 들여쓰기 */
        printf("%s\n", cur->data);
        cur = cur->left; depth++;
    }
    stack_free(&s);
}

/* 전위 순회: Root -> Left -> Right */
void preorder(Node *tree) {
    Stack s; stack_init(&s);
    if (tree) push(&s, tree, 0);
    while (!stack_empty(&s)) {
        Node *n = pop(&s);
        printf("%s ", n->data); /* 방문 */
        if (n->right) push(&s, n->right, 0); /* 오른쪽을 먼저 넣어야 왼쪽이 먼저 나옴 */
        if (n->left)  push(&s, n->left, 0);
    }
    printf("\n");
    stack_free(&s);
}
/* 중위 순회: Left -> Root -> Right */
void inorder(Node *tree) {
    Stack s; stack_init(&s);
    Node *cur = tree;
    while (cur || !stack_empty(&s)) {
        while (cur) { push(&s, cur, 0); cur = cur->left; }   /* 가능한 만큼 왼쪽으로 */
        cur = pop(&s);
        printf("%s ", cur->data); /* 방문 */
        cur = cur->right; /* 오른쪽 서브트리로 이동 */
    }
    printf("\n");
    stack_free(&s);
}
/* 후위 순회: Left -> Right -> Root (마지막으로 방문한 노드를 추적) */
void postorder(Node *tree) {
    Stack s; stack_init(&s);
    Node *cur = tree, *last = NULL;
    while (cur || !stack_empty(&s)) {
        while (cur) { push(&s, cur, 0); cur = cur->left; }
        Node *top = peek(&s);
        if (top->right && top->right != last) {
            cur = top->right; /* 오른쪽 서브트리가 아직 남음 */
        } else {
            printf("%s ", top->data);   /* 방문 */
            last = pop(&s);
        }
    }
    printf("\n");
    stack_free(&s);
}
int main(void) {
    char line[MAX_LINE], err[256];
    printf("Enter a binary tree in parenthesis notation.\n");
    printf("e.g.) A(B(D,E),C(,F))\n> ");
    if (!fgets(line, sizeof(line), stdin)) {
        fprintf(stderr, "Error: no input.\n");
        return 1;
    }
    Node *root = parse_tree(line, err, sizeof(err)); /* 괄호 표현 -> 연결 자료구조 */
    if (!root) {
        printf("Error: invalid parenthesis expression. %s\n", err);
        return 1;
    }
    line[strcspn(line, "\r\n")] = '\0';
    printf("\n[Input tree] %s\n", line);
    printf("\n[Tree structure] (right child on top, left child below)\n");
    print_tree(root);
    printf("\nPreorder  : "); preorder(root);
    printf("Inorder   : ");   inorder(root);
    printf("Postorder : ");   postorder(root);
    free_tree(root);
    return 0;
}