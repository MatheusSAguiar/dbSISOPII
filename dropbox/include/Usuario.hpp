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

// Classe de um usuário no servidor
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

            // Cria a pasta do usuário no servidor se for nova
            void createUserDir();

        public:
            Usuario(string username, string dirPath);
            string getUsername();
            string getDirPath();

            // Retorna o número de dispositivos conectados
            int getNumDevicesConnected();

            // Adicionado o socket para a lista de dispositivos
            void addDevice(FuncoesSocket *socket);

            // Remove o socket da lista de dispositivos
            void closeDeviceSession(FuncoesSocket *socket);

            // Mutex para evitar problemas de condição de corrida
            void lockDevices() { this->mt.lock(); };
            void unlockDevices() { this->mt.unlock(); };
            vector<RegistroDeArquivos> getFileRecords();

            // Remove o registro de arquivos do arquivo
            void removeFileRecord(string filename);

            // Atualiza o registro de arquivos de um arquivo
            void updateFileRecord(RegistroDeArquivos newFile);
    };
}
#endif