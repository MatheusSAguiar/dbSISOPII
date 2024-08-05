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
    // Pacotes de dados não tem \0 no fim da mensagem, por isso o memcpy
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
    // Pacotes de dados não tem \0 no fim da mensagem, por isso o memcpy
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