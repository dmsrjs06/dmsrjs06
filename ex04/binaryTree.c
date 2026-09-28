/*
 * Binary tree input in parenthesis notation + iterative preorder/inorder/postorder traversal
 *
 * Input format (parenthesis notation):
 *   Tree  := Data [ '(' [Tree] ',' [Tree] ')' ]
 *   e.g.) A(B(D,E),C(,F))   -> A has left child B, right child C;
 *                              C has no left child, right child F
 *   - Inside '(' ... ')' there must be a comma separating "left,right";
 *     a missing child is left empty.
 *   - Whitespace is ignored. Data is an alphanumeric string (max 31 chars).
 *
 * No recursive functions are used (parser, printing, traversals and memory
 * release all use loops + an explicit stack).
 * Compile: gcc -Wall -o bintree bintree.c
 */
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

/* ---------------- Stack (dynamic array) ---------------- */
typedef struct {
    Node **items;
    int *aux;          /* auxiliary info (parser: side, printer: depth) */
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
    if (s->top == s->cap) {
        s->cap *= 2;
        s->items = realloc(s->items, sizeof(Node *) * s->cap);
        s->aux = realloc(s->aux, sizeof(int) * s->cap);
        if (!s->items || !s->aux) { fprintf(stderr, "Out of memory\n"); exit(1); }
    }
    s->items[s->top] = n; s->aux[s->top] = aux; s->top++;
}
static Node *pop(Stack *s) { s->top--; return s->items[s->top]; }
static Node *peek(const Stack *s) { return s->items[s->top - 1]; }

/* ---------------- Node creation / release ---------------- */
static Node *new_node(const char *data) {
    Node *n = malloc(sizeof(Node));
    if (!n) { fprintf(stderr, "Out of memory\n"); exit(1); }
    strcpy(n->data, data);
    n->left = n->right = NULL;
    return n;
}

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

/* ---------------- Parenthesis-notation parser (iterative) ----------------
 * Stack holds parent nodes not yet closed.
 * aux = 0 (filling left child) / 1 (filling right child) /
 *      -1 (node just created, '(' not yet seen)
 * Returns the root on success; NULL with errmsg set on failure.            */
static Node *parse_tree(const char *str, char *errmsg, size_t errsz) {
    Stack st; stack_init(&st);
    Node *root = NULL;
    int can_data = 1;      /* may a node's data appear at this position? */
    int can_open = 0;      /* may '(' appear (a node was just created)?  */
    size_t i = 0, len = strlen(str);

#define FAIL(...) do { snprintf(errmsg, errsz, __VA_ARGS__); \
                       free_tree(root); stack_free(&st); return NULL; } while (0)

    while (i < len) {
        unsigned char c = (unsigned char)str[i];
        if (isspace(c)) { i++; continue; }

        if (isalnum(c)) {
            char buf[MAX_DATA]; int k = 0;
            size_t start = i;
            if (!can_data) FAIL("Position %zu: data '%c' must be preceded by a separator ('(' or ',').", start + 1, c);
            while (i < len && isalnum((unsigned char)str[i])) {
                if (k >= MAX_DATA - 1) FAIL("Position %zu: data is too long (max %d characters).", start + 1, MAX_DATA - 1);
                buf[k++] = str[i++];
            }
            buf[k] = '\0';
            Node *n = new_node(buf);
            if (stack_empty(&st)) {
                if (root) { free(n); FAIL("Position %zu: more than one root.", start + 1); }
                root = n;
            } else {
                Node *p = peek(&st);
                if (st.aux[st.top - 1] == 0) p->left = n; else p->right = n;
            }
            push(&st, n, -1);          /* mark: node just created, '(' unconfirmed */
            can_data = 0; can_open = 1;
            continue;
        }

        if (c == '(') {
            if (!can_open || stack_empty(&st) || st.aux[st.top - 1] != -1)
                FAIL("Position %zu: '(' may only follow a node's data.", i + 1);
            st.aux[st.top - 1] = 0;    /* confirmed as parent, start left child */
            can_data = 1; can_open = 0;
            i++; continue;
        }

        /* before ',' or ')', drop the "just created" marker */
        if (!stack_empty(&st) && st.aux[st.top - 1] == -1) { pop(&st); can_open = 0; }

        if (c == ',') {
            if (stack_empty(&st)) FAIL("Position %zu: ',' outside of parentheses.", i + 1);
            if (st.aux[st.top - 1] != 0) FAIL("Position %zu: a node has at most 2 children (extra comma).", i + 1);
            st.aux[st.top - 1] = 1;
            can_data = 1;
            i++; continue;
        }
        if (c == ')') {
            if (stack_empty(&st)) FAIL("Position %zu: unmatched ')'.", i + 1);
            if (st.aux[st.top - 1] != 1) FAIL("Position %zu: parentheses must contain a comma in the form 'left,right'.", i + 1);
            pop(&st);
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

/* ---------------- Tree structure printer (iterative, right child on top) ----
 * Uses reverse inorder (right -> root -> left) to print the tree rotated
 * 90 degrees counter-clockwise.                                              */
static void print_tree(Node *root) {
    Stack s; stack_init(&s);
    Node *cur = root; int depth = 0;
    while (cur || !stack_empty(&s)) {
        while (cur) { push(&s, cur, depth); cur = cur->right; depth++; }
        depth = s.aux[s.top - 1];
        cur = pop(&s);
        for (int i = 0; i < depth; i++) printf("    ");
        printf("%s\n", cur->data);
        cur = cur->left; depth++;
    }
    stack_free(&s);
}

/* Preorder: Root -> Left -> Right */
// Succcess
void preorder(Node *tree) {
    Stack s; stack_init(&s);
    if (tree) push(&s, tree, 0);
    while (!stack_empty(&s)) {
        Node *n = pop(&s);
        printf("%s ", n->data);                 /* visit */
        if (n->right) push(&s, n->right, 0);    /* push right first so left pops first */
        if (n->left)  push(&s, n->left, 0);
    }
    printf("\n");
    stack_free(&s);
}

/* Inorder: Left -> Root -> Right */
void inorder(Node *tree) {
    Stack s; stack_init(&s);
    Node *cur = tree;
    while (cur || !stack_empty(&s)) {
        while (cur) { push(&s, cur, 0); cur = cur->left; }   /* go as far left as possible */
        cur = pop(&s);
        printf("%s ", cur->data);                            /* visit */
        cur = cur->right;
    }
    printf("\n");
    stack_free(&s);
}

/* Postorder: Left -> Right -> Root (tracks the last visited node) */
void postorder(Node *tree) {
    Stack s; stack_init(&s);
    Node *cur = tree, *last = NULL;
    while (cur || !stack_empty(&s)) {
        while (cur) { push(&s, cur, 0); cur = cur->left; }
        Node *top = peek(&s);
        if (top->right && top->right != last) {
            cur = top->right;                   /* right subtree still pending */
        } else {
            printf("%s ", top->data);           /* visit */
            last = pop(&s);
        }
    }
    printf("\n");
    stack_free(&s);
}

/* ---------------- main ---------------- */
int main(void) {
    char line[MAX_LINE], err[256];

    printf("Enter a binary tree in parenthesis notation.\n");
    printf("e.g.) A(B(D,E),C(,F))\n> ");
    if (!fgets(line, sizeof(line), stdin)) {
        fprintf(stderr, "Error: no input.\n");
        return 1;
    }

    Node *root = parse_tree(line, err, sizeof(err));
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