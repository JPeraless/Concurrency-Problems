#include "deadlock_detector.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include "common.h"


class Graph {
public:
    std::vector<std::vector<int>> adj_list;
    std::vector<int> out_counts;
    std::vector<std::string> names;
} graph;


std::vector<std::string> topological_sort(Graph &g) {
    std::vector<std::string> deadlocked_robots;
    std::vector<int> out = g.out_counts;
    std::vector<int> zeros;

    for (size_t i = 0; i < out.size(); i++)
        if (out[i] == 0)
            zeros.emplace_back(i);

    while (!zeros.empty()) {
        int n = zeros.back();
        zeros.pop_back();
        for (const int& n2 : g.adj_list[n]) {
            out[n2]--;
            if (out[n2] == 0)
                zeros.emplace_back(n2);
        }
    }

    // Check for nodes that represent robots with non-zero out-degree, indicating a deadlock
    for (size_t i = 0; i < out.size(); i++)
        if (out[i] > 0 && g.names[i] != "$")
            deadlocked_robots.emplace_back(g.names[i]);

    return deadlocked_robots;
}


// Main function to detect deadlock
Result detect_deadlock(const std::vector<std::string> &edges) {
    Word2Int w2i;
    Result result;
    Graph g;

    int max_size = (edges.size() * 2) + 1;  // max possible number of nodes

    // Allocate enough memory for the graph
    g.adj_list.resize(max_size);
    g.out_counts.resize(max_size);
    g.names.resize(max_size);

    for (size_t i = 0; i < edges.size(); i++) {
        std::vector<std::string> e = split(edges[i]);

        // Convert node names to unique ints using Word2Int
        int robot = w2i.get(e[0]);
        int resource = w2i.get(e[2] + "$");
        
        // Keep track of robot names
        g.names[robot] = e[0];
        g.names[resource] = "$";

        // Check whether it's a request or assignment
        if (e[1] == "->") {
            g.adj_list[resource].emplace_back(robot);
            g.out_counts[robot]++;
        } else {
            g.adj_list[robot].emplace_back(resource);
            g.out_counts[resource]++;
        }

        // Run topological sort and return if there is a deadlock
        std::vector<std::string> deadlocked_robots = topological_sort(g);
        if (!deadlocked_robots.empty()) {
            result.dl_procs = deadlocked_robots;
            result.edge_index = i;
            return result;
        }
    }

    result.edge_index = -1;
    return result;
}
