#ifndef CLIENT_H
#define CLIENT_H


class Client {
    int numtasks;
    int rank;


 public:
    Client(int numtasks, int rank);

    void run();

    void initialize();
};


void *download_thread_func(void *arg);


void *upload_thread_func(void *arg);


#endif
