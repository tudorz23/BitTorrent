#include "helper_objects.h"

#include <algorithm>

Segment::Segment(std::string hash, int index) {
    this->hash = hash;
    this->index = index;
}


void Swarm::add_seed(int seed) {
    this->seeds.push_back(seed);
}


void Swarm::add_peer(int peer) {
    this->peers.push_back(peer);
}


void Swarm::remove_peer(int peer) {
    std::vector<int>::iterator pos = std::find(this->peers.begin(), this->peers.end(), peer);

    if (pos != this->peers.end()) {
        this->peers.erase(pos);
    }
}


int Swarm::get_size() {
    return seeds.size() + peers.size();
}
