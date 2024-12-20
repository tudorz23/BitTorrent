#ifndef HELPER_STRUCTS_H
#define HELPER_STRUCTS_H

#include <string>
#include <vector>


struct Segment {
    std::string hash;
    int index;

    Segment(std::string hash, int index);
};


class Swarm {
 public:
    std::vector<int> seeds;
    std::vector<int> peers;

    void add_seed(int seed);

    void add_peer(int peer);

    void remove_peer(int peer);

    int get_size();
};


#endif
