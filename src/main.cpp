#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "Tracker.h"
#include "Client.h"
#include "constants.h"


int main (int argc, char *argv[]) {
    int numtasks, rank;

    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI nu are suport pentru multi-threading\n");
        exit(-1);
    }
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == TRACKER_RANK) {
        Tracker *tracker = new Tracker(numtasks, rank);
        tracker->run();
        delete tracker;
    } else {
        Client *client = new Client(numtasks, rank);
        client->run();
        delete client;
    }

    MPI_Finalize();
}
