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

 private:
    void initialize();

    void recv_file_details_from_client(int client_idx);

    void handle_file_details_request(int client_idx);

    void send_file_swarm_to_client(const std::string &file_name, int client_idx);

    void send_file_segment_details_to_client(const std::string &file_name, int client_idx);

    void handle_update_swarm_request(int client_idx);

    void handle_file_download_complete_from_client(int client_idx);

    void announce_all_clients_to_stop();

    // For debug.
    void print_database_and_swarms();
};


#endif /* TRACKER_H */
