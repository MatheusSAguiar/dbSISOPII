#ifndef __PACOTE_HPP__
#define __PACOTE_HPP__

#include <string>
#include <vector>
#include <iostream>
#include <math.h>
#include <string.h>

#include "Constantes.hpp"

using namespace std;

namespace dropbox {

/*
*   Struct representing a Packet send on Sockets
*/
struct _Pacote
{
    int type;           // See constants.hpp to check Type
    int seq;            // Sequence number 1...*
    int totalSize;      // Total number of packets that are going to be sent, used to check if it is the last packet
    int len;            // Lenght of the payload
    int socketSeq;      // Socket sequence
    char payload[MESSAGE_LEN];  // Bytes sent
    char username[30];
};

typedef _Pacote Pacote;

/**
 *  Packet constructor.
 *  IMPORTANT:
 *  if type == TYPE_DATA or type == TYPE_SEND_FILE
 *  it will be used memcpy to copy the bytes,
 *  otherwise it will be used strcpy.
 */
Pacote make_packet(int type, int seq, int totalSize, int len, const char *payload);
Pacote make_packet(int type, int seq, int totalSize, int len, const char *payload, const char *username);
void set_socketSeq(Pacote * data, int socketSeq);

}

#endif