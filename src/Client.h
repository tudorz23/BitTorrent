#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <unordered_map>
#include <vector>

#include "helper_objects.h"


class Client {
 public:
    int numtasks;
    int rank;

    std::unordered_map<std::string, std::vector<Segment>> owned_files;
    std::vector<std::string> wanted_files;


    Client(int numtasks, int rank);

    void run();

    void initialize();

    void read_input_file();

    void send_owned_files_to_tracker();

    void receive_file_details_from_tracker(std::string &wanted_file, std::vector<int> &swarm,
                                           std::vector<Segment> &segments);

    void receive_file_swarm_from_tracker(std::vector<int> &swarm);

    void receive_file_segment_details_from_tracker(std::vector<Segment> &segments);

    // For debug.
    void print_files_after_read();

    void print_swarm_for_file(std::string &file, std::vector<int> &swarm);

    void print_segment_details_for_file(std::string &file, std::vector<Segment> &segments);
};


void *download_thread_func(void *arg);


void *upload_thread_func(void *arg);


#endif /* CLIENT_H */
