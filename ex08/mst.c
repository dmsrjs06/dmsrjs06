#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define V 50
#define MAX_W 100
#define REPS 20000 /* 실행시간 측정 반복 횟수 (평균을 사용) */

typedef struct { int u, v, w; } Edge;

typedef struct Node {
    int to, w;
    struct Node *next;
} Node;

/* ---------------- 시간 측정 (ns 단위, 모든 케이스 동일 방법) ---------------- */
static double now_ns(void) {
#ifdef _WIN32
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart * 1e9 / (double)f.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
#endif
}

/* ---------------- 그래프 생성 ---------------- */
/* 연결 그래프 보장: 먼저 임의 신장 트리(V-1개)를 만들고, 나머지 간선을 중복 없이 추가 */
static int generate_graph(Edge *edges, int E) {
    static char used[V][V];
    int perm[V], cnt = 0;
    memset(used, 0, sizeof(used));

    for (int i = 0; i < V; i++) perm[i] = i;
    for (int i = V - 1; i > 0; i--) { /* Fisher-Yates 셔플 */
        int j = rand() % (i + 1), t = perm[i];
        perm[i] = perm[j]; perm[j] = t;
    }
    for (int i = 1; i < V; i++) {
        int a = perm[i], b = perm[rand() % i];
        used[a][b] = used[b][a] = 1;
        edges[cnt++] = (Edge){a, b, rand() % MAX_W + 1};
    }
    while (cnt < E) {
        int a = rand() % V, b = rand() % V;
        if (a == b || used[a][b]) continue; /* self-loop, 중복 간선 금지 */
        used[a][b] = used[b][a] = 1;
        edges[cnt++] = (Edge){a, b, rand() % MAX_W + 1};
    }
    return cnt;
}

/* 연결성 검사: 라벨 전파를 변화가 없을 때까지 반복 */
static int is_connected(const Edge *edges, int E) {
    int comp[V], changed = 1;
    for (int i = 0; i < V; i++) comp[i] = i;
    while (changed) {
        changed = 0;
        for (int i = 0; i < E; i++) {
            int a = comp[edges[i].u], b = comp[edges[i].v];
            if (a < b) { comp[edges[i].v] = a; changed = 1; }
            else if (b < a) { comp[edges[i].u] = b; changed = 1; }
        }
    }
    for (int i = 1; i < V; i++) if (comp[i] != 0) return 0;
    return 1;
}

/* ---------------- 그래프 표현 ---------------- */
static Node **build_adj_list(const Edge *edges, int E) {
    Node **head = calloc(V, sizeof(Node *));
    for (int i = 0; i < E; i++) {
        Node *a = malloc(sizeof(Node)), *b = malloc(sizeof(Node));
        *a = (Node){edges[i].v, edges[i].w, head[edges[i].u]}; head[edges[i].u] = a;
        *b = (Node){edges[i].u, edges[i].w, head[edges[i].v]}; head[edges[i].v] = b;
    }
    return head;
}

static void free_adj_list(Node **head) {
    for (int i = 0; i < V; i++) {
        Node *p = head[i];
        while (p) { Node *n = p->next; free(p); p = n; }
    }
    free(head);
}

static void build_adj_matrix(const Edge *edges, int E, int m[V][V]) {
    memset(m, 0, sizeof(int) * V * V); /* 0 = 간선 없음 (가중치는 양수) */
    for (int i = 0; i < E; i++)
        m[edges[i].u][edges[i].v] = m[edges[i].v][edges[i].u] = edges[i].w;
}

/* ---------------- Kruskal (인접 리스트) ---------------- */
static int cmp_edge(const void *a, const void *b) {
    return ((const Edge *)a)->w - ((const Edge *)b)->w;
}
static int uf_find(int *p, int x) {
    while (p[x] != x) { p[x] = p[p[x]]; x = p[x]; }
    return x;
}

static long kruskal(Node **head, int E, Edge *mst, int *mst_cnt) {
    Edge *list = malloc(sizeof(Edge) * E);
    int parent[V], rank[V], n = 0, cnt = 0;
    long total = 0;

    /* 인접 리스트에서 간선 추출 (u < v 인 것만 -> 중복 제거) */
    for (int u = 0; u < V; u++)
        for (Node *p = head[u]; p; p = p->next)
            if (u < p->to) list[n++] = (Edge){u, p->to, p->w};

    qsort(list, n, sizeof(Edge), cmp_edge);
    for (int i = 0; i < V; i++) { parent[i] = i; rank[i] = 0; }

    for (int i = 0; i < n && cnt < V - 1; i++) {
        int a = uf_find(parent, list[i].u), b = uf_find(parent, list[i].v);
        if (a == b) continue; /* 사이클 */
        if (rank[a] < rank[b]) { int t = a; a = b; b = t; }
        parent[b] = a;
        if (rank[a] == rank[b]) rank[a]++;
        mst[cnt++] = list[i];
        total += list[i].w;
    }
    free(list);
    *mst_cnt = cnt;
    return total;
}

/* ---------------- Prim (인접 행렬) ---------------- */
static long prim(int m[V][V], Edge *mst, int *mst_cnt) {
    int key[V], par[V];
    char vis[V];
    long total = 0;
    int cnt = 0;

    for (int i = 0; i < V; i++) { key[i] = 1 << 30; par[i] = -1; vis[i] = 0; }
    key[0] = 0;

    for (int it = 0; it < V; it++) {
        int u = -1;
        for (int i = 0; i < V; i++) /* 최소 key 정점 선택: O(V) */
            if (!vis[i] && (u == -1 || key[i] < key[u])) u = i;
        vis[u] = 1;
        if (par[u] != -1) {
            mst[cnt++] = (Edge){par[u], u, key[u]};
            total += key[u];
        }
        for (int v = 0; v < V; v++)
            if (m[u][v] && !vis[v] && m[u][v] < key[v]) { key[v] = m[u][v]; par[v] = u; }
    }
    *mst_cnt = cnt;
    return total;
}

/* ---------------- 메모리 계산 (sizeof 기반) ---------------- */
static size_t mem_kruskal(int E) {
    return sizeof(Node *) * V          /* 인접 리스트 head 배열 */
         + sizeof(Node) * 2 * E        /* 인접 리스트 노드 (무방향 -> 2E개) */
         + sizeof(Edge) * E            /* 정렬용 간선 배열 */
         + sizeof(int) * V * 2         /* Union-Find parent, rank */
         + sizeof(Edge) * (V - 1);     /* MST 결과 간선 */
}
static size_t mem_prim(void) {
    return sizeof(int) * V * V         /* 인접 행렬 */
         + sizeof(int) * V * 2         /* key[], parent[] */
         + sizeof(char) * V            /* visited[] */
         + sizeof(Edge) * (V - 1);     /* MST 결과 간선 */
}

/* ---------------- 결과 출력 ---------------- */
static void print_mst(const Edge *mst, int cnt) {
    printf("MST Edges List:\n");
    for (int i = 0; i < cnt; i++)
        printf("  (%2d - %2d) w=%3d%s", mst[i].u, mst[i].v, mst[i].w, (i % 4 == 3) ? "\n" : "");
    if (cnt % 4) printf("\n");
}

static void run_case(int no, const char *title, const char *kind, int E,
                     Node **head, int m[V][V], int use_kruskal, long *cost_out) {
    Edge mst[V];
    int cnt = 0;
    long cost = 0;
    double t0, t1;

    /* 1회 실행 (결과 출력용) */
    cost = use_kruskal ? kruskal(head, E, mst, &cnt) : prim(m, mst, &cnt);

    /* 실행시간: REPS회 반복 후 1회 평균 (MST 계산만 측정) */
    t0 = now_ns();
    for (int r = 0; r < REPS; r++) {
        int c;
        if (use_kruskal) kruskal(head, E, mst, &c);
        else prim(m, mst, &c);
    }
    t1 = now_ns();
    double avg_ms = (t1 - t0) / REPS / 1e6;

    /* 출력용 MST 재계산 (측정 루프가 mst를 덮어쓰므로) */
    cost = use_kruskal ? kruskal(head, E, mst, &cnt) : prim(m, mst, &cnt);

    printf("Case %d : %s - %s\n", no, kind, title);
    printf("Vertices      : %d\n", V);
    printf("Edges         : %d\n", E);
    printf("MST Edges     : %d\n", cnt);
    printf("MST Cost      : %ld\n", cost);
    printf("Memory        : %zu Bytes\n", use_kruskal ? mem_kruskal(E) : mem_prim());
    printf("Execution Time: %.6f ms (avg of %d runs)\n", avg_ms, REPS);
    print_mst(mst, cnt);
    printf("\n");
    *cost_out = cost;
}

int main(int argc, char **argv) {
    unsigned seed = (argc > 1) ? (unsigned)atoi(argv[1]) : (unsigned)time(NULL);
    srand(seed);
    printf("Random seed: %u\n\n", seed);

    int Es[2] = {100, 600};
    const char *kinds[2] = {"Sparse Graph", "Dense Graph"};
    static Edge edges[2][600];
    static int matrix[2][V][V];
    Node **lists[2];

    for (int g = 0; g < 2; g++) {
        generate_graph(edges[g], Es[g]);
        while (!is_connected(edges[g], Es[g])) generate_graph(edges[g], Es[g]);
        lists[g] = build_adj_list(edges[g], Es[g]);
        build_adj_matrix(edges[g], Es[g], matrix[g]);
    }

    long cost[4];
    run_case(1, "Adjacency List - Kruskal", kinds[0], Es[0], lists[0], matrix[0], 1, &cost[0]);
    run_case(2, "Adjacency List - Kruskal", kinds[1], Es[1], lists[1], matrix[1], 1, &cost[1]);
    run_case(3, "Adjacency Matrix - Prim",  kinds[0], Es[0], lists[0], matrix[0], 0, &cost[2]);
    run_case(4, "Adjacency Matrix - Prim",  kinds[1], Es[1], lists[1], matrix[1], 0, &cost[3]);

    printf("=== 검증 ===\n");
    printf("Sparse: Kruskal %ld vs Prim %ld -> %s\n", cost[0], cost[2], cost[0] == cost[2] ? "일치" : "불일치!");
    printf("Dense : Kruskal %ld vs Prim %ld -> %s\n", cost[1], cost[3], cost[1] == cost[3] ? "일치" : "불일치!");

    free_adj_list(lists[0]);
    free_adj_list(lists[1]);
    return 0;
}