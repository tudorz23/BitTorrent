#ifndef TRACKER_H
#define TRACKER_H


class Tracker {
    int numtasks;
    int rank;


 public:
    Tracker(int numtasks, int rank);


    void run();


};




#endif
