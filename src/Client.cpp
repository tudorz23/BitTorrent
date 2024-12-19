#include "Client.h"

#include <mpi.h>
#include <pthread.h>
#include <iostream>


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

    r = pthread_create(&download_thread, NULL, download_thread_func, (void *) &rank);
    if (r) {
        printf("Eroare la crearea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_create(&upload_thread, NULL, upload_thread_func, (void *) &rank);
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
    printf("Client with rank <%d> initialized.\n", rank);
}




void *download_thread_func(void *arg) {
    int rank = *(int*) arg;

    printf("Client with rank <%d> started download thread.\n", rank);

    return NULL;
}


void *upload_thread_func(void *arg) {
    int rank = *(int*) arg;

    printf("Client with rank <%d> started upload thread.\n", rank);

    return NULL;
}
