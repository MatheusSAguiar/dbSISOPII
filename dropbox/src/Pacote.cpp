#include "../include/Pacote.hpp"

using namespace std;
using namespace dropbox;

Pacote dropbox::make_packet(int type, int seq, int totalSize, int len, const char *payload){
    Pacote data;
    data.type = type;
    data.seq = seq;
    data.totalSize = totalSize;
    data.len = len;
    data.socketSeq = -1;
    // File or record packets can't have \0 at the end of the payload
    if(type == TYPE_DATA || type == TYPE_SEND_FILE)
        memcpy(data.payload, payload, len);
    else
        strncpy(data.payload, payload, MESSAGE_LEN);

    return data;
}

Pacote dropbox::make_packet(int type, int seq, int totalSize, int len, const char *payload, const char *username){
    Pacote data;
    data.type = type;
    data.seq = seq;
    data.totalSize = totalSize;
    data.len = len;
    data.socketSeq = -1;
    // File or record packets can't have \0 at the end of the payload
    if(type == TYPE_DATA || type == TYPE_SEND_FILE)
        memcpy(data.payload, payload, len);
    else
        strncpy(data.payload, payload, MESSAGE_LEN);
    
    strncpy(data.username, username, 30);

    return data;
}

void dropbox::set_socketSeq(Pacote * data, int socketSeq) {
    data->socketSeq = socketSeq;
}