#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LEN   256
#define MAX_PATH_LEN   256
#define MAX_TOKENS     10

typedef struct Node {
    char data;
    struct Node *left;
    struct Node *right;
    struct Node *parent;
} Node;

typedef struct {
    Node *root;
    int   size; // 최대 노드 개수
    int   count; // 현재 노드 개수
} Tree;

// 연산 결과
typedef enum {
    OK = 0,
    ERR_TREE_EMPTY,
    ERR_TREE_NOT_EMPTY,
    ERR_TREE_FULL,
    ERR_PATH_NOT_FOUND,
    ERR_INVALID_CHILD,
    ERR_POSITION_OCCUPIED,
    ERR_DUPLICATE_VALUE,
    ERR_NOT_LEAF,
    ERR_INVALID_DATA,
    ERR_INVALID_COMMAND,
    ERR_INVALID_ARGS
} ResultCode;

// 연산

Node *create_node(char data) {
    Node *n = (Node *)malloc(sizeof(Node));
    if (!n) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    n->data = data;
    n->left = n->right = n->parent = NULL;
    return n;
}

// size개를 저장할 수 있는 빈 노드 생성
Tree *create_btree(int size) {
    Tree *t = (Tree *)malloc(sizeof(Tree));
    if (!t) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    t->root = NULL;
    t->size = size;
    t->count = 0;
    return t;
}

static void free_subtree(Node *n) {
    if (!n) return;
    free_subtree(n->left);
    free_subtree(n->right);
    free(n);
}

// 모든 노드 삭제 및 트리 삭제
void destroy_btree(Tree *tree) {
    if (!tree) return;
    free_subtree(tree->root);
    free(tree);
}

// 대 소문자 구별 없는 문자열 비교
static int ci_equal(const char *a, const char *b) {
    while (*a && *b) {
        if (toupper((unsigned char)*a) != toupper((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

// 노드 탐색 + 탐색 불가 시 NULL까지
Node *find_node(Tree *tree, const char *path) {
    if (!path || path[0] != '/') return NULL;
    if (!tree->root) return NULL;

    char buf[MAX_PATH_LEN];
    strncpy(buf, path, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *token = strtok(buf, "/");
    if (!token || strlen(token) != 1) return NULL;

    if (tree->root->data != token[0]) return NULL;
    Node *cur = tree->root;

    token = strtok(NULL, "/");
    while (token) {
        if (strlen(token) != 1) return NULL;
        char c = token[0];
        if (cur->left && cur->left->data == c) {
            cur = cur->left;
        } else if (cur->right && cur->right->data == c) {
            cur = cur->right;
        } else {
            return NULL;
        }
        token = strtok(NULL, "/");
    }
    return cur;
}

// 빈트리에 루트 노드 생성
ResultCode insert_root(Tree *tree, char value) {
    if (tree->root) return ERR_TREE_NOT_EMPTY;
    if (tree->count >= tree->size) return ERR_TREE_FULL;
    tree->root = create_node(value);
    tree->count++;
    return OK;
}

// 부모 아래 자식 추가
// 부모는 비어있는 위치에 있어야 하며 지정한 위치가 차 있거나, 반대편 형제와 값이 같아도 오류 출력
ResultCode insert_child(Tree *tree, const char *parent_path, char child, char value) {
    if (child != 'L' && child != 'R') return ERR_INVALID_CHILD;

    Node *parent = find_node(tree, parent_path);
    if (!parent) return ERR_PATH_NOT_FOUND;

    if (tree->count >= tree->size) return ERR_TREE_FULL;

    Node **target = (child == 'L') ? &parent->left : &parent->right;
    Node *sibling = (child == 'L') ? parent->right : parent->left;

    if (*target != NULL) return ERR_POSITION_OCCUPIED;
    if (sibling != NULL && sibling->data == value) return ERR_DUPLICATE_VALUE;

    Node *n = create_node(value);
    n->parent = parent;
    *target = n;
    tree->count++;
    return OK;
}

// 단말 노드 삭제
ResultCode delete_node(Tree *tree, const char *leaf_path) {
    Node *n = find_node(tree, leaf_path);
    if (!n) return ERR_PATH_NOT_FOUND;
    if (n->left || n->right) return ERR_NOT_LEAF;

    if (n->parent == NULL) {
        tree->root = NULL;
    } else if (n->parent->left == n) {
        n->parent->left = NULL;
    } else {
        n->parent->right = NULL;
    }
    free(n);
    tree->count--;
    return OK;
}

// 노드의 데이터 변경
ResultCode update_value(Tree *tree, const char *node_path, char value) {
    Node *n = find_node(tree, node_path);
    if (!n) return ERR_PATH_NOT_FOUND;

    if (n->parent) {
        Node *sibling = (n->parent->left == n) ? n->parent->right : n->parent->left;
        if (sibling && sibling->data == value) return ERR_DUPLICATE_VALUE;
    }
    n->data = value;
    return OK;
}

// 자식 정보를 화면에 출력
ResultCode read_child(Tree *tree, const char *parent_path) {
    Node *n = find_node(tree, parent_path);
    if (!n) return ERR_PATH_NOT_FOUND;

    if (!n->left && !n->right) {
        printf("This node is a leaf and has no children.\n");
        return OK;
    }

    int first = 1;
    if (n->left) {
        printf("%c(L)", n->left->data);
        first = 0;
    }
    if (n->right) {
        if (!first) printf(", ");
        printf("%c(R)", n->right->data);
    }
    printf("\n");
    return OK;
}

// 트리를 왼쪽으로 눕힌 형태로 출력
static void print_recursive(Node *n, int depth) {
    if (!n) return;

    if (depth == 0) {
        printf("%c\n", n->data);
    } else {
        for (int i = 0; i < depth - 1; i++) printf("    ");
        printf("+---%c\n", n->data);
    }
    print_recursive(n->left, depth + 1);
    print_recursive(n->right, depth + 1);
}

void print_btree(Tree *tree) {
    if (!tree->root) {
        printf("The tree is empty.\n");
        return;
    }
    print_recursive(tree->root, 0);
}

// 오류 메세지

void print_error(ResultCode code) {
    switch (code) {
        case ERR_TREE_EMPTY:
            printf("[Error] The tree is empty.\n");
            break;
        case ERR_TREE_NOT_EMPTY:
            printf("[Error] A root node already exists. Cannot add a new root.\n");
            break;
        case ERR_TREE_FULL:
            printf("[Error] Cannot add more nodes to the tree (maximum size exceeded).\n");
            break;
        case ERR_PATH_NOT_FOUND:
            printf("[Error] The node at the specified path was not found.\n");
            break;
        case ERR_INVALID_CHILD:
            printf("[Error] The child value must be L or R.\n");
            break;
        case ERR_POSITION_OCCUPIED:
            printf("[Error] A child node already exists at the specified position.\n");
            break;
        case ERR_DUPLICATE_VALUE:
            printf("[Error] The left and right children of the same parent cannot have the same data.\n");
            break;
        case ERR_NOT_LEAF:
            printf("[Error] Cannot delete: the node is not a leaf node.\n");
            break;
        case ERR_INVALID_DATA:
            printf("[Error] Data must be a single uppercase letter.\n");
            break;
        case ERR_INVALID_COMMAND:
            printf("[Error] Unknown command.\n");
            break;
        case ERR_INVALID_ARGS:
            printf("[Error] Invalid number of arguments or command format.\n");
            break;
        default:
            break;
    }
}

// 명령어 파싱 보조 함수

static int is_single_upper(const char *s) {
    return s && strlen(s) == 1 && isupper((unsigned char)s[0]);
}

// 왼쪽 오른쪽 판별 메소드
static int parse_child_token(const char *s, char *out) {
    if (ci_equal(s, "L") || ci_equal(s, "Left"))  { *out = 'L'; return 1; }
    if (ci_equal(s, "R") || ci_equal(s, "Right")) { *out = 'R'; return 1; }
    return 0;
}

static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = '\0';
        len--;
    }
}
// 메인

int main(void) {
    Tree *tree = create_btree(1000);
    char line[MAX_LINE_LEN];

    printf("Starting the binary tree program. (empty tree)\n");
    printf("Commands: I[nsert], D[elete], U[pdate], R[ead], P[rint]\n\n");

    while (fgets(line, sizeof(line), stdin)) {
        trim_newline(line);
        if (line[0] == '\0') continue;

        char *tokens[MAX_TOKENS];
        int ntok = 0;
        char *tok = strtok(line, " \t");
        while (tok && ntok < MAX_TOKENS) {
            tokens[ntok++] = tok;
            tok = strtok(NULL, " \t");
        }
        if (ntok == 0) continue;

        char *cmd = tokens[0];

        // Insert
        if (strcmp(cmd, "Insert") == 0 || strcmp(cmd, "I") == 0) {
            if (ntok == 3) {
                // 루트 노드 생성
                if (strcmp(tokens[1], "/") != 0 || !is_single_upper(tokens[2])) {
                    print_error(ERR_INVALID_ARGS);
                    continue;
                }
                ResultCode rc = insert_root(tree, tokens[2][0]);
                if (rc == OK) {
                    printf("Created root node %c.\n", tokens[2][0]);
                } else {
                    print_error(rc);
                }
            } else if (ntok == 4) {
                char childpos;
                if (!is_single_upper(tokens[3])) {
                    print_error(ERR_INVALID_DATA);
                    continue;
                }
                if (!parse_child_token(tokens[2], &childpos)) {
                    print_error(ERR_INVALID_CHILD);
                    continue;
                }
                ResultCode rc = insert_child(tree, tokens[1], childpos, tokens[3][0]);
                if (rc == OK) {
                    printf("Added node %c at position %c under %s.\n",
                           tokens[3][0], childpos, tokens[1]);
                } else {
                    print_error(rc);
                }
            } else {
                print_error(ERR_INVALID_ARGS);
            }
        }
        // delete
        else if (strcmp(cmd, "Delete") == 0 || strcmp(cmd, "D") == 0) {
            if (ntok != 2) {
                print_error(ERR_INVALID_ARGS);
                continue;
            }
            ResultCode rc = delete_node(tree, tokens[1]);
            if (rc == OK) {
                printf("Deleted node %s.\n", tokens[1]);
            } else {
                print_error(rc);
            }
        }
        // Update
        else if (strcmp(cmd, "Update") == 0 || strcmp(cmd, "U") == 0) {
            if (ntok != 3) {
                print_error(ERR_INVALID_ARGS);
                continue;
            }
            if (!is_single_upper(tokens[2])) {
                print_error(ERR_INVALID_DATA);
                continue;
            }
            ResultCode rc = update_value(tree, tokens[1], tokens[2][0]);
            if (rc == OK) {
                printf("Changed the data of node %s to %c.\n", tokens[1], tokens[2][0]);
            } else {
                print_error(rc);
            }
        }
        // Read
        else if (strcmp(cmd, "Read") == 0 || strcmp(cmd, "R") == 0) {
            if (ntok != 2) {
                print_error(ERR_INVALID_ARGS);
                continue;
            }
            ResultCode rc = read_child(tree, tokens[1]);
            if (rc != OK) print_error(rc);
        }
        // Print
        else if (strcmp(cmd, "Print") == 0 || strcmp(cmd, "P") == 0) {
            if (ntok != 1) {
                print_error(ERR_INVALID_ARGS);
                continue;
            }
            print_btree(tree);
        }
        // 잘못 설정된 노드 시
        else {
            print_error(ERR_INVALID_COMMAND);
        }
    }

    destroy_btree(tree);
    printf("\nExiting the program.\n");
    return 0;
}