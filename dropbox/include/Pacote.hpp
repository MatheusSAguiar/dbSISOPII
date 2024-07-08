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

// Struct do pacote que é enviado nos sockets
struct _Pacote
{
    int type;           // Tipo do pacote, definido em Constantes.hpp
    int seq;            // Numero de sequencia
    int totalSize;      // Numero total de pacotes, usado com o seq para ver se é o último
    int len;            // Tamanho da mensagem do pacote
    int socketSeq;      // Sequencia de sockets
    char payload[MESSAGE_LEN];  // Bytes enviados
    char username[30];
};

typedef _Pacote Pacote;


Pacote make_packet(int type, int seq, int totalSize, int len, const char *payload);
Pacote make_packet(int type, int seq, int totalSize, int len, const char *payload, const char *username);
void set_socketSeq(Pacote * data, int socketSeq);

}

#endif