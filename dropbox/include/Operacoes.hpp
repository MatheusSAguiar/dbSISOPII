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

    /**
    *  Return the FileRecords for all the files in the given directory
    */
    vector<RegistroDeArquivos> getFileList(string dirPath);

    /**
    *  Return the FileRecord of the specific filename
    */
    RegistroDeArquivos getRecord(vector<RegistroDeArquivos> files, string filename);

    /**
    *  Remove the file
    */
    void deleteFile(string filepath);

    /**
    *  Remove all files from the given directory
    */
    void deleteAll(vector<RegistroDeArquivos> files, string dirPath);

    /**
    *  Send a packet of TYPE_NOTHING_TO_SEND
    */
    void sendNothing(FuncoesSocket * socket);

    /**
    *  Send TYPE_DATA packets containing each FileRecord of the list
    */
    void sendFileList(FuncoesSocket * socket, vector<RegistroDeArquivos> files);

    /**
    *  Send a TYPE_SEND_FILE with the file record and then send the whole file in TYPE_DATA packets
    */
    void sendFile(FuncoesSocket * socket, string filePath, RegistroDeArquivos fileRec, string username);

    /**
    *  Send a TYPE_SEND_FILE_NO_RECORD and then send the whole file in TYPE_DATA packets
    */
    void sendFile(FuncoesSocket * socket, string filePath);
    void sendFile(FuncoesSocket * socket, string filePath, string username);

    /**
    *  Send a TYPE_DELETE packet with the name of the file to delete
    */
    void sendDeleteFile(FuncoesSocket * socket, string filename);

    void sendDeleteFile(FuncoesSocket * socket, string filename, string username);

    /**
    *  Send all the files on the dirPath with their file records
    */
    void sendUploadAll(FuncoesSocket * socket, string dirPath, vector<RegistroDeArquivos> files, string username);

    /**
    *  Receive a file and create it in filename
    */
    void receiveFile(FuncoesSocket * socket, string filename, string dirPath);

    /**
    *  Receive and return a file records list
    */
    vector<RegistroDeArquivos> receiveFileList(FuncoesSocket * socket);

    /**
    *  Print the given file records list
    */
    void printFileList(vector<RegistroDeArquivos> fileRecords);

    /**
    *  Receive all files
    */
    void receiveUploadAll(FuncoesSocket * socket, string dirPath);

    /**
    * Receive a file record
    */
    RegistroDeArquivos receiveFileRecord(FuncoesSocket * socket);

    /**
    * Send a file and receives a filerecord
    */
    RegistroDeArquivos sendFileClient(FuncoesSocket * socket, string filePath, string username);
};
}

#endif