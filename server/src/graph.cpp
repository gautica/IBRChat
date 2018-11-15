#include "graph.h"

void Graph::create_graph(std::vector<Serv_to_Serv> &connections)
{
    std::cout << "Creating graph...\n";
    for (unsigned int i = 0; i < connections.size(); i++) {
        create_node(connections.at(i));
    }
    std::cout << "Created graph with " << nodes.size() << " nodes:\n";
    for (unsigned int i = 0; i < nodes.size(); i++) {
        std::cout << nodes.at(i).ID << " with " << nodes.at(i).neighbors.size() << " neighbors" << "\n";
    }
}

void Graph::create_node(Serv_to_Serv &conn)
{
    
    Node node_1, node_2;
    node_1.ID = strtok(conn.conn, "-");
    node_2.ID = strtok(NULL, "-");

    std::cout << "node_1.ID: " << node_1.ID << "\n";
    std::cout << "node_2.ID: " << node_2.ID << "\n";
    int i;
    if ((i = isContained(node_1, this->nodes)) == -1) {
        node_1.neighbors.push_back(node_2);
        this->nodes.push_back(node_1);
    } else {
        if (isContained(node_2, nodes.at(i).neighbors) == -1) {
            nodes.at(i).neighbors.push_back(node_2);
        }
    }
    if ((i = isContained(node_2, this->nodes)) == -1) {
        node_2.neighbors.push_back(node_1);
        this->nodes.push_back(node_2);
    } else {
        if (isContained(node_1, nodes.at(i).neighbors) == -1) {
            nodes.at(i).neighbors.push_back(node_1);
        }
    }
    
}

int Graph::isContained(Node &node, std::vector<Node> &nodes)
{
    for (unsigned int i = 0; i < nodes.size(); i++) {
        if (node == nodes.at(i)) {
            return i;
        }
    }
    return -1;
}

void Graph::start_search(char* start, char* dest)
{
    Node starN, desN;
    for (unsigned int i = 0; i < this->nodes.size(); i++) {
        if (strcmp(nodes.at(i).ID, start) == 0)  {
            starN = nodes.at(i);
        }
        if (strcmp(nodes.at(i).ID, dest) == 0) {
            desN = nodes.at(i);
        }
    }
    depth_search(starN, desN);
}

void Graph::depth_search(Node &start, Node &dest)
{
    std::cout << "Searching...\n";
    std::cout << "With Start Node: " << start.ID << "\n";
    std::cout << "With End Node: " << dest.ID << "\n";

    path.push_back(start);
    Node last = path.back();
    while (last != dest) {
        for (unsigned int i = 0; i < last.neighbors.size(); i++) {
            if (!last.neighbors.at(i).isVisted) {
                path.push_back(last.neighbors.at(i));
                last.neighbors.at(i).isVisted = true;
                break;
            }
        }
        if (!have_neighbor(path.back())) {
            path.pop_back();
        }
        last = path.back();
    }
    std::cout << "Have found a path...\n";
}

char* Graph::get_next_Hup() {
    return path.back().ID;
}

bool Graph::have_neighbor(Node &node)
{
    for (unsigned int i = 0; i < node.neighbors.size(); i++) {
        if (!node.neighbors.at(i).isVisted) {
            return true;
        }
    }
}