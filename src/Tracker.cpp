#include "Tracker.h"

#include <mpi.h>
#include "constants.h"

using namespace std;


Tracker::Tracker(int numtasks, int rank) {
    this->numtasks = numtasks;
    this->rank = rank;
}


void Tracker::run() {
    initialize();

    int finished_clients = 0;
    bool should_stop = false;

    // Handle client requests.
    while (true) {
        MPI_Status status;
        int msg;

        // Receive "Hello" message.
        MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, TRACKER_TAG, MPI_COMM_WORLD, &status);

        switch (msg) {
            case FILE_DETAILS_REQ:
                handle_file_details_request(status.MPI_SOURCE);
                break;

            case UPDATE_SWARM_REQ:
                handle_update_swarm_request(status.MPI_SOURCE);
                break;

            case FILE_DOWNLOAD_COMPLETE:
                handle_file_download_complete_from_client(status.MPI_SOURCE);
                break;

            case ALL_FILES_RECEIVED:
                finished_clients++;
                if (finished_clients == this->numtasks - 1) {
                    should_stop = true;
                    announce_all_clients_to_stop();
                }
                break;
        }

        if (should_stop) {
            break;
        }
    }
}


void Tracker::initialize() {
    for (int client_idx = 1; client_idx < numtasks; client_idx++) {
        recv_file_details_from_client(client_idx);
    }

    // Send ACK to all clients.
    for (int client_idx = 1; client_idx < numtasks; client_idx++) {
        int msg = ACK;
        MPI_Send(&msg, 1, MPI_INT, client_idx, INIT_TAG, MPI_COMM_WORLD);
    }
}


void Tracker::recv_file_details_from_client(int client_idx) {
    // Receive the files count.
    int files_cnt;
    MPI_Recv(&files_cnt, 1, MPI_INT, client_idx, INIT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    for (int i = 0; i < files_cnt; i++) {
        // Receive file name (including '\0').
        char buff[MAX_FILENAME + 1];
        MPI_Recv(buff, MAX_FILENAME + 1, MPI_CHAR, client_idx, INIT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        string file_name(buff);

        // If the file is already in the database, don't store its segment details again.
        bool already_stored = this->file_database.find(file_name) != this->file_database.end();

        // Save the client as a seed for this file.
        this->file_to_swarm[file_name].add_seed(client_idx);

        // Receive segments count.
        int segments_cnt;
        MPI_Recv(&segments_cnt, 1, MPI_INT, client_idx, INIT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        for (int j = 0; j < segments_cnt; j++) {
            // Receive segment hash (add '\0' manually).
            char hash_buff[HASH_SIZE + 1];
            MPI_Recv(hash_buff, HASH_SIZE, MPI_CHAR, client_idx, INIT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            hash_buff[HASH_SIZE] = '\0';

            // Receive segment index.
            int index;
            MPI_Recv(&index, 1, MPI_INT, client_idx, INIT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if (!already_stored) {
                string hash(hash_buff);

                file_database[file_name].emplace_back(hash, index);
            }
        }
    }
}


void Tracker::handle_file_details_request(int client_idx) {
    // Receive file name (including '\0').
    char buff[MAX_FILENAME + 1];
    MPI_Recv(buff, MAX_FILENAME + 1, MPI_CHAR, client_idx, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    string file_name(buff);

    send_file_swarm_to_client(file_name, client_idx);
    send_file_segment_details_to_client(file_name, client_idx);

    // Set the client as a peer for the file.
    this->file_to_swarm[file_name].add_peer(client_idx);
}


void Tracker::send_file_swarm_to_client(const std::string &file_name, int client_idx) {
    // Send the swarm size.
    int swarm_size = this->file_to_swarm[file_name].get_size();
    MPI_Send(&swarm_size, 1, MPI_INT, client_idx, DOWNLOAD_TAG, MPI_COMM_WORLD);

    // Send the swarm.
    for (int seed : this->file_to_swarm[file_name].seeds) {
        MPI_Send(&seed, 1, MPI_INT, client_idx, DOWNLOAD_TAG, MPI_COMM_WORLD);
    }

    for (int peer : this->file_to_swarm[file_name].peers) {
        MPI_Send(&peer, 1, MPI_INT, client_idx, DOWNLOAD_TAG, MPI_COMM_WORLD);
    }
}


void Tracker::send_file_segment_details_to_client(const std::string &file_name, int client_idx) {
    // Send the number of segments.
    int segment_cnt = file_database[file_name].size();
    MPI_Send(&segment_cnt, 1, MPI_INT, client_idx, DOWNLOAD_TAG, MPI_COMM_WORLD);

    for (const auto &segment : file_database[file_name]) {
        // Send segment hash.
        MPI_Send(segment.hash.c_str(), HASH_SIZE, MPI_CHAR, client_idx, DOWNLOAD_TAG, MPI_COMM_WORLD);

        // Send segment index.
        MPI_Send(&segment.index, 1, MPI_INT, client_idx, DOWNLOAD_TAG, MPI_COMM_WORLD);
    }
}


void Tracker::handle_update_swarm_request(int client_idx) {
    // Receive file name (including '\0').
    char buff[MAX_FILENAME + 1];
    MPI_Recv(buff, MAX_FILENAME + 1, MPI_CHAR, client_idx, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    string file_name(buff);

    send_file_swarm_to_client(file_name, client_idx);
}


void Tracker::handle_file_download_complete_from_client(int client_idx) {
    // Receive file name (including '\0').
    char buff[MAX_FILENAME + 1];
    MPI_Recv(buff, MAX_FILENAME + 1, MPI_CHAR, client_idx, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    string file_name(buff);

    // Mark the client as a seed for the file.
    this->file_to_swarm[file_name].mark_peer_as_seed(client_idx);
}


void Tracker::announce_all_clients_to_stop() {
    for (int client_idx = 1; client_idx < this->numtasks; client_idx++) {
        int msg = STOP;
        MPI_Send(&msg, 1, MPI_INT, client_idx, UPLOAD_TAG, MPI_COMM_WORLD);
    }
}
