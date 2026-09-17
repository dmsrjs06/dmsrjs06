#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LEN 1000

typedef struct {
    char items[MAX_LEN];
    int top;
} CharStack;

void initStack(CharStack *s) {
    s->top = -1;
}

int isEmpty(CharStack *s) {
    return s->top == -1;
}

void push(CharStack *s, char c) {
    if (s->top < MAX_LEN - 1) {
        s->items[++(s->top)] = c;
    }
}

char pop(CharStack *s) {
    if (isEmpty(s)) return '\0';
    return s->items[(s->top)--];
}

char peek(CharStack *s) {
    if (isEmpty(s)) return '\0';
    return s->items[s->top];
}


int isValidTree(const char *str) {
    int len = strlen(str);
    if (len == 0) return 0;

    CharStack s;
    initStack(&s);

    int expect_node = 1;

    for (int i = 0; i < len; i++) {
        char c = str[i];
        if (c >= 'A' && c <= 'Z') {
            if (!expect_node) return 0;
            if (i + 1 < len) {
                char next = str[i + 1];
                if (next != '(' && next != ',' && next != ')') return 0;
            }
            expect_node = 0;
        } else if (c == '(') {
            push(&s, '(');
            expect_node = 1;
        } else if (c == ')') {
            if (isEmpty(&s)) return 0;
            pop(&s);
            expect_node = 0;
        } else if (c == ',') {
            expect_node = 1;
        } else {
            return 0; // 허용되지않은문자 설정
        }
    }
    return isEmpty(&s);
}

// 특정 노드위치에서 다음에 형제 노드가 잇는지 검색
int hasNextSibling(const char *str, int start_idx) {
    int len = strlen(str);
    int nesting = 0;
    for (int i = start_idx + 1; i < len; i++) {
        char c = str[i];
        if (c == '(') {
            nesting++;
        } else if (c == ')') {
            nesting--;
            if (nesting < 0) break;
        } else if (c == ',' && nesting == 0) {
            return 1; // 같은 레벨에 형제 존재
        }
    }
    return 0;
}

int main() {
    char expr[MAX_LEN];
    printf("Enter tree parenthesis expression: ");
    if (scanf("%s", expr) != 1) {
        printf("Input Error\n");
        return 1;
    }

    if (!isValidTree(expr)) {
        printf("Error: Not a valid tree parenthesis notation.\n");
        return 0;
    }

    int len = strlen(expr);

    // 통계 정보 
    int total_nodes = 0;
    int leaf_nodes = 0;
    int non_leaf_nodes = 0;
    int max_height = 0;
    int current_depth = 0;
    int max_degree = 0;

    char c_parent = '\0';
    char c_children[MAX_LEN];
    int c_children_count = 0;
    int c_exists = 0;

    // 전체노드, 단말,비단말 검사
    for (int i = 0; i < len; i++) {
        char c = expr[i];
        if (c >= 'A' && c <= 'Z') {
            total_nodes++;
            int is_non_leaf = (i + 1 < len && expr[i + 1] == '(');
            if (is_non_leaf) {
                non_leaf_nodes++;
            } else {
                leaf_nodes++;
            }
        } else if (c == '(') {
            current_depth++;
            if (current_depth > max_height) {
                max_height = current_depth;
            }
        } else if (c == ')') {
            current_depth--;
        }
    }

    // 부모, 자식 차수 계산
    int node_degrees[26] = {0};
    char parent_map[26] = {0};

    {
        CharStack path_stack;
        initStack(&path_stack);
        
        for (int i = 0; i < len; i++) {
            char c = expr[i];
            if (c >= 'A' && c <= 'Z') {
                if (!isEmpty(&path_stack)) {
                    char parent = peek(&path_stack);
                    parent_map[c - 'A'] = parent;
                    node_degrees[parent - 'A']++;
                } else {
                    parent_map[c - 'A'] = '\0'; // 루트 노드
                }
                
                if (i + 1 < len && expr[i + 1] == '(') {
                    push(&path_stack, c);
                }
            } else if (c == ')') {
                if (!isEmpty(&path_stack)) {
                    pop(&path_stack);
                }
            }
        }
    }

    // 최대 차수 계산
    for (int i = 0; i < 26; i++) {
        if (node_degrees[i] > max_degree) {
            max_degree = node_degrees[i];
        }
    }

    // 노드 c의 부모찾기
    c_parent = parent_map['C' - 'A'];

    // 노드 c의 자식 찾기
    for (int i = 0; i < len; i++) {
        if (expr[i] == 'C') {
            c_exists = 1;
            if (i + 1 < len && expr[i + 1] == '(') {
                int paren_count = 0;
                int j = i + 1;
                while (j < len) {
                    if (expr[j] == '(') paren_count++;
                    else if (expr[j] == ')') paren_count--;
                    
                    if (paren_count == 1 && expr[j] >= 'A' && expr[j] <= 'Z') {
                        c_children[c_children_count++] = expr[j];
                    }
                    if (paren_count == 0) break;
                    j++;
                }
            }
            break;
        }
    }
    c_children[c_children_count] = '\0';

    printf("\n=== Tree Information ===\n");
    printf("Total nodes: %d\n", total_nodes);
    printf("Leaf nodes: %d\n", leaf_nodes);
    printf("Non-leaf nodes: %d\n", non_leaf_nodes);
    printf("Tree height: %d\n", max_height);
    printf("Tree degree: %d\n", max_degree);
    
    if (c_exists) {
        if (c_parent != '\0') {
            printf("Parent of node C: %c\n", c_parent);
        } else {
            printf("Parent of node C: None (Root node)\n");
        }
        
        printf("Children of node C: ");
        if (c_children_count > 0) {
            for (int i = 0; i < c_children_count; i++) {
                printf("%c ", c_children[i]);
            }
            printf("\n");
        } else {
            printf("None (Leaf node)\n");
        }
    } else {
        printf("Node C does not exist in the tree.\n");
    }

    // 계층적 트리 구조출력
    printf("\n=== Hierarchical Tree Structure ===\n");
    {
        int depth = 0;
        int ancestor_has_sibling[MAX_LEN] = {0};
        
        for (int i = 0; i < len; i++) {
            char c = expr[i];
            if (c >= 'A' && c <= 'Z') {
                int has_sibling = hasNextSibling(expr, i);
                
                if (i == 0) {
                    printf("%c\n", c);
                } else {
                    for (int d = 1; d < depth; d++) {
                        if (ancestor_has_sibling[d]) {
                            printf("|   ");
                        } else {
                            printf("    ");
                        }
                    }
                    printf("+---%c\n", c);
                }
                ancestor_has_sibling[depth] = has_sibling;
            } else if (c == '(') {
                depth++;
            } else if (c == ')') {
                depth--;
            }
        }
    }

    return 0;
}