#ifndef __FUNCOES_SOCKET_HPP__
#define __FUNCOES_SOCKET_HPP__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <chrono>
#include <poll.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "Constantes.hpp"
#include "Pacote.hpp"

using namespace std;

namespace dropbox {

class FuncoesSocket
{
  public:

   // Construtor do cliente conecta a um host
    FuncoesSocket(string host, int port);

    // Construtor do servidor conecta a uma porta
    FuncoesSocket(int port);
    FuncoesSocket();
    ~FuncoesSocket();

   // Manda um pacote
    int send(Pacote *packet);
    bool send(Pacote *packet, int wait);

    // Bind no socket com a porta
    void bindSocket(int port);

   // Recebe um pacote
    Pacote *receive(int timeout);
    Pacote *receive(int timeout, int time);
    int getPortInt();

  private:
    int localSocketHandler;
    int portInt;
    int socketSeq;
    Pacote * lastData;
    hostent *Servidor;
    sockaddr_in remoteSocketAddr;
    sockaddr_in localSocketAddr;
    socklen_t remoteSocketLen;
    void sendAck(Pacote *ack);
    bool waitAck(int seq);
    bool waitAck(int seq, int wait);
    int getPort() { return portInt; }
    bool isEqual(Pacote * a, Pacote * b);

};

}

#endif