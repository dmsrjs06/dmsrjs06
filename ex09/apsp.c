#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define V 10
#define E 20
#define MAX_W 100
#define INF 1000000000

typedef struct { int u, v, w; } Edge;

static Edge edges[E];
static int adj[V][V];   /* 0 = 간선 없음 (가중치는 양수) */
static int dist[V][V];
static int nxt[V][V];   /* nxt[i][j]: i에서 j로 갈 때 i 다음 정점 */

/* 연결성 검사: 라벨 전파 */
static int is_connected(void) {
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

/* 연결 그래프가 될 때까지 재생성 (self-loop, 중복 간선 금지) */
static void generate_graph(void) {
    do {
        memset(adj, 0, sizeof(adj));
        int cnt = 0;
        while (cnt < E) {
            int a = rand() % V, b = rand() % V;
            if (a == b || adj[a][b]) continue;
            int w = rand() % MAX_W + 1;
            adj[a][b] = adj[b][a] = w;
            edges[cnt++] = (Edge){a < b ? a : b, a < b ? b : a, w};
        }
    } while (!is_connected());
}

static int cmp_edge(const void *a, const void *b) {
    const Edge *x = a, *y = b;
    return x->u != y->u ? x->u - y->u : x->v - y->v;
}

static void floyd_warshall(void) {
    for (int i = 0; i < V; i++)
        for (int j = 0; j < V; j++) {
            if (i == j)        { dist[i][j] = 0;        nxt[i][j] = i; }
            else if (adj[i][j]) { dist[i][j] = adj[i][j]; nxt[i][j] = j; }
            else               { dist[i][j] = INF;      nxt[i][j] = -1; }
        }
    for (int k = 0; k < V; k++)
        for (int i = 0; i < V; i++)
            for (int j = 0; j < V; j++)
                if (dist[i][k] + dist[k][j] < dist[i][j]) {
                    dist[i][j] = dist[i][k] + dist[k][j];
                    nxt[i][j] = nxt[i][k];
                }
}

/* 검증용 Dijkstra (O(V^2)) */
static void dijkstra(int s, int d[V]) {
    char vis[V] = {0};
    for (int i = 0; i < V; i++) d[i] = INF;
    d[s] = 0;
    for (int it = 0; it < V; it++) {
        int u = -1;
        for (int i = 0; i < V; i++)
            if (!vis[i] && (u == -1 || d[i] < d[u])) u = i;
        vis[u] = 1;
        for (int v = 0; v < V; v++)
            if (adj[u][v] && d[u] + adj[u][v] < d[v]) d[v] = d[u] + adj[u][v];
    }
}

int main(int argc, char **argv) {
    unsigned seed = (argc > 1) ? (unsigned)atoi(argv[1]) : (unsigned)time(NULL);
    srand(seed);
    generate_graph();
    qsort(edges, E, sizeof(Edge), cmp_edge);

    printf("Random seed: %u\n\n", seed);
    printf("Graph\nVertices : %d\nEdges    : %d\n\n", V, E);
    printf("Edges\n");
    for (int i = 0; i < E; i++)
        printf("(%d, %d) Weight: %d\n", edges[i].u, edges[i].v, edges[i].w);

    floyd_warshall();

    printf("\nShortest Paths\n");
    int pairs = 0, ok_sum = 0, ok_dij = 0;
    for (int u = 0; u < V; u++) {
        int dj[V];
        dijkstra(u, dj);
        for (int v = u + 1; v < V; v++) {
            int path[V], len = 0, sum = 0, valid = 1;
            for (int x = u; x != v; x = nxt[x][v]) {
                path[len++] = x;
                if (len > V) { valid = 0; break; }
            }
            path[len++] = v;
            for (int i = 0; valid && i + 1 < len; i++) {
                int w = adj[path[i]][path[i + 1]];
                if (!w) valid = 0; else sum += w;
            }
            printf("\n%d -> %d\nPath     : ", u, v);
            for (int i = 0; i < len; i++) printf(i ? " -> %d" : "%d", path[i]);
            printf("\nDistance : %d\n", dist[u][v]);
            pairs++;
            if (valid && sum == dist[u][v]) ok_sum++;
            if (dj[v] == dist[u][v]) ok_dij++;
        }
    }

    printf("\nTotal vertex pairs : %d\n", pairs);

    /* 확인 사항 */
    int self = 0, dup = 0;
    for (int i = 0; i < E; i++) {
        if (edges[i].u == edges[i].v) self++;
        for (int j = i + 1; j < E; j++)
            if (edges[i].u == edges[j].u && edges[i].v == edges[j].v) dup++;
    }
    printf("\n=== Verification ===\n");
    printf("Vertices = %d, Edges = %d\n", V, E);
    printf("Self-loops: %d, Duplicate edges: %d\n", self, dup);
    printf("Connected graph      : %s\n", is_connected() ? "Yes" : "No");
    printf("Pairs printed        : %d / 45\n", pairs);
    printf("Path weight sum == distance : %d / %d\n", ok_sum, pairs);
    printf("Matches Dijkstra            : %d / %d\n", ok_dij, pairs);
    return 0;
}