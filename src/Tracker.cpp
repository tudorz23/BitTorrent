#include "Tracker.h"

#include <mpi.h>
#include <iostream>


Tracker::Tracker(int numtasks, int rank) {
    this->numtasks = numtasks;
    this->rank = rank;
}


void Tracker::run() {
    printf("Tracker is running\n");
}
