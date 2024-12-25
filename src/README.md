*Designed by Marius-Tudor Zaharia, 333CA, December 2024*

# BitTorrent

---

## Table of contents
1. [Overview](#overview)
2. [Running](#running)
3. [General details](#general-details)
4. [Client](#client)
    * [Flow](#flow)
    * [Download thread](#download-thread)
    * [Upload thread](#upload-thread)
    * [Implementation details](#implementation-details)
5. [Tracker](#tracker)

---

## Overview
* This is an implementation of the BitTorrent file transfer protocol.
* It is written in C++, using MPI for inter-process communication.

---

## Running
* To compile the project, `make` should be run from `src`, generating an
executable called `tema2`.
* The running command is `mpirun -np <N> ./tema2`, where N is the number of
MPI tasks that will be started. Task #0 will be the tracker, while the others
will be clients.

---

## General details
* In this implementation, a `segment` is defined by an `index` and a `hash`,
thus uniquely identifying each one. A `Segment` struct is used for managing
this metadata of a segment.
* It is more of a simulation of the protocol, so there will be no files sent
and received. A client asks a peer for a certain segment and receives an `ACK`
message, instead of the actual segment.
* For each file from the network, there is a group of clients that own segments
of it, called the `swarm` of the file. A `Swarm` class is used to group the
`seeds` (clients that own all the segments) and the `peers` (clients that own
only some of the segments).

---

## Client
### Flow
* The first step of a client is to read a configuration file, from which it
gets its `owned files` and its `wanted files`.
    * `owned_files` are stored as a `map` with `file name` as key and a vector
      of `Segments` as value.
    * `wanted_files` are stored as a `set`, containing `file names`.
* It sends the owned files (i.e. name, segment count, and for each segment,
its index and hash) to the tracker. It waits for an `ACK` from the tracker,
signaling the start of the actual protocol.
* It then starts two threads, one for `downloading` files, and the other for
`uploading` to other clients.

### Download Thread
* For each wanted file, the client querries the tracker for its `swarm` and
`segments` metadata.
* It then starts querrying clients for each segment of that file. Because new
clients can join the `swarm` of a file during the algorithm, a `segment counter`
is kept. When 10 segments have been received, the tracker is querried again for
the updated `swarm` of the file, allowing for a more varied downloading process
among different swarm members of that file.
* For each segment, all clients from the swarm are querried, to find those who
own that segment. From these, the one with the minimum `load` (i.e. segments
sent up to that moment in time) is chosen, and the segment is received from it
(i.e. an `ACK` message).
* The client then adds the new segment to the `owned files` map, so it can now
send it to other clients that ask for it too.
* After all the segments of a file are downloaded, a message to the tracker is
sent, notifying that is has now become a `seed` of that file. The hashes of
that file are written in order to an output file.
* After all the files are downloaded, the tracker is again notified, and the
download thread is closed.

### Upload Thread
* It handles segment requests from clients and receives the `stop` signal from
the tracker.
* When it receives a querry asking if it owns a certain segment of a file,
iterates the owned segments of that file. If the querried segment is found, a
non-negative message representing the current `load` of the client is sent as
response. Else, a `NACK` response is sent.
* When it receives a querry asking for a certain segment (i.e. after confirming
the posession of that segment), sends an `ACK` and increments the `load`.

### Implementation details
* A `mutex` is used for the `owned_files` map, because it is shared between both
threads. After receiving a segment, `download` adds it to the list of its file,
but at the same time, `upload` can look for that exact segment, hence resulting
in a race condition.
* As an optimization, when reading the segments of a file, `upload` will only
`lock` the mutex if the file is among the `wanted files` of the client (the
`unordered_set` allows constant look-up times). Also, when writing to the output
files, `download` will not lock the mutex, because the file that is read has been
completely downloaded.

---

## Tracker
* It first receives, from the clients, the files from the network and the details
of their segments. Then, sends an `ACK` to each client to start the algorithm.
* Holds a map linking each `file` to its `swarm`, but at no time does it know
which client owns which segment.
* Holds a map linking each `file` to its vector of `Segments` (i.e. hash and
index), but at no time does it know the actual content of a file.
* When receiving a querry asking for the details of a file, it sends the `swarm`
and the `segments` details of that file.
* When receiving a message that a client fully downloaded a file, marks it as a
`seed` for that file.
* When receiving a message that a client downloaded all its wanted files,
increments a counter to know when the algorithm should stop.
* When that counter equals the number of clients from the network, notifies all
of them that they should `stop`.
