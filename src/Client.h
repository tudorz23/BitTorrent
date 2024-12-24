#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <unordered_map>
#include <vector>
#include <pthread.h>

#include "helper_objects.h"


class Client {
 public:
    int numtasks;
    int rank;
    int load;
    pthread_mutex_t owned_files_mutex;

    std::unordered_map<std::string, std::vector<Segment>> owned_files;
    std::vector<std::string> wanted_files;


    Client(int numtasks, int rank);

    ~Client();

    void run();

    void initialize();

    void read_input_file();

    void send_owned_files_to_tracker();

    void receive_file_details_from_tracker(std::string &wanted_file, std::vector<int> &swarm,
                                           std::vector<Segment> &segments);

    void receive_file_swarm_from_tracker(std::vector<int> &swarm);

    void receive_file_segment_details_from_tracker(std::vector<Segment> &segments);

    void update_swarm_from_tracker(std::string &wanted_file, std::vector<int> &swarm);

    int get_peer_with_min_load_for_segment(std::string &file, int segment_idx,
                                           std::vector<int> &swarm);

    void handle_has_segment_req_from_peer(int peer_idx);

    void handle_get_segment_req_from_peer(int peer_idx);

    void announce_tracker_whole_file_received(std::string &file);

    void save_file(std::string &file);

    void announce_tracker_all_files_received();

    // For debug.
    void print_files_after_read();

    void print_swarm_for_file(std::string &file, std::vector<int> &swarm);

    void print_segment_details_for_file(std::string &file, std::vector<Segment> &segments);
};


void *download_thread_func(void *arg);

void *upload_thread_func(void *arg);


#endif /* CLIENT_H */
