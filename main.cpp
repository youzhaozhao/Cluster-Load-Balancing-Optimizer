/**
 * 数据结构 Project: 服务器集群负载均衡优化器
 * 
 * 优化策略:
 * 1. 基础: Floyd-Warshall 计算全图最短路径。
 * 2. 初解: 使用贪心算法生成一个合法的初始方案。
 * 3. 提升: 使用模拟退火寻找全局更优解。
 * 4. 模拟: 根据定好的起终点，模拟网络带宽限制下的具体移动过程。
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <iomanip>

using namespace std;

const int MAXN = 55;    // 最大节点数
const int INF = 1e9;    // 定义无穷大，用于初始化距离矩阵，表示不可达

struct Node {
    int id;
    int capacity;       // 节点的总容量限制
    int current_usage;  // 当前已分配的任务总负载
};

struct Task {
    int id;
    int start_node;     // 初始所在节点
    int demand;         // 任务占用的资源量
    
    // 规划结果
    int end_node;       // 算法计算出的最终目标节点
    int migration_cost; // 从 start 到 end 的迁移代价
    
    // 模拟状态
    vector<int> path;   // 存储从起点到终点的具体路径节点序列
    size_t path_idx;    // 当前处于路径数组的索引位置
    int current_pos_node;   // 模拟过程中，任务当前所在的节点
    bool finished;          // 标记任务是否已经到达终点
};

// 全局数据
int N, M, T;            // N:节点数, M:边数, T:任务数
Node nodes[MAXN];       // 存储所有节点信息的数组
int adj_bandwidth[MAXN][MAXN];  // 邻接矩阵：存储直接连接的带宽
int dist[MAXN][MAXN];           // 距离矩阵：存储两点间最短路径的成本
int next_hop[MAXN][MAXN];       // 路由表：从节点 i 到 j 的最短路径上，下一节点是谁
vector<Task> tasks;             // 任务列表

// 日志结构
struct LogEntry {
    int time;
    int task_id;
    int from;
    int to;
};
vector<LogEntry> logs;
int total_time_steps = 0;       // 记录完成所有迁移所需的总时间步数

// 初始化与输入

void init() {
    // 初始化 dist 矩阵、邻接矩阵、路由表
    for (int i = 1; i <= N; ++i) {
        for (int j = 1; j <= N; ++j) {
            if (i == j) {
                dist[i][j] = 0;
            } else {
                dist[i][j] = INF;
            }
            adj_bandwidth[i][j] = 0;
            next_hop[i][j] = j; // 默认下一跳为目标节点本身
        }
    }
}

void readInput() {
    // 读取 N, M, T、读取节点信息、读取链路信息、读取任务信息
    if (!(cin >> N >> M >> T)) return;
    
    init();

    // 读取节点信息
    for (int i = 0; i < N; ++i) {
        int id, cap;
        cin >> id >> cap;
        nodes[id].id = id;
        nodes[id].capacity = cap;
        nodes[id].current_usage = 0;
    }

    // 读取链路信息
    for (int i = 0; i < M; ++i) {
        int u, v, c, b;
        cin >> u >> v >> c >> b;
        adj_bandwidth[u][v] = adj_bandwidth[v][u] = b;
        // 初始化距离矩阵：如果输入多条边，保留成本最小的一条
        if (c < dist[u][v]) {
            dist[u][v] = dist[v][u] = c;    
        }
    }

    // 读取任务信息
    for (int i = 0; i < T; ++i) {
        int tid, s_node, dem;
        cin >> tid >> s_node >> dem;
        tasks.push_back({tid, s_node, dem, s_node, 0, {}, 0, s_node, false});
    }
}

// Floyd-Warshall 最短路径
// 三重循环遍历所有节点，计算出任意两点间迁移任务的最小单位成本。
// 同时，next_hop 记录路径重构所需的信息（要从 i 去 j，应该先去 next_hop[i][j]）。
void floydWarshall() {
    // k: 中转节点，i: 起点，j: 终点
    for (int k = 1; k <= N; ++k) {
        for (int i = 1; i <= N; ++i) {
            for (int j = 1; j <= N; ++j) {
                // 检查 i->k 和 k->j 是否连通
                if (dist[i][k] != INF && dist[k][j] != INF) {
                    // 如果经过 k 中转的总距离小于当前已知距离
                    if (dist[i][k] + dist[k][j] < dist[i][j]) {
                        dist[i][j] = dist[i][k] + dist[k][j];   
                        next_hop[i][j] = next_hop[i][k];       
                    }
                }
            }
        }
    }
}

// 贪心分配，生成初始解
void solveAllocationGreedy() {
    vector<int> task_indices(T);
    for(int i=0; i<T; ++i) task_indices[i] = i;

    // 按任务需求降序排列，优先安排大任务填满空间
    sort(task_indices.begin(), task_indices.end(), [&](int a, int b) {
        return tasks[a].demand > tasks[b].demand;
    });

    // 清空节点负载记录，重新计算
    for(int i=1; i<=N; ++i) nodes[i].current_usage = 0;

    // 按排序后的顺序遍历每个任务
    for (int idx : task_indices) {
        Task& t = tasks[idx];
        int best_node = -1;
        int min_cost = -1; 

        // 遍历所有节点，找合法的最小成本节点
        for (int target = 1; target <= N; ++target) {
            // 如果不可达，跳过
            if (dist[t.start_node][target] == INF) continue;

            // 检查容量约束：如果放进去后不超过该节点容量
            if (nodes[target].current_usage + t.demand <= nodes[target].capacity) {
                // 计算迁移成本
                int cost = dist[t.start_node][target] * t.demand;
                if (best_node == -1 || cost < min_cost) {
                    min_cost = cost;
                    best_node = target;
                }
            }
        }

        // 找到最佳节点后，执行分配
        if (best_node != -1) {
            t.end_node = best_node;
            t.migration_cost = min_cost;
            nodes[best_node].current_usage += t.demand;
        }
    }
}

// 模拟退火优化
// 在贪心解的基础上，通过随机扰动寻找全局更优解。
// 计算当前方案下所有任务的总迁移成本
long long calculateTotalCost() {
    long long total = 0;
    for(const auto& t : tasks) {
        total += (long long)dist[t.start_node][t.end_node] * t.demand;
    }
    return total;
}

void optimizeAllocationSA() {
    srand(time(NULL));

    // 初始温度参数
    double T_start = 2000.0;    // 初始温度
    double T_end = 1e-8;         // 终止温度
    double cooling_rate = 0.999;    // 降温系数

    double current_temp = T_start;
    long long current_cost = calculateTotalCost();   // 当前总成本

    // 记录全局最优
    long long best_cost = current_cost;
    vector<int> best_assignment(T);
    for(int i=0; i<T; ++i) best_assignment[i] = tasks[i].end_node;

    // 使用时钟控制
    double time_limit = 1.8; 
    clock_t start_clock = clock();

    int iter = 0;
    while (true) {
        // 每 1024 次检查一次时间
        if ((iter & 1023) == 0) {
            double elapsed = (double)(clock() - start_clock) / CLOCKS_PER_SEC;
            if (elapsed > time_limit) break;
        }
        iter++;

        int t_idx = rand() % T;
        Task& t = tasks[t_idx];
        int old_node = t.end_node;
        int new_node = (rand() % N) + 1;

        if (new_node == old_node || dist[t.start_node][new_node] == INF) continue;

        if (nodes[new_node].current_usage + t.demand <= nodes[new_node].capacity) {
            long long cost_diff = ((long long)dist[t.start_node][new_node] * t.demand) - 
                                  ((long long)dist[t.start_node][old_node] * t.demand);

            if (cost_diff < 0 || exp(-cost_diff / current_temp) > ((double)rand() / RAND_MAX)) {
                nodes[old_node].current_usage -= t.demand;
                nodes[new_node].current_usage += t.demand;
                t.end_node = new_node;
                t.migration_cost = dist[t.start_node][new_node] * t.demand;
                current_cost += cost_diff;

                if (current_cost < best_cost) {
                    best_cost = current_cost;
                    for(int k=0; k<T; ++k) best_assignment[k] = tasks[k].end_node;
                }
            }
        }

        // 动态降温策略
        current_temp *= cooling_rate;
        // 如果温度过低，重置温度，继续利用剩余时间搜索
        if (current_temp < T_end) {
            current_temp = T_start * 0.5; 
        }
    }

    // 恢复最优解
    for(int i=1; i<=N; ++i) nodes[i].current_usage = 0;
    for(int i=0; i<T; ++i) {
        tasks[i].end_node = best_assignment[i];
        tasks[i].migration_cost = dist[tasks[i].start_node][tasks[i].end_node] * tasks[i].demand;
        nodes[tasks[i].end_node].current_usage += tasks[i].demand;
    }
}

// 模拟迁移
// 利用 next_hop 数组重构路径
void reconstructPath(Task& t) {
    if (t.start_node == t.end_node) return;
    int curr = t.start_node;
    while (curr != t.end_node) {
        int next = next_hop[curr][t.end_node];  
        t.path.push_back(next);     
        curr = next;    
    }
}

void simulateMigration() {
    // 为所有需要移动的任务生成路径
    for (auto& t : tasks) {
        if (t.start_node != t.end_node) {
            reconstructPath(t);
            t.path_idx = 0;
            t.current_pos_node = t.start_node;
            t.finished = false;
        } else {
            t.finished = true;
        }
    }

    int current_time = 0;
    bool any_unfinished = true;

    while (any_unfinished) {
        // 检查是否所有任务都已完成
        any_unfinished = false;
        for(const auto& t : tasks) {
            if(!t.finished) {
                any_unfinished = true;
                break;
            }
        }
        if (!any_unfinished) break;

        current_time++;
        
        // 记录当前这一秒，每条链路上的任务数量
        map<int, int> link_usage;
        // 记录这一秒成功获得移动权的任务下标
        vector<int> tasks_moved_indices;

        // 遍历所有未完成任务，检查是否能移动
        for (int i = 0; i < T; ++i) {
            if (tasks[i].finished) continue;

            Task& t = tasks[i];
            int u = t.current_pos_node;
            int v = t.path[t.path_idx]; 

            int key = (u < v) ? (u * 1000 + v) : (v * 1000 + u);
            int bw = adj_bandwidth[u][v];

            if (link_usage[key] < bw) {
                link_usage[key]++;
                tasks_moved_indices.push_back(i);
            }
        }

        if (tasks_moved_indices.empty() && any_unfinished) break; 

        // 执行移动
        for (int idx : tasks_moved_indices) {
            Task& t = tasks[idx];
            int from = t.current_pos_node;
            int to = t.path[t.path_idx];
            
            logs.push_back({current_time, t.id, from, to});
            
            t.current_pos_node = to;
            t.path_idx++;
            
            if (t.path_idx >= t.path.size()) {
                t.finished = true;
            }
        }
    }
    total_time_steps = current_time;
}

// 输出
void printOutput() {
    vector<Task> sorted_tasks = tasks;
    sort(sorted_tasks.begin(), sorted_tasks.end(), [](const Task& a, const Task& b){
        return a.id < b.id;
    });

    long long total_migration_cost = 0;

    // 输出任务分配详情
    for (const auto& t : sorted_tasks) {
        cout << t.id << " " << t.start_node << " " << t.end_node << " " << t.migration_cost << endl;
        total_migration_cost += t.migration_cost;
    }

    // 输出各节点最终负载
    for (int i = 1; i <= N; ++i) {
        cout << nodes[i].id << " " << nodes[i].current_usage << endl;
    }

    cout << total_migration_cost << endl;
    
    cout << total_time_steps << endl;
    for (const auto& log : logs) {
        cout << log.time << " " << log.task_id << " " << log.from << " " << log.to << endl;
    }
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    readInput();
    floydWarshall();            // 计算最短路径
    solveAllocationGreedy();    // 贪心初解
    optimizeAllocationSA();     // 模拟退火优化
    simulateMigration();        // 模拟迁移过程
    printOutput();

    return 0;
}