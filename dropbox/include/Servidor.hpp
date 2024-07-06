#ifndef __MYServidor_HPP__
#define __MYServidor_HPP__

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


        /**
         *  Return a Port not beign used.
         *  Race condition is protected by portsMutex
         */
        int getAvailablePort();

        /**
         *  Set all port to available
         */
        void initializePorts();

        /**
         *  Set a port to available
         *  Race condition is protected by portsMutex
         */
        void setPortAvailable(int port);

        /**
         *  Check all directories on /tmp/DropboxService/ and create an User for each directory
         */
        void initializeUsers();

        /**
         *  This method is executed in another thread, listening to client requests
         */
        void listenToClient(FuncoesSocket *socket, Usuario *user);

        /**
         *  Receive a request to check if a file is outdated
         */
        void receiveAskUpdate(FuncoesSocket * socket, Usuario * user);

        /**
         *  Refuse a client if the number of devices being used by the client is bigger than MAX_DEVICES
         */
        void refuseOverLimitClient(Usuario *user);

        /**
         *  Connect a new Client on a new socket with a new port
         */
        void connectNewClient();

        /**
         *  Returns the User from username
         *  Returns nullptr if the User does not exist
         */
        Usuario* getUser(string username);

        /**
         *  Look for the FileRecord given on Servidor files and remove it from files if it doesn't need
         *  to be sent to client. If it needs to be updated the updatedFile parameter is updated.
         */
        int lookForRecordAndRemove(RegistroDeArquivos file, vector<RegistroDeArquivos> *files, RegistroDeArquivos * updatedFile);

        /**
         *  Update oudated or missing client files
         */
        void updateClient(vector<RegistroDeArquivos> ServidorFiles, vector<RegistroDeArquivos> clientFiles, FuncoesSocket * socket, Usuario * user);

        /** 
         *  Ends a client session, closing its socket
         */
        void exitUser(FuncoesSocket *socket, Usuario *user);
        
        /**
         * Sends the file's filerecord that's within the dirPath through the given socket
         */
        void sendFileRecord(FuncoesSocket * socket, string filename, Usuario * user);
    };

}

void synchronize();
#endif