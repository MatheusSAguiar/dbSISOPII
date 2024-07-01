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
*    Class representing the Client on the client-side of the System.
*    The sync directory will be created on path /tmp/sync_dir_<username>
*/
namespace dropbox
{
    class Cliente : public Operacoes
    {

        private:
            string username;
            string localIp;
            dropbox::FuncoesSocket *socket;
            string syncDirPath;
            mutex mtx;
            mutex socketMtx;
            vector<RegistroDeArquivos> fileRecords;

            /**
             *  This method is executed in another thread, treating the inotify and the updates from the Servidor
             */
            void askServidorUpdates();
            /**
             *  Create the sync directory on path /tmp/sync_dir_<username>
             */
            void createSyncDir();

        public:
            Cliente(string username, string ServidorAddr, int ServidorDistributorPort, string localIp);
            ~Cliente();

            string getUsername() { return username; }
            /**
             *  Send a TYPE_REQUEST_UPDATE packet for the Servidor and deals with the response to syncronize the local dir with the Servidor
             */
			string payload1;
			int payload2;
            void askUpdate();

            /**
             *  Process the detected inotify events
             */
            void eventsInotify(int* fd);

            /**
             *  Initialize the inotify and add the watch to the local dir
             */
            void initializeInotify(int *fd, int *wd);

            /**
             *  Download filename from Servidor
             */
            void download(string filename);

            /**
             *  Print the file records of the client
             */
            void list_client();

            /**
             * Delete all files from local dir and download all from Servidor
             */
            void get_sync_dir();

            /**
             * End session
             */
            void exit();

            /**
             * Prints the Servidor file record list
             */
            void requestServerFileList();

            /**
             * Remove the file record of filename from the list
             */
            void removeFileRecord(string filename);

            /**
             * Update filerecord on the list
             */
            void updateFileRecord(RegistroDeArquivos newFile);
            
            /**
             * Return the Client's socket
             */
            dropbox::FuncoesSocket * getSocket() { return socket; }

            /**
             * Return full path of syncDir folder
             */
            string getSyncDirPath() { return syncDirPath; }
            
            /**
             * Mutex to synchronize update sending and keyboard commnands 
             */
            void lockMutex() { this->mtx.lock(); };
            void unlockMutex() { this->mtx.unlock(); };

    };
}



#endif