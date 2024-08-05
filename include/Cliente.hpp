#ifndef __CLIENT_HPP_
#define __CLIENT_HPP_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/stat.h>
#include <utime.h>
#include <dirent.h>
#include <iomanip>
#include <thread>
#include <mutex>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <limits.h>

#include "FuncoesSocket.hpp"
#include "Pacote.hpp"
#include "Constantes.hpp"
#include "RegistroDeArquivos.hpp"
#include "Operacoes.hpp"

/*
    Classe que representa a parte do cliente.
    A pasta é criada em /tmp/sync_dir_<username>
*/
namespace dropbox
{
    class Cliente : public Operacoes
    {

        private:
            string username;
            string localIp;
            dropbox::FuncoesSocket *socket;
            dropbox::FuncoesSocket *listenToMaster;
            string syncDirPath;
            mutex mtx;
            mutex socketMtx;
            vector<RegistroDeArquivos> fileRecords;

            // Esse método trata o inotify em outra thread
            void askServidorUpdates();
            
            // Cria a pasta sincronizada em /tmp/sync_dir_<username>
            void createSyncDir();

        public:
            Cliente(string username, string ServidorAddr, int ServidorDistributorPort, string localIp);
            ~Cliente();

            string getUsername() { return username; }

            // Manda um pacote TYPE_REQUEST_UPDATE e sincroniza o diretório local com o do servidor
			string payload1;
			int payload2;
            void askUpdate();

            // Quando o inotify detectar eventos, eles são processados aqui
            void eventsInotify(int* fd);

            // Coloca o watch do inotify no diretório local
            void initializeInotify(int *fd, int *wd);

            
            // Baixar arquivo do servidor
            void download(string filename);

            // Printa os arquivos do cliente
            void list_client();

            // Deleta os arquivos da pasta local e baixa do servidor
            void get_sync_dir();

            // Finaliza a sessão
            void exit();

            // Printa a lista de arquivos do servidor
            void requestServerFileList();

            // Remove um arquivo do registro
            void removeFileRecord(string filename);

            // Atualiza o registro de arquivos
            void updateFileRecord(RegistroDeArquivos newFile);
            
            // Retorna o socket do cliente
            dropbox::FuncoesSocket * getSocket() { return socket; }

            // Retorna o caminho da pasta de sync
            string getSyncDirPath() { return syncDirPath; }
            
            // Mutex
            void lockMutex() { this->mtx.lock(); };
            void unlockMutex() { this->mtx.unlock(); };

            /**
             * Keep listening on port 10000 for a primary server change
             */
            void listenMaster();
    };
}



#endif