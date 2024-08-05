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
            Servidor(string ipLocal, string ipMain, vector<string> backups);


            ~Servidor();

            void run();

            const string dirRaiz = "/tmp/DropboxSISOPII/";
        
        private:
        
        mutex electionMutex;
        mutex propagationMutex;
        mutex portsMutex;
        mutex backupList;
        mutex talkToPrimaryMtx;

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
    
        /**
         * Used to make a connection between a backup and the primary server
         */
        void makeConnection();

        /**
         *  Runs on a separate thread to listen to propagations from the primary and election messages from others backups
         */
        void listenToServers(FuncoesSocket * socket);
        
        /**
         * The primary server propates a new device connection to backups
         */
        void propagateConnection(string username, string userIp);

        /**
         * The new primary server sends a message to all front ends to let them know the ip of the new primary server
         */
        void propagateNewBoss();

        /**
         * The primary server propagates the file to all backups
         */
        void propagateFile(string filename, string username);

        /**
         * The primary server tells all backups to delete the file
         */
        void propagateDelete(string filename, string username);

        /**
         * Used during election to send a message a ANSWER message on Bully Algorithm
         */
        void answer(string ip);

        /**
         * The election winner informs all remaining backups and client devices that it is the new primary server
         */
        void becomeMain();

        /**
         * Return the IP's of all servers which the ip is greater than this server's IP
         */
        vector<string> getHighers();
    
        /**
         * The backup server starts the election
         */
        void startElection(vector<string> highers);

        /**
         * Sends a ELECTION message to all servers with IP greater than this server's IP
         */
        void sendHighersElection(vector<string> highers);

        /**
         * Wait for other backups answers after an ELECTION message
         */
        bool waitForAnswer();

        /**
         * Wait for other backup to win the election
         */
        bool waitForCoordinator();

        /**
         * Create a new thread and socket to listen to a backup server
         */
        void createNewPortBackup(FuncoesSocket * socket);

        /**
         * Remove from the backup list the primary server after he is killed
         */
        void removeFromBackup(vector<string> backups, string main);

        /**
         * The backup connects to the new primary server
         */
        void handleReceiveCoordinator(Pacote * res);
    };

}

void synchronize();
#endif