#include "../include/Servidor.hpp"

using namespace std;
using namespace dropbox; 

Servidor::Servidor(string ipLocal) : connectClientSocket(SERVER_PORT), listenToServersSocket(BACKUPS_PORT)
{
    this->isMain = true;
    this->ipLocal = ipLocal;
    this->ipMain = ipLocal;
    this->backups = {};
    this->usersIp = {};
    this->backupsSockets = {};
    this->status = STATUS_NORMAL;
    initializeUsers();
    initializePorts();
}

Servidor::Servidor(string ipLocal, string ipMain, vector<string> backups) : connectClientSocket(SERVER_PORT), listenToServersSocket(BACKUPS_PORT)
{
    this->isMain = false;
    this->ipLocal = ipLocal;
    this->ipMain = ipMain;
    this->backups = backups;
    this->usersIp = {};
    this->backupsSockets = {};
    this->status = STATUS_NOT_YET;
    this->talkToPrimary = new FuncoesSocket(ipMain, BACKUPS_PORT);
    initializeUsers();
    initializePorts();
}

void Servidor::run(){
    thread listenToServersThread(&Servidor::listenToServers, this, &(this->listenToServersSocket));
    listenToServersThread.detach();

    if(!this->isMain) {
        this->makeConnection();
    }
    while(true){
        if(this->isMain){
            connectNewClient();
        }
        else {
            this->electionMutex.lock();
            if (this->status == STATUS_NORMAL) {
                this->electionMutex.unlock();
                Pacote packet = make_packet(TYPE_PING, 1, 1, -1, "");
                talkToPrimaryMtx.lock();
                bool isPrimaryAlive = this->talkToPrimary->send(&packet, 100);
                talkToPrimaryMtx.unlock();
                this->electionMutex.lock();
                if(!isPrimaryAlive && this->status == STATUS_NORMAL){
                    this->status = BEGIN_ELECTION;
                    this->electionMutex.unlock();
                    backupList.lock();
                    this->removeFromBackup(this->backups, this->ipMain);
                    backupList.unlock();
                } else {
                    this->electionMutex.unlock();
                }
                this_thread::sleep_for(chrono::milliseconds(3000));
            } else {
                this->electionMutex.unlock();
            }
        }
        
    }
}

void Servidor::removeFromBackup(vector<string> backups, string main) {
    vector<string>::iterator it = backups.begin();
    vector<string> newBackups;
    for(it; it != backups.end(); it++) {
        if (main.compare(*it) != 0) {
            newBackups.push_back(*it);
        }
    }
    this->backups.clear();
    for(string backup: newBackups) {
        this->backups.push_back(backup);
    }
}


Servidor::~Servidor(){
    for(Usuario *user : users){
        delete user;
    }
}   

void Servidor::propagateNewBoss() {
    for (string ip : this->usersIp) {
        FuncoesSocket userSocket(ip, 10000);
        Pacote packet = make_packet(TYPE_NEW_BOSS, 1, 1, -1, this->ipLocal.c_str());
        userSocket.send(&packet);
    }
}

void Servidor::makeConnection(){
    Pacote packet = make_packet(TYPE_MAKE_BACKUP, 1, 1, -1, ipLocal.c_str());
    talkToPrimaryMtx.lock();
    talkToPrimary->send(&packet);
    talkToPrimaryMtx.unlock();
}

void Servidor::answer(string ip) {
    FuncoesSocket socket(ip, 9000);
    Pacote packet = make_packet(TYPE_ANSWER, 1, 1, ip.size(), ip.c_str());
    socket.send(&packet, 100);
}

void Servidor::becomeMain() {
    backupList.lock();
    for(string ip : this->backups) {
        FuncoesSocket socket(ip, 9000);
        Pacote packet = make_packet(TYPE_COORDINATOR, 1, 1, ipLocal.size(), ipLocal.c_str());
        socket.send(&packet, 100);
    }
    backupList.unlock();
    this->isMain = true;
    this->electionMutex.lock();
    this->status = STATUS_COORDINATOR;
    this->electionMutex.unlock();
    this->propagateNewBoss();
}

vector<string> Servidor::getHighers() {
    vector<string> highers;
    backupList.lock();
    removeFromBackup(this->backups, this->ipMain);
    for(string ip : this->backups) {
        if (ip > ipLocal)
            highers.push_back(ip);
    }
    backupList.unlock();
    return highers;
}

void Servidor::sendHighersElection(vector<string> highers) {
    for(string higher : highers) {
        FuncoesSocket socket(higher, 9000);
        Pacote packet = make_packet(TYPE_ELECTION, 1, 1, ipLocal.size(), ipLocal.c_str());
        socket.send(&packet);
    }
}

void Servidor::startElection(vector<string> highers)
{
    this->electionMutex.lock();
    this->status = STATUS_ELECTION;
    this->electionMutex.unlock();
    this->sendHighersElection(highers);
    if (this->waitForAnswer()) {
        if (this->waitForCoordinator() || this->status == STATUS_NOT_YET)
            return;
        this->becomeMain();
    }
    this->becomeMain();
}

void Servidor::createNewPortBackup(FuncoesSocket * socket)
{
    int newPort = getAvailablePort();
    Pacote packet = make_packet(TYPE_MAKE_CONNECTION, 1, 1, -1, to_string(newPort).c_str());
    FuncoesSocket * newSocket = new FuncoesSocket(newPort);
    socket->send(&packet);
    thread listenToBackupThread(&Servidor::listenToServers, this, newSocket);
    listenToBackupThread.detach();
}

void Servidor::handleReceiveCoordinator(Pacote * res) {
    this->electionMutex.lock();
    this->status = STATUS_NOT_YET;
    this->electionMutex.unlock();
    backupList.lock();
    removeFromBackup(this->backups, this->ipMain);
    backupList.unlock();
    this->ipMain = string(res->payload);
    talkToPrimaryMtx.lock();
    this->talkToPrimary = new FuncoesSocket(ipMain, 9000);
    synchronize();
    talkToPrimaryMtx.unlock();
    this->makeConnection();
    this->electionMutex.lock();
    this->status = STATUS_NORMAL;
    this->electionMutex.unlock();
}

void Servidor::listenToServers(FuncoesSocket * socket){
    string ipDoBackup;
    FuncoesSocket *talkToBackup;
    Pacote * data = NULL;
    while(true){
        this->electionMutex.lock();
        if (this->status == BEGIN_ELECTION) {
            this->status = STATUS_ELECTION;
            this->electionMutex.unlock();
            if (this->getHighers().size() > 0) {
                this->startElection(this->getHighers());
            }
            else {
                this->becomeMain();
            }
        }
        else {
            this->electionMutex.unlock();
            data = socket->receive(TIMEOUT_ON, 5);
        }
        if (data == NULL)
            continue;
        string username = string(data->username);
        Usuario * user = getUser(username);
        string ip;
        RegistroDeArquivos fileRecord;
        switch (data->type)
        {
            case TYPE_MAKE_CONNECTION:
                talkToPrimaryMtx.lock();
                this->talkToPrimary = new FuncoesSocket(ipMain, stoi(data->payload));
                synchronize();
                talkToPrimaryMtx.unlock();
                this->electionMutex.lock();
                this->status = STATUS_NORMAL;
                this->electionMutex.unlock();
                break;
            case TYPE_COORDINATOR:
                handleReceiveCoordinator(data);
                break;
            case TYPE_ANSWER:
                if(!this->waitForCoordinator() && this->status == STATUS_ELECTION) {
                    this->becomeMain();
                }
                break;
            case TYPE_ELECTION:
                ip = string(data->payload);
                this->answer(ip);
                this->electionMutex.lock();
                if (this->status == STATUS_NORMAL) {
                    this->status = STATUS_ELECTION;
                    this->electionMutex.unlock();
                    if (this->getHighers().size() > 0) {
                        this->startElection(this->getHighers());
                    }
                    else {
                        this->becomeMain();
                    }
                }
                this->electionMutex.unlock(); 
                break;
            case TYPE_MAKE_BACKUP:
                ipDoBackup = string(data->payload);
                talkToBackup = new FuncoesSocket(ipDoBackup, BACKUPS_PORT);
                this->backupsSockets.push_back(talkToBackup);
                this->createNewPortBackup(talkToBackup);
                break;
            case TYPE_PING:
                break;
            case TYPE_CREATE_USER:
                this->usersIp.push_back(string(data->payload));
                if(user == nullptr) {
                    user = new Usuario(username, dirRaiz+username+"/");
                    users.push_back(user);
                }
                break;
            case TYPE_DELETE: 
                deleteFile(user->getDirPath() + string(data->payload));
                user->removeFileRecord(string(data->payload));
                break;
            case TYPE_SEND_FILE:
                fileRecord = *((RegistroDeArquivos *)data->payload);
                receiveFile(&listenToServersSocket, string(data->payload), user->getDirPath());
                user->updateFileRecord(fileRecord);
                break;
            default:
                cout << "PACOTE INCORRETO! " << data->type << endl;
                break;
        }
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
        cout << "Criado o diretório raiz" << endl;
    else
        cout << "Não foi possivel abrir o diretório raiz" << endl;

    cout << "Usuários :" << endl;
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

void Servidor::propagateDelete(string filename, string username) 
{
    propagationMutex.lock();
    Usuario * user = getUser(username);
    for(FuncoesSocket * socket : this->backupsSockets) {
        this->sendDeleteFile(socket, filename.c_str(), username);
        user->removeFileRecord(filename);
    }
    propagationMutex.unlock();
}

void Servidor::propagateFile(string filename, string username) 
{
    propagationMutex.lock();
    Usuario * user = getUser(username);
    for(FuncoesSocket * socket : this->backupsSockets) {
        RegistroDeArquivos record = this->getRecord(user->getFileRecords(),filename);
        this->sendFile(socket, user->getDirPath() + filename, record, username);
    }
    propagationMutex.unlock();
}

void Servidor::propagateConnection(string username, string userIp)
{   
    propagationMutex.lock();
    Pacote packet = make_packet(TYPE_CREATE_USER, 1, 1, -1, userIp.c_str(), username.c_str());
    for(FuncoesSocket * socket : this->backupsSockets) {
        socket->send(&packet);
    }
    propagationMutex.unlock();
}

void Servidor::refuseOverLimitClient(Usuario *user)
{
    string message = "Numero máximo de dispositivos de " + user->getUsername() + " atingido. Numero máximo : " + to_string(MAX_DEVICES);
    Pacote packet = make_packet(TYPE_REJECT_TO_LISTEN, 1, 1, -1, message.c_str());
    connectClientSocket.send(&packet);
}

bool Servidor::waitForAnswer()
{
    while(true) {
        Pacote * res = this->listenToServersSocket.receive(TIMEOUT_ON, 1500);
        if (res == NULL)
            return false;
        cout << res->type << endl;
        if (res->type == TYPE_ANSWER) {
            return true;
        }
        if (res->type == TYPE_COORDINATOR) {
            handleReceiveCoordinator(res);
            return true;
        }
    }
}

bool Servidor::waitForCoordinator()
{
    while(true) {
        Pacote * res = this->listenToServersSocket.receive(TIMEOUT_ON, 1500);
        if (res == NULL)
            return false;
        if (res->type == TYPE_COORDINATOR) {
            handleReceiveCoordinator(res);
            return true;
        }
    }
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
    cout << "Checando se o usuário" << user->getUsername() << " esta atualizado." << endl;
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
    cout << "Usuario " + user->getUsername() + " finalizou a sessão na porta " << port << endl;
}

void synchronize(){
    this_thread::sleep_for(chrono::milliseconds(1000));
}