#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include "server.h"

struct Node {
    char* ID;
    bool isVisted = false;
    std::vector<Node> neighbors;
    bool operator==(Node &node) {
        return strcmp(this->ID, node.ID) == 0;
    }
    bool operator!=(Node &node) {
        return strcmp(this->ID, node.ID) != 0;
    }
};

class Graph 
{
public:
    Graph() {};

    void create_graph(std::vector<s2s_t> &connections);

    void start_search(char* start, char* dest);

    void get_path();

    char* get_next_Hop();

private:
    void create_node(s2s_t &conn);

    int isContained(Node &node, std::vector<Node> &nodes);

    void depth_search(Node &start, Node &dest);

    bool have_neighbor(Node &node);

private:
    std::vector<Node> nodes;
    std::vector<Node> path;
};

#endif