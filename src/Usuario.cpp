#include "../include/Usuario.hpp"
#include <iostream>
#include <fstream>

using namespace std;
using namespace dropbox;

Usuario::Usuario(string username, string dirPath): username(username), dirPath(dirPath){
    createUserDir();
}

void Usuario::createUserDir(){
    struct stat st;
	if(stat(dirPath.c_str(), &st) == -1) //Se não existe cria, se existe faz nada
		mkdir(dirPath.c_str(), 0777);
    DIR * dir;
    struct dirent *ent;
    struct stat filestatus;
    if ((dir = opendir (dirPath.c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            if(ent->d_type == 0x8) {
                stat((dirPath + ent->d_name).c_str(), &filestatus);
                RegistroDeArquivos fileRecord = criarRegistro(ent->d_name, filestatus.st_ctim.tv_sec, filestatus.st_atim.tv_sec, filestatus.st_mtim.tv_sec, filestatus.st_size);
				fileRecords.push_back(fileRecord);
            }
        }
        closedir(dir);
    } 
}

vector<RegistroDeArquivos> Usuario::getFileRecords() {
    return this->fileRecords;
}

void Usuario::removeFileRecord(string filename) {
	vector<RegistroDeArquivos>::iterator it;
	for(it = this->fileRecords.begin(); it != this->fileRecords.end(); it++) {
		if (string(it->nomeArquivo) == filename) {
			this->fileRecords.erase(it);
			return;
		}
	}
}

void Usuario::updateFileRecord(RegistroDeArquivos newFile) {
	vector<RegistroDeArquivos>::iterator it;
	for(it = this->fileRecords.begin(); it != this->fileRecords.end(); it++) {
		if (string(it->nomeArquivo) == string(newFile.nomeArquivo)) {
			*it = newFile;
			return;
		}
	}
	this->fileRecords.push_back(newFile);
}

string Usuario::getUsername(){
    return this->username;
}

string Usuario::getDirPath(){
    return this->dirPath;
}

int Usuario::getNumDevicesConnected(){
    return this->devices.size();
}

void Usuario::addDevice(FuncoesSocket *socket){
    this->devices.push_back(socket);
}

void Usuario::closeDeviceSession(FuncoesSocket *socket){
    for(auto it = this->devices.begin(); it != this->devices.end(); it++){
        if(*it == socket){
            this->devices.erase(it);
            break;
        }
    }
}