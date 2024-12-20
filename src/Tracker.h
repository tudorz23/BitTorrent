#ifndef TRACKER_H
#define TRACKER_H

#include <unordered_map>
#include <vector>
#include <string>

#include "helper_objects.h"


class Tracker {
    int numtasks;
    int rank;

    // file -> (seeds, peers)
    std::unordered_map<std::string, Swarm> file_to_swarm;

    std::unordered_map<std::string, std::vector<Segment>> file_database;

 public:
    Tracker(int numtasks, int rank);


    void run();

    void initialize();

    void recv_file_details_from_client(int client_idx);

    void handle_file_details_request(int client_idx);

    void send_file_swarm_to_client(std::string &file_name, int client_idx);

    void send_file_segment_details_to_client(std::string &file_name, int client_idx);

    // For debug.
    void print_database_and_swarms();
};




#endif /* TRACKER_H */
