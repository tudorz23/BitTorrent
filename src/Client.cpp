#include "Client.h"

#include <mpi.h>
#include <pthread.h>
#include <fstream>

#include <iostream>

#include "constants.h"


using namespace std;


Client::Client(int numtasks, int rank) {
    this->numtasks = numtasks;
    this->rank = rank;
}


void Client::run() {
    initialize();

    pthread_t download_thread;
    pthread_t upload_thread;
    void *status;
    int r;

    r = pthread_create(&download_thread, NULL, download_thread_func, (void *) this);
    if (r) {
        printf("Eroare la crearea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_create(&upload_thread, NULL, upload_thread_func, (void *) this);
    if (r) {
        printf("Eroare la crearea thread-ului de upload\n");
        exit(-1);
    }

    r = pthread_join(download_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_join(upload_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de upload\n");
        exit(-1);
    }

}


void Client::initialize() {
    read_input_file();

    #ifdef DEBUG
    print_files_after_read();
    printf("Client with rank <%d> initialized.\n", rank);
    #endif

    send_owned_files_to_tracker();

    // Wait for ACK from the tracker.
    int msg;
    MPI_Recv(&msg, 1, MPI_INT, TRACKER_RANK, READY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    if (msg != ACK) {
        cerr << "Did not receive ACK from the tracker.\n";
        exit(-1);
    }
}


void Client::read_input_file() {
    string in_file_name = "in" + to_string(this->rank) + ".txt";
    ifstream input_file(in_file_name);

    // Read owned files.
    int owned_files_cnt;
    input_file >> owned_files_cnt;

    for (int i = 0; i < owned_files_cnt; i++) {
        string file_name;
        int segment_cnt;

        input_file >> file_name;
        input_file >> segment_cnt;

        for (int idx = 0; idx < segment_cnt; idx++) {
            string segment_hash;
            input_file >> segment_hash;

            this->owned_files[file_name].emplace_back(segment_hash, idx);
        }
    }

    // Read wanted files.
    int wanted_files_cnt;
    input_file >> wanted_files_cnt;

    for (int i = 0; i < wanted_files_cnt; i++) {
        string file_name;
        input_file >> file_name;

        this->wanted_files.push_back(file_name);
    }

    input_file.close();
}


void Client::send_owned_files_to_tracker() {
    // Send the files count.
    int owned_files_cnt = owned_files.size();
    MPI_Send(&owned_files_cnt, 1, MPI_INT, TRACKER_RANK, INIT_TAG, MPI_COMM_WORLD);

    for (const auto &[file, segments] : owned_files) {
        // Send file name (including '\0').
        MPI_Send(file.c_str(), file.size() + 1, MPI_CHAR, TRACKER_RANK, INIT_TAG, MPI_COMM_WORLD);

        // Send segment count.
        int segments_cnt = segments.size();
        MPI_Send(&segments_cnt, 1, MPI_INT, TRACKER_RANK, INIT_TAG, MPI_COMM_WORLD);

        for (const auto &segment : segments) {
            // Send segment hash.
            MPI_Send(segment.hash.c_str(), HASH_SIZE, MPI_CHAR, TRACKER_RANK, INIT_TAG, MPI_COMM_WORLD);

            // Send segment index.
            MPI_Send(&segment.index, 1, MPI_INT, TRACKER_RANK, INIT_TAG, MPI_COMM_WORLD);
        }
    }
}


void *download_thread_func(void *arg) {
    Client *client = (Client*) arg;

    #ifdef DEBUG
    printf("Client with rank <%d> started download thread.\n", client->rank);
    #endif


    for (auto &wanted_file : client->wanted_files) {
        vector<int> swarm;

        client->receive_file_swarm(wanted_file, swarm);

        #ifdef DEBUG
        client->print_swarm_for_file(wanted_file, swarm);
        #endif
    }



    return NULL;
}


void Client::receive_file_swarm(std::string &wanted_file, std::vector<int> &swarm) {
    // Send HELO message to the tracker, with SWARM_REQ_TAG.
    int msg = HELO;
    MPI_Send(&msg, 1, MPI_INT, TRACKER_RANK, SWARM_REQ_TAG, MPI_COMM_WORLD);

    // Ask the tracker for the swarm of wanted_file.
    MPI_Send(wanted_file.c_str(), wanted_file.size() + 1, MPI_CHAR, TRACKER_RANK, SWARM_REQ_TAG, MPI_COMM_WORLD);

    // Receive the size of the swarm.
    int swarm_size;
    MPI_Recv(&swarm_size, 1, MPI_INT, TRACKER_RANK, SWARM_REQ_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Receive the swarm.
    for (int i = 0; i < swarm_size; i++) {
        int client_id;
        MPI_Recv(&client_id, 1, MPI_INT, TRACKER_RANK, SWARM_REQ_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        swarm.push_back(client_id);
    }
}


void *upload_thread_func(void *arg) {
    Client *client = (Client*) arg;

    #ifdef DEBUG
    printf("Client with rank <%d> started upload thread.\n", client->rank);
    #endif

    return NULL;
}



// DEBUG METHODS //

void Client::print_files_after_read() {
    ofstream fout("client<" + to_string(rank) + ">.debug");

    fout << "Owned files cnt: " << owned_files.size() << "\n";
    for (const auto &[file, segments] : owned_files) {
        fout << "FileName: " << file << "\n";
        for (const auto &segment : segments) {
            fout << "Segment hash: " << segment.hash << ", segment idx: " << segment.index << "\n";
        }
        fout << "\n";
    }

    fout << "\n";

    fout << "Wanted files cnt: " << wanted_files.size() << "\n";
    for (const auto &file : wanted_files) {
        fout << "Filename: " << file << "\n";
    }

    fout << "\n";

    fout.close();
}


void Client::print_swarm_for_file(std::string &file, std::vector<int> &swarm) {
    ofstream fout;
    fout.open("client<" + to_string(rank) + ">.debug", std::fstream::app);

    fout << "Swarm for filename <" << file << ">:\n";
    for (int peer : swarm) {
        fout << peer << "\n";
    }

    fout << "\n";
}
