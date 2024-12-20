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

    void receive_file_swarm(std::string &wanted_file, std::vector<int> &swarm);

    // For debug.
    void print_files_after_read();

    void print_swarm_for_file(std::string &file, std::vector<int> &swarm);
};


void *download_thread_func(void *arg);


void *upload_thread_func(void *arg);


#endif /* CLIENT_H */
