#include "Client.h"

#include <mpi.h>
#include <fstream>
#include <limits.h>
#include "constants.h"

#include <iostream>

using namespace std;


Client::Client(int numtasks, int rank) {
    this->numtasks = numtasks;
    this->rank = rank;
    this->load = 0;

    pthread_mutex_init(&owned_files_mutex, NULL);
}


Client::~Client() {
    pthread_mutex_destroy(&owned_files_mutex);
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

    #ifdef DEBUG
    cout << "Client with rank <" << rank << "> finished.\n";
    #endif
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
    MPI_Recv(&msg, 1, MPI_INT, TRACKER_RANK, INIT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

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
        vector<Segment> segments;
        client->receive_file_details_from_tracker(wanted_file, swarm, segments);

        #ifdef DEBUG
        client->print_swarm_for_file(wanted_file, swarm);
        client->print_segment_details_for_file(wanted_file, segments);
        #endif

        int counter = 0;

        // Ask peers for segments.
        for (auto segment : segments) {
            counter++;
            if (counter == 10) {
                counter = 0;
                client->update_swarm_from_tracker(wanted_file, swarm);
            }

            // Get the peer from the swarm that owns the segment and has minimum load.
            int peer = client->get_peer_with_min_load_for_segment(wanted_file, segment.index, swarm);

            // Send "Hello" message to that peer, initialising a GET_SEGMENT communication.
            int msg = GET_SEGMENT_REQ;
            MPI_Send(&msg, 1, MPI_INT, peer, UPLOAD_TAG, MPI_COMM_WORLD);

            // Send file name (including '\0').
            MPI_Send(wanted_file.c_str(), wanted_file.size() + 1, MPI_CHAR, peer, UPLOAD_TAG, MPI_COMM_WORLD);

            // Send segment index.
            MPI_Send(&segment.index, 1, MPI_INT, peer, UPLOAD_TAG, MPI_COMM_WORLD);

            // Receive response (simulate the receival of the segment).
            int response;
            MPI_Recv(&response, 1, MPI_INT, peer, DOWNLOAD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Add the "newly received" segment to the owned list.
            pthread_mutex_lock(&client->owned_files_mutex);
            client->owned_files[wanted_file].emplace_back(segment.hash, segment.index);
            pthread_mutex_unlock(&client->owned_files_mutex);
        }

        client->announce_tracker_whole_file_received(wanted_file);

        client->save_file(wanted_file);
    }

    client->announce_tracker_all_files_received();

    return NULL;
}


void Client::receive_file_details_from_tracker(std::string &wanted_file, std::vector<int> &swarm,
                                               std::vector<Segment> &segments) {
    // Send "Hello" message to the tracker, initialising a FILE_DETAILS communication.
    int msg = FILE_DETAILS_REQ;
    MPI_Send(&msg, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);

    // Send the name of the file to the tracker.
    MPI_Send(wanted_file.c_str(), wanted_file.size() + 1, MPI_CHAR, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);

    receive_file_swarm_from_tracker(swarm);
    receive_file_segment_details_from_tracker(segments);
}


void Client::receive_file_swarm_from_tracker(std::vector<int> &swarm) {
    // Receive the size of the swarm.
    int swarm_size;
    MPI_Recv(&swarm_size, 1, MPI_INT, TRACKER_RANK, DOWNLOAD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Receive the swarm.
    for (int i = 0; i < swarm_size; i++) {
        int client_id;
        MPI_Recv(&client_id, 1, MPI_INT, TRACKER_RANK, DOWNLOAD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        swarm.push_back(client_id);
    }
}


void Client::receive_file_segment_details_from_tracker(std::vector<Segment> &segments) {
    // Receive the number of segments.
    int segment_cnt;
    MPI_Recv(&segment_cnt, 1, MPI_INT, TRACKER_RANK, DOWNLOAD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Receive segment details.
    for (int i = 0; i < segment_cnt; i++) {
        // Receive segment hash (add '\0' manually).
        char hash_buff[HASH_SIZE + 1];
        MPI_Recv(hash_buff, HASH_SIZE, MPI_CHAR, TRACKER_RANK, DOWNLOAD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        hash_buff[HASH_SIZE] = '\0';
        string hash(hash_buff);

        // Receive segment index.
        int idx;
        MPI_Recv(&idx, 1, MPI_INT, TRACKER_RANK, DOWNLOAD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        segments.emplace_back(hash, idx);
    }
}


void Client::update_swarm_from_tracker(std::string &wanted_file, std::vector<int> &swarm) {
    swarm.clear();

    // Send "Hello" message to the tracker, initialising an UPDATE_SWARM communication.
    int msg = UPDATE_SWARM_REQ;
    MPI_Send(&msg, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);

    // Send the name of the file to the tracker.
    MPI_Send(wanted_file.c_str(), wanted_file.size() + 1, MPI_CHAR, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);

    receive_file_swarm_from_tracker(swarm);
}


int Client::get_peer_with_min_load_for_segment(std::string &file, int segment_idx,
                                               std::vector<int> &swarm) {
    int min_load = INT_MAX;
    int peer_with_min_load = -1;

    for (int peer : swarm) {
        // Send "Hello" message to peer, initialising a HAS_SEGMENT communication.
        int msg = HAS_SEGMENT_REQ;
        MPI_Send(&msg, 1, MPI_INT, peer, UPLOAD_TAG, MPI_COMM_WORLD);

        // Send file name (including '\0').
        MPI_Send(file.c_str(), file.size() + 1, MPI_CHAR, peer, UPLOAD_TAG, MPI_COMM_WORLD);

        // Send segment index.
        MPI_Send(&segment_idx, 1, MPI_INT, peer, UPLOAD_TAG, MPI_COMM_WORLD);

        // Receive response.
        int response;
        MPI_Recv(&response, 1, MPI_INT, peer, DOWNLOAD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (response == NACK) {
            // Peer does not own this segment.
            continue;
        }

        // If response is not NACK, then it represents the load of the peer.
        if (response == 0) {
            return peer;
        }

        if (response < min_load) {
            min_load = response;
            peer_with_min_load = peer;
        }
    }

    return peer_with_min_load;
}


void *upload_thread_func(void *arg) {
    Client *client = (Client*) arg;

    #ifdef DEBUG
    printf("Client with rank <%d> started upload thread.\n", client->rank);
    #endif

    bool should_stop = false;

    while (true) {
        MPI_Status status;
        int msg;

        // Receive "Hello" message.
        MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, UPLOAD_TAG, MPI_COMM_WORLD, &status);

        switch (msg) {
            case HAS_SEGMENT_REQ:
                client->handle_has_segment_req_from_peer(status.MPI_SOURCE);
                break;

            case GET_SEGMENT_REQ:
                client->handle_get_segment_req_from_peer(status.MPI_SOURCE);
                break;

            case STOP:
                should_stop = true;
                break;
        }

        if (should_stop) {
            break;
        }
    }


    return NULL;
}


void Client::handle_has_segment_req_from_peer(int peer_idx) {
    // Receive file name (including '\0').
    char buff[MAX_FILENAME + 1];
    MPI_Recv(buff, MAX_FILENAME + 1, MPI_CHAR, peer_idx, UPLOAD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    string file_name(buff);

    // Receive segment index.
    int segment_idx;
    MPI_Recv(&segment_idx, 1, MPI_INT, peer_idx, UPLOAD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Check if that segment is owned by the client.
    pthread_mutex_lock(&this->owned_files_mutex);
    for (const auto &segment : owned_files[file_name]) {
        if (segment.index == segment_idx) {
            // Send ACK message back to the peer, by sending the load of the client.
            int response = this->load;
            MPI_Send(&response, 1, MPI_INT, peer_idx, DOWNLOAD_TAG, MPI_COMM_WORLD);

            pthread_mutex_unlock(&this->owned_files_mutex);
            return;
        }
    }

    pthread_mutex_unlock(&this->owned_files_mutex);

    // Send NACK message back to the peer.
    int response = NACK;
    MPI_Send(&response, 1, MPI_INT, peer_idx, DOWNLOAD_TAG, MPI_COMM_WORLD);
}


void Client::handle_get_segment_req_from_peer(int peer_idx) {
    // Receive file name (including '\0').
    char buff[MAX_FILENAME + 1];
    MPI_Recv(buff, MAX_FILENAME + 1, MPI_CHAR, peer_idx, UPLOAD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    string file_name(buff);

    // Receive segment index.
    int segment_idx;
    MPI_Recv(&segment_idx, 1, MPI_INT, peer_idx, UPLOAD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Send response to the peer (simulate the sending of the segment).
    int response = ACK;
    MPI_Send(&response, 1, MPI_INT, peer_idx, DOWNLOAD_TAG, MPI_COMM_WORLD);
}


void Client::announce_tracker_whole_file_received(std::string &file) {
    // Send "Hello" message to the tracker, initialising a FILE_DOWNLOAD_COMPLETE communication.
    int msg = FILE_DOWNLOAD_COMPLETE;
    MPI_Send(&msg, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);

    // Send the name of the file to the tracker.
    MPI_Send(file.c_str(), file.size() + 1, MPI_CHAR, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);
}


void Client::save_file(std::string &file) {
    ofstream out_file("client" + to_string(this->rank) + "_" + file);

    vector<Segment>::iterator it = this->owned_files[file].begin();

    while (it != this->owned_files[file].end()) {
        out_file << it->hash;

        it++;
        if (it != this->owned_files[file].end()) {
            out_file << "\n";
        }
    }

    out_file.close();
}


void Client::announce_tracker_all_files_received() {
    // Send "Hello" message to the tracker, initialising an ALL_FILES_RECEIVED communication.
    int msg = ALL_FILES_RECEIVED;
    MPI_Send(&msg, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);
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


void Client::print_segment_details_for_file(std::string &file, std::vector<Segment> &segments) {
    ofstream fout;
    fout.open("client<" + to_string(rank) + ">.debug", std::fstream::app);

    fout << "Segments for filename <" << file << ">:\n";
    for (const auto &segment : segments) {
        fout << "Segment hash: " << segment.hash << ", index: " << segment.index << "\n";
    }

    fout << "\n";
}
