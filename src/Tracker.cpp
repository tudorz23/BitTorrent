#include "Tracker.h"

#include <mpi.h>

#include <iostream>


#include "constants.h"

using namespace std;


Tracker::Tracker(int numtasks, int rank) {
    this->numtasks = numtasks;
    this->rank = rank;
}


void Tracker::run() {
    #ifdef DEBUG
    printf("Tracker is running\n");
    #endif

    initialize();
}


void Tracker::initialize() {
    for (int client_idx = 1; client_idx < numtasks; client_idx++) {
        recv_file_details_from_client(client_idx);
    }

    #ifdef DEBUG
    print_database_and_swarms();
    #endif
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

        // If the file is already in the database, don't store its segments again.
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


void Tracker::print_database_and_swarms() {
    ofstream fout("tracker.debug");

    for (const auto &[file, segments] : file_database) {
        fout << "Filename: " << file << "\n";

        for (const auto &segment : segments) {
            fout << "Segment hash: " << segment.hash << ", index: " << segment.index << "\n";
        }

        fout << "\n";
    }

    for (const auto &[file, swarm] : file_to_swarm) {
        fout << "Seeds for filename<" << file << ">:\n";

        for (const auto &seed : swarm.seeds) {
            fout << seed << "\n";
        }

        fout << "\n";
    }

    fout.close();
}
