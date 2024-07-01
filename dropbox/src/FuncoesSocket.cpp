#include "../include/FuncoesSocket.hpp"
#include "../include/Pacote.hpp"

#define ERROR -1

using namespace std;
using namespace dropbox;

FuncoesSocket::FuncoesSocket(){}

FuncoesSocket::FuncoesSocket (string host, int port) {
  this->localSocketHandler = socket(AF_INET, SOCK_DGRAM, 0);
  this->lastData = NULL;
  if (this->localSocketHandler == ERROR) {
    fprintf(stderr, "Error on creating socket");
    exit(1);
  }
  this->socketSeq = 0;
  this->Servidor = gethostbyname(host.c_str());
  this->remoteSocketAddr.sin_family = AF_INET;
  this->remoteSocketAddr.sin_port = htons(port);
  this->remoteSocketAddr.sin_addr = *((struct in_addr *)this->Servidor->h_addr);
  bzero(&(this->remoteSocketAddr.sin_zero), 8);
  this->remoteSocketLen = sizeof(struct sockaddr_in);

  this->portInt = port;
}

FuncoesSocket::~FuncoesSocket(){
  close(this->localSocketHandler);
}

FuncoesSocket::FuncoesSocket (int port) {
  this->socketSeq = 0;
  this->lastData = NULL;
  this->localSocketHandler = socket(AF_INET, SOCK_DGRAM, 0);
  if (this->localSocketHandler == ERROR) {
    fprintf(stderr, "Error on creating socket");
    exit(1);
  }
  this->bindSocket(port);
  this->remoteSocketLen = sizeof(struct sockaddr_in);

  this->portInt = port;
}

bool FuncoesSocket::send(Pacote *packet, int wait) {
  set_socketSeq(packet, this->socketSeq);
  int tries = 0;
  do{

    if (sendto(this->localSocketHandler, (void *)packet, PACKET_LEN, 0,(const struct sockaddr *) &(this->remoteSocketAddr),sizeof(struct sockaddr_in)) < 0) {
      fprintf(stderr, "Error on sending");
      cout << string(packet->payload) << packet->type << " " << string(packet->username) << endl;
      exit(1);
    }
  
  }while(!this->waitAck(packet->seq, wait) && ++tries < TOTAL_TRIES);
  this->socketSeq++;
  
  if(tries == TOTAL_TRIES) return false;
  return true;
}

bool FuncoesSocket::waitAck(int seq, int wait) {
  bool acked = false;
  while(!acked) {
    Pacote *response = this->receive(TIMEOUT_ON, wait);
    if(!response) {
      return false;
    } 
    if(response->type == TYPE_ACK && response->seq == seq) { 
      return true;
    }
  }
  return false;
}

int FuncoesSocket::send(Pacote *packet) {
  set_socketSeq(packet, this->socketSeq);
  int tries = 0;
  do{

    if (sendto(this->localSocketHandler, (void *)packet, PACKET_LEN, 0,(const struct sockaddr *) &(this->remoteSocketAddr),sizeof(struct sockaddr_in)) < 0) {
      fprintf(stderr, "Error on sending");
      cout << string(packet->payload) << packet->type << " " << string(packet->username) << endl;
      return -1;
    }
  
  }while(!this->waitAck(packet->seq) && ++tries < TOTAL_TRIES);
  this->socketSeq++;
  
  if(tries == TOTAL_TRIES) return 0;
  return 1;
}

bool FuncoesSocket::waitAck(int seq) {
  bool acked = false;
  while(!acked) {
    Pacote *response = this->receive(TIMEOUT_ON);
    if(!response) {
      return false;
    } 
    if(response->type == TYPE_ACK && response->seq == seq) { 
      return true;
    }
  }
  return false;
}

bool FuncoesSocket::isEqual(Pacote * a, Pacote * b) {
  if (a->type != b->type || a->seq != b->seq || a->totalSize != b->totalSize)
    return false;
  if (a->len != b->len || a->socketSeq != b->socketSeq)
    return false;
  if (string(a->payload).compare(string(b->payload)) != 0)
    return false;
  if (string(a->username).compare(string(b->username)) != 0)
    return false;
  return true;
}

Pacote* FuncoesSocket::receive(int timeout) {
  if(timeout == TIMEOUT_ON) {
    struct pollfd fd;
    fd.fd = this->localSocketHandler;
    fd.events = POLLIN;
    int ret = poll(&fd, 1, 2000);
    switch(ret) {
      case -1:
        return NULL;
      case 0: printf("Timeout\n");
        return NULL;
      default: break;
    }
  }

  char* msg = new char[PACKET_LEN];
  memset(msg, 0, PACKET_LEN);
  bool receivedCorrectly = false;
  Pacote * data;
  while(!receivedCorrectly) {

    int msgSize = recvfrom(this->localSocketHandler, (void *) msg, 
      PACKET_LEN, 0, (struct sockaddr *) &this->remoteSocketAddr, &this->remoteSocketLen);
    if (msgSize < 0) {
      fprintf(stderr, "Error on receiving\n");
      exit(1);
    }

    data = (Pacote *) msg;

    if (data->type != TYPE_ACK && this->lastData != NULL && this->isEqual(this->lastData, data))
      continue;
    if (data->socketSeq == -1)
      break;
    if (data->socketSeq == 0)
      this->socketSeq = 0;
    if (data->socketSeq == this->socketSeq && data->type != TYPE_ACK) {
      Pacote message = make_packet(TYPE_ACK, data->seq, 1, -1, "");
      this->sendAck(&message);
      this->socketSeq = data->socketSeq + 1;
      break;
    }
  }

  this->lastData = data;
  // TODO delete msg
  return data;
}

Pacote* FuncoesSocket::receive(int timeout, int time) {
  if(timeout == TIMEOUT_ON) {
    struct pollfd fd;
    fd.fd = this->localSocketHandler;
    fd.events = POLLIN;
    int ret = poll(&fd, 1, time);
    switch(ret) {
      case -1: printf("Error\n");
        return NULL;
      case 0: 
        return NULL;
      default: break;
    }
  }

  char* msg = new char[PACKET_LEN];
  memset(msg, 0, PACKET_LEN);
  bool receivedCorrectly = false;
  Pacote * data;
  while(!receivedCorrectly) {

    int msgSize = recvfrom(this->localSocketHandler, (void *) msg, 
      PACKET_LEN, 0, (struct sockaddr *) &this->remoteSocketAddr, &this->remoteSocketLen);
    if (msgSize < 0) {
      fprintf(stderr, "Error on receiving\n");
      exit(1);
    }
    data = (Pacote *) msg;

    if (data->socketSeq == -1)
      break;
    if (data->socketSeq == 0)
      this->socketSeq = 0;
    if (data->socketSeq == this->socketSeq && data->type != TYPE_ACK) {
      Pacote message = make_packet(TYPE_ACK, data->seq, 1, -1, "");
      this->sendAck(&message);
      this->socketSeq = data->socketSeq + 1;
      receivedCorrectly = true;
    }
  }

  
  // TODO delete msg
  return data;
}

void FuncoesSocket::sendAck(Pacote *ack) {
  if (sendto(this->localSocketHandler, (void *)ack, PACKET_LEN, 0,(const struct sockaddr *) &(this->remoteSocketAddr), sizeof(struct sockaddr_in)) < 0) {
    fprintf(stderr, "Error on sending ACK");
    exit(1);
  }
}

void FuncoesSocket::bindSocket(int port) {
  this->remoteSocketAddr.sin_family = AF_INET;
	this->remoteSocketAddr.sin_port = htons(port);
	this->remoteSocketAddr.sin_addr.s_addr = INADDR_ANY;
  bzero(&(this->remoteSocketAddr.sin_zero), 8);
  if (bind(this->localSocketHandler, (struct sockaddr *) &this->remoteSocketAddr, sizeof(struct sockaddr)) < 0) {
    fprintf(stderr, "Error on binding");
    exit(1);
  }
}

int FuncoesSocket::getPortInt(){
  return this->portInt;
}