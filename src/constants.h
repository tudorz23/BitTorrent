#ifndef CONSTANTS_H
#define CONSTANTS_H

#define TRACKER_RANK 0
#define MAX_FILES 10
#define MAX_FILENAME 15
#define HASH_SIZE 32
#define MAX_CHUNKS 100


// #define DEBUG

/*
 * Rule: For an MPI message, the tag is:
 *      -INIT_TAG -> for messages from the initialization stage
 *      -TRACKER_TAG -> for messages that have the tracker as destination
 *      -DOWNLOAD_TAG -> for messages that have a download thread of a client as destination
 *      -UPLOAD_TAG -> for messages that have an upload thread of a client as destination
 * 
 * Thus, there will be no risk of miscommunication if two threads execute
 * a Recv at the same time.
 */

#define INIT_TAG 1
#define TRACKER_TAG 2
#define DOWNLOAD_TAG 3
#define UPLOAD_TAG 4

#define ACK 42
#define NACK 43

#define FILE_DETAILS_REQ 10
#define UPDATE_SWARM_REQ 11
#define HAS_SEGMENT_REQ 12
#define GET_SEGMENT_REQ 13
#define FILE_DOWNLOAD_COMPLETE 14
#define ALL_FILES_RECEIVED 15
#define STOP 16


#endif /* CONSTANTS_H */
