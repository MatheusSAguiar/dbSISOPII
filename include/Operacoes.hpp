#ifndef __OPERACOES_HPP__
#define __OPERACOES_HPP__

#include <fstream>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <iomanip>
#include <vector>

#include "RegistroDeArquivos.hpp"
#include "FuncoesSocket.hpp"

using namespace std;

namespace dropbox {
class Operacoes
{
    public:
    Operacoes();

    // Retorna o registro de arquivos de todos os arquivos do caminho
    vector<RegistroDeArquivos> getFileList(string dirPath);

    // Retorna o registro de um arquivo
    RegistroDeArquivos getRecord(vector<RegistroDeArquivos> files, string filename);

    // Deleta o arquivo de um caminho
    void deleteFile(string filepath);

    // Deleta todos os arquivos de uma pasta
    void deleteAll(vector<RegistroDeArquivos> files, string dirPath);

    // Manda um pacote TYPE_NOTHING_TO_SEND
    void sendNothing(FuncoesSocket * socket);

    // Manda pacotes TYPE_DATA com o registro dos arquivos da lista
    void sendFileList(FuncoesSocket * socket, vector<RegistroDeArquivos> files);

    // Manda um pacote TYPE_SEND_FILE com o registro de arquivos e depois manda o arquivo em pacotes TYPE_DATA
    void sendFile(FuncoesSocket * socket, string filePath, RegistroDeArquivos fileRec, string username);

    // Manda um pacote TYPE_SEND_FILE_NO_RECORD e manda o arquivo em pacotes TYPE_DATA
    void sendFile(FuncoesSocket * socket, string filePath);
    void sendFile(FuncoesSocket * socket, string filePath, string username);

    // Manda um pacote TYPE_DELETE com o nome do arquivo para deletar
    void sendDeleteFile(FuncoesSocket * socket, string filename);

    void sendDeleteFile(FuncoesSocket * socket, string filename, string username);

    // Manda todos os arquivos do diretório com o registro
    void sendUploadAll(FuncoesSocket * socket, string dirPath, vector<RegistroDeArquivos> files, string username);

    // Recebe um arquivo e cria em <filename>
    void receiveFile(FuncoesSocket * socket, string filename, string dirPath);

    // Recebe uma lista de registro de arquivos
    vector<RegistroDeArquivos> receiveFileList(FuncoesSocket * socket);

    // Printa a lista de registro de arquivos
    void printFileList(vector<RegistroDeArquivos> fileRecords);

    // Recebe todos os arquivos do caminho
    void receiveUploadAll(FuncoesSocket * socket, string dirPath);

    // Recebe um registro de arquivos
    RegistroDeArquivos receiveFileRecord(FuncoesSocket * socket);

    // Manda um arquivo e recebe um registro
    RegistroDeArquivos sendFileClient(FuncoesSocket * socket, string filePath, string username);
};
}

#endif