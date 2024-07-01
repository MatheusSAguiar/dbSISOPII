#ifndef __USER_HPP_
#define __USER_HPP_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/stat.h>
#include <utime.h>
#include <mutex>
#include <dirent.h>
#include "FuncoesSocket.hpp"
#include "Pacote.hpp"
#include "RegistroDeArquivos.hpp"
#include "Constantes.hpp"

/*
*    Class representing a instance of an user on the Servidor, grouping their devices
*/
namespace dropbox
{
    class Usuario
    {
        private:
            string username;
            string dirPath;
            vector<RegistroDeArquivos> fileRecords;
            vector<FuncoesSocket*> devices;
            mutex mt;

            /**
            *  Create the user directory on the Servidor if it is a new one, otherwise get the file records of the dir
            */
            void createUserDir();

        public:
            Usuario(string username, string dirPath);
            string getUsername();
            string getDirPath();

            /**
            *  Returns if there is 0, 1 or 2 devices connected
            */
            int getNumDevicesConnected();

            /**
            *  Add the given FuncoesSocket to the devices list
            */
            void addDevice(FuncoesSocket *socket);

            /**
            *  Remove the given FuncoesSocket of the devices list
            */
            void closeDeviceSession(FuncoesSocket *socket);

            /**
            *  Lock and unlock functions for the devices mutex
            */
            void lockDevices() { this->mt.lock(); };
            void unlockDevices() { this->mt.unlock(); };
            vector<RegistroDeArquivos> getFileRecords();
            /**
             * Remove the file record of filename from the list
             */
            void removeFileRecord(string filename);

            /**
             * Update filerecord on the list
             */
            void updateFileRecord(RegistroDeArquivos newFile);
    };
}
#endif