#include "../include/Servidor.hpp"

using namespace std;
using namespace dropbox; 

Servidor::Servidor(string ipLocal) : connectClientSocket(SERVER_PORT), listenToServersSocket(BACKUPS_PORT)
{
    this->isMain = true;
    this->ipLocal = ipLocal;
    this->ipMain = ipLocal;
    this->usersIp = {};
    initializeUsers();
    initializePorts();
}

void Servidor::run(){

    while(true){
        if(this->isMain){
            connectNewClient();
        }        
    }
}

Servidor::~Servidor(){
    for(Usuario *user : users){
        delete user;
    }
}   

void Servidor::initializeUsers()
{
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (dirRaiz.c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            if(ent->d_type == 0x4 && string(ent->d_name) != string("..") && string(ent->d_name) != string(".")) {
                Usuario *user = new Usuario(ent->d_name, dirRaiz+ent->d_name+"/");
                users.push_back(user);
            }
        }
        closedir(dir);
    } 
    else if(mkdir(dirRaiz.c_str(), 0777) == 0) 
        cout << "CREATED ROOT DIR" << endl;
    else
        cout << "COULDN'T OPEN OR CREATE ROOT DIR" << endl;

    cout << "System Users :" << endl;
    for(Usuario *user : users)
    {
        cout << user->getUsername().c_str() << endl;
    }
}

void Servidor::initializePorts()
{
    for(int p = FIRST_PORT; p <= LAST_PORT; p++){
        portsAvailable[p - FIRST_PORT] = true;
    }
}

int Servidor::getAvailablePort()
{
    portsMutex.lock();
    for(int p = FIRST_PORT; p <= LAST_PORT; p++){
        int i = p - FIRST_PORT;
        if(portsAvailable[i]){
            portsAvailable[i] = false;
            portsMutex.unlock();
            return p;
        }
    }
    portsMutex.unlock();
    return -1;
}

Usuario* Servidor::getUser(string username)
{
    for(unsigned i = 0; i < users.size(); i++){
        if(users[i]->getUsername() == username){
            return users[i];
        }
    }
    return nullptr;
}

void Servidor::connectNewClient()
{

    Pacote *d = connectClientSocket.receive(TIMEOUT_OFF);
    if (d->type == TYPE_MAKE_CONNECTION) {
        string username(d->username);
        string userIp(d->payload);
        Usuario *user = getUser(username);
        if(user == nullptr){
            user = new Usuario(username, dirRaiz+username+"/");
            users.push_back(user);
        }
        else{
            user->lockDevices();
            if(user->getNumDevicesConnected() == MAX_DEVICES){
                refuseOverLimitClient(user);
                user->unlockDevices();
                return;
            }
            user->unlockDevices();
        }
        int newPort = getAvailablePort();
        Pacote packet = make_packet(TYPE_MAKE_CONNECTION, 1, 1, -1, to_string(newPort).c_str());
	    FuncoesSocket *socket = new FuncoesSocket(newPort);
        connectClientSocket.send(&packet);
        
        user->lockDevices();
        user->addDevice(socket);
        user->unlockDevices();

        thread listenToClientThread(&Servidor::listenToClient, this, socket, user);
        listenToClientThread.detach();
    }
}

void Servidor::refuseOverLimitClient(Usuario *user)
{
    string message = "Number of devices for user " + user->getUsername() + " were used up! Max number of devices : " + to_string(MAX_DEVICES);
    Pacote packet = make_packet(TYPE_REJECT_TO_LISTEN, 1, 1, -1, message.c_str());
    connectClientSocket.send(&packet);
}

void Servidor::listenToClient(FuncoesSocket *socket, Usuario *user)
{   
    bool exit = false;
    RegistroDeArquivos * temp = NULL;
    RegistroDeArquivos fileTemp;
    vector<RegistroDeArquivos> tempFiles = user->getFileRecords();
	while(!exit){
		Pacote *data = socket->receive(TIMEOUT_OFF);
        user->lockDevices();
        switch(data->type){
            case TYPE_REQUEST_DOWNLOAD:
                tempFiles = user->getFileRecords();
                fileTemp = this->getRecord(tempFiles, string(data->payload));
                sendFile(socket, user->getDirPath() + string(data->payload), fileTemp, string(data->username));
                break;
            case TYPE_DELETE: 
                deleteFile(user->getDirPath() + string(data->payload));
                user->removeFileRecord(string(data->payload));
                break;
            case TYPE_LIST_SERVER:
                sendFileList(socket, user->getFileRecords());
                break;
            case TYPE_SEND_FILE_NO_RECORD:
                receiveFile(socket, string(data->payload), user->getDirPath());
                user->updateFileRecord(this->getRecord(this->getFileList(user->getDirPath()), string(data->payload)));
                sendFileRecord(socket, string(data->payload), user);
                break;
            case TYPE_REQUEST_UPLOAD_ALL:
                sendUploadAll(socket, user->getDirPath(), getFileList(user->getDirPath()), user->getUsername());
                break;
            case TYPE_REQUEST_UPDATE:
                receiveAskUpdate(socket, user);
                break;
            case EXIT:
                exitUser(socket, user);
                exit = true;
                break;
        }
        user->unlockDevices();
	}

    delete socket;
}

void Servidor::sendFileRecord(FuncoesSocket * socket, string filename, Usuario * user) {
    RegistroDeArquivos record = this->getRecord(user->getFileRecords(), filename);
    Pacote packet = make_packet(TYPE_DATA, 1, 1, sizeof(RegistroDeArquivos), (char*)&record);
    socket->send(&packet);
}

int Servidor::lookForRecordAndRemove(RegistroDeArquivos file, vector<RegistroDeArquivos> *files, RegistroDeArquivos *updatedFile) {
    vector<RegistroDeArquivos>::iterator it;
    for(it = files->begin(); it != files->end(); it++) {
        if (string(it->nomeArquivo) == string(file.nomeArquivo)) {
            if (it->mTime == file.mTime) {
                files->erase(it);
                return OK;
            }
            else {
                *updatedFile = *it;
                files->erase(it);
                return UPDATE;
            }
        }
    }
    return DELETE;
}

void Servidor::updateClient(vector<RegistroDeArquivos> serverFiles, vector<RegistroDeArquivos> clientFiles, FuncoesSocket *socket, Usuario *user) {
    cout << "Checking if user " << user->getUsername() << " is updated." << endl;
    vector<RegistroDeArquivos>::iterator it;
    RegistroDeArquivos temp;
    for(it = clientFiles.begin(); it != clientFiles.end(); it++) {
        switch(this->lookForRecordAndRemove(*it, &serverFiles, &temp)) {
            case OK: 
                break;
            case DELETE:
                this->sendDeleteFile(socket, string(it->nomeArquivo));
                break;
            case UPDATE: 
                this->sendDeleteFile(socket, string(temp.nomeArquivo));
                this->sendFile(socket, user->getDirPath() + string(temp.nomeArquivo), temp, user->getUsername());
                break;
        }
    }

    for(RegistroDeArquivos serverFile: serverFiles) { 
        this->sendFile(socket, user->getDirPath() + string(serverFile.nomeArquivo), serverFile, user->getUsername());
    }

}

void Servidor::receiveAskUpdate(FuncoesSocket * socket, Usuario * user) {
    vector<RegistroDeArquivos> clientFiles = this->receiveFileList(socket);
    this->updateClient(this->getFileList(user->getDirPath()), clientFiles, socket, user);
    Pacote packet = make_packet(TYPE_REQUEST_UPDATE_DONE, 1, 1, sizeof("request_update_done"), "request_update_done");
	socket->send(&packet);
}

void Servidor::setPortAvailable(int port){
    portsMutex.lock();
    this->portsAvailable[port - FIRST_PORT] = true;
    portsMutex.unlock();
}

void Servidor::exitUser(FuncoesSocket *socket, Usuario *user){
    int port = socket->getPortInt();
    user->closeDeviceSession(socket);
    setPortAvailable(port);
    cout << "User " + user->getUsername() + " ended session on device on port " << port << endl;
}

void synchronize(){
    this_thread::sleep_for(chrono::milliseconds(1000));
}