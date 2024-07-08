#ifndef __Servidor_HPP__
#define __Servidor_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <iostream>
#include <thread>
#include <chrono>
#include <poll.h>
#include <arpa/inet.h>
#include <mutex>
#include <fstream>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "Operacoes.hpp"
#include "RegistroDeArquivos.hpp"
#include "FuncoesSocket.hpp"
#include "Constantes.hpp"
#include "Pacote.hpp"
#include "Usuario.hpp"


using namespace std;

namespace dropbox {
    class Servidor : public Operacoes
    {
        public:
            Servidor(string ipLocal);

            ~Servidor();

            void run();

            const string dirRaiz = "/tmp/DropboxSISOPII/";
        
        private:
        
        mutex portsMutex;

        FuncoesSocket connectClientSocket;
        FuncoesSocket listenToServersSocket;
        FuncoesSocket * talkToPrimary;
        int status;
        bool portsAvailable[LAST_PORT - FIRST_PORT + 1];
        vector<Usuario*> users;
        vector<string> backups;
        vector<string> usersIp;
        vector<FuncoesSocket*> backupsSockets;
        string ipLocal;
        string ipMain;
        bool isMain;


        // Pega uma porta disponivel
        int getAvailablePort();

        // Inicializa todas as portas como disponíveis
        void initializePorts();

        // Seta a porta como disponível
        void setPortAvailable(int port);

        // Cria um usuário para cada pasta no diretório raiz
        void initializeUsers();

        // Método que espera requests do cliente em outra thread
        void listenToClient(FuncoesSocket *socket, Usuario *user);

        // Recebe um pacote para checar se o arquivo está atualizado
        void receiveAskUpdate(FuncoesSocket * socket, Usuario * user);

        // Recusa o cliente se o limite de dispositivos foi alcançado
        void refuseOverLimitClient(Usuario *user);

        // Conecta um cliente em um novo socket
        void connectNewClient();

        // Retorna o usuário pelo username
        Usuario* getUser(string username);

        // Procura um arquivo no registro do servidor, se achar, remove para não enviar para o usuário de novo
        int lookForRecordAndRemove(RegistroDeArquivos file, vector<RegistroDeArquivos> *files, RegistroDeArquivos * updatedFile);

        // Atualiza arquivos do cliente faltando
        void updateClient(vector<RegistroDeArquivos> ServidorFiles, vector<RegistroDeArquivos> clientFiles, FuncoesSocket * socket, Usuario * user);

        // Fecha o socket de um usuario
        void exitUser(FuncoesSocket *socket, Usuario *user);

        // Manda o registro do arquivos do diretório do servidor pelo socket
        void sendFileRecord(FuncoesSocket * socket, string filename, Usuario * user);
    };

}

void synchronize();
#endif