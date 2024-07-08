#include "../include/Cliente.hpp"
#include "../include/Pacote.hpp"
#include <iostream>
#include <fstream>
#include <unistd.h>



using namespace std;
using namespace dropbox;


Cliente::Cliente (string username, string ServidorAddr, int ServidorDistributorPort, string localIp) : username(username), syncDirPath("/tmp/sync_dir_"+username+"/")
{
	FuncoesSocket socketToGetPort(ServidorAddr, ServidorDistributorPort);
	this->username = username;
	this->localIp = localIp;
	Pacote request = make_packet(TYPE_MAKE_CONNECTION, 1, 1, this->localIp.length(),this->localIp.c_str(), this->username.c_str());
	socketToGetPort.send(&request);
	
	Pacote *newPort = socketToGetPort.receive(TIMEOUT_OFF);
	if(newPort->type == TYPE_MAKE_CONNECTION){
		this->socket = new FuncoesSocket(ServidorAddr, stoi(newPort->payload));
	} 
	else if(newPort->type == TYPE_REJECT_TO_LISTEN) {
		std::cout << newPort->payload << endl;
		std::exit(1);
	}

	get_sync_dir();
	thread askServidorUpdatesThread(&Cliente::askServidorUpdates, this);
	askServidorUpdatesThread.detach();
	std::cout << "Conectado." << endl;
}

void Cliente::initializeInotify(int *fd, int *wd){
	*fd = inotify_init();
	if ( *fd < 0 ) {
		std::cout << "Erro ao iniciar o inotify" << endl;
	}
	*wd = inotify_add_watch(*fd, syncDirPath.c_str(), IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO); 

	if (*wd == -1){
		std::cout << "Erro ao adicionar inotify em " << syncDirPath.c_str() << endl;
	}
}
void Cliente::askServidorUpdates() {
	
	int fd, wd;
	this->initializeInotify(&fd, &wd);
	
	while(1) {
		this->lockMutex();
		
		this->eventsInotify(&fd);
		
		inotify_rm_watch(fd, wd);
		close(fd);

		this->askUpdate();

		int fd, wd;
		this->initializeInotify(&fd, &wd);

		this->unlockMutex();
		this_thread::sleep_for(chrono::milliseconds(5000));
	}
	inotify_rm_watch(fd, wd);
	close(fd);
}

void Cliente::createSyncDir(){
	struct stat st;
	if(stat(syncDirPath.c_str(), &st) == -1){ //Se não existe cria, se existe faz nada
		mkdir(syncDirPath.c_str(), 0777);
		cout << "Pasta de sync criada em " + syncDirPath << endl;
	}
}

Cliente::~Cliente(){
	delete this->socket;
}

void Cliente::askUpdate() {
	Pacote request = make_packet(TYPE_REQUEST_UPDATE, 1 , 1, -1, "type_request_update", this->username.c_str());
	socketMtx.lock();
	bool ass = this->socket->send(&request);
	socketMtx.unlock();
	if (ass == -1) {
		socketMtx.lock();
		this->socket = new FuncoesSocket(this->payload1, this->payload2);
		socketMtx.unlock();
		ass = this->socket->send(&request);
	}
	else if (ass == 0)
		return;
	this->sendFileList(this->socket, this->fileRecords);
	while(1) {
		socketMtx.lock();
		Pacote * response = this->socket->receive(TIMEOUT_ON);
		socketMtx.unlock();
		RegistroDeArquivos fileRecord;
		if (response == NULL) return;
		switch (response->type) {
			case TYPE_NOTHING_TO_SEND:
				break;
			case TYPE_REQUEST_UPDATE_DONE:
				return;
			case TYPE_DELETE:
				socketMtx.lock();
				this->deleteFile(this->getSyncDirPath() + string(response->payload));
				socketMtx.unlock();
				this->removeFileRecord(string(response->payload));
				break;
			case TYPE_SEND_FILE:
				fileRecord = *((RegistroDeArquivos *)response->payload);
				socketMtx.lock();
				this->receiveFile(this->socket, string(fileRecord.nomeArquivo), 
					this->getSyncDirPath());
				socketMtx.unlock();
				this->updateFileRecord(fileRecord);
				break;
			case TYPE_SEND_UPLOAD_ALL:
				socketMtx.lock();
				this->receiveUploadAll(this->socket, this->getSyncDirPath());
				socketMtx.unlock();
				break;
			case TYPE_DELETE_ALL:
				socketMtx.lock();
				this->deleteAll(this->getFileList(this->getSyncDirPath()), this->getSyncDirPath());
				socketMtx.unlock();
				break;
			default:
				std::cout << "Recebido " + to_string(response->type) << endl;
		}
	}
}

void Cliente::removeFileRecord(string filename) {
	vector<RegistroDeArquivos>::iterator it;
	for(it = this->fileRecords.begin(); it != this->fileRecords.end(); it++) {
		if (string(it->nomeArquivo) == filename) {
			this->fileRecords.erase(it);
			return;
		}
	}
}

void Cliente::updateFileRecord(RegistroDeArquivos newFile) {
	vector<RegistroDeArquivos>::iterator it;
	for(it = this->fileRecords.begin(); it != this->fileRecords.end(); it++) {
		if (string(it->nomeArquivo) == string(newFile.nomeArquivo)) {
			*it = newFile;
			return;
		}
	}
	this->fileRecords.push_back(newFile);
}

void Cliente::eventsInotify(int* fd){
	int length, i = 0;
	char buffer[BUF_LEN];
	struct pollfd pfd = { *fd, POLLIN, 0 };
	int ret = poll(&pfd, 1, 50);  // timeout de 50ms
	if (ret < 0) {
		fprintf(stderr, "Erro no poll %s\n", strerror(errno));
	} else if (ret == 0) {
		// Se não tem eventos, não faz nada
	} else {
		// Se tem eventos, processa eles
		length = read(*fd, buffer, BUF_LEN);  
		while (i < length) {
			struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
			if (event->len ) {
				string filename(event->name);
				string path = syncDirPath + filename;
				vector<RegistroDeArquivos> tempFiles = this->getFileList(this->getSyncDirPath());
				if(event->mask & IN_MOVED_TO){
					socketMtx.lock();
					RegistroDeArquivos newRecord = this->sendFileClient(this->socket, path.c_str(), this->username);
					socketMtx.unlock();
					this->updateFileRecord(newRecord);
				}
			
				if(event->mask & IN_MOVED_FROM || event->mask & IN_DELETE){
					socketMtx.lock();
					this->sendDeleteFile(this->socket, filename.c_str(), this->username);
					socketMtx.unlock();
					this->removeFileRecord(filename);
				}  
		
				if(event->mask & IN_CLOSE_WRITE){
					socketMtx.lock();
					this->sendDeleteFile(this->socket, filename.c_str(), this->username);
					RegistroDeArquivos newRecord = this->sendFileClient(this->socket, path.c_str(), this->username);
					socketMtx.unlock();
					this->updateFileRecord(newRecord);
				} 
	
			i += EVENT_SIZE + event->len;
			}
		}
	}
}

void Cliente::download(string filename){
	Pacote request = make_packet(TYPE_REQUEST_DOWNLOAD, 1 , 1, -1, filename.c_str());
	this->socket->send(&request);

	char cCurrentPath[FILENAME_MAX];
	if (!getcwd(cCurrentPath, sizeof(cCurrentPath))) 
		std::cout << "Erro ao pegar o diretorio atual" << endl;
	cCurrentPath[sizeof(cCurrentPath) - 1] = '\0';
	string currentPath(cCurrentPath);
	currentPath += "/";

	Pacote *data = this->socket->receive(TIMEOUT_OFF);
	if(data->type == TYPE_SEND_FILE) {
		RegistroDeArquivos * fileRecord = (RegistroDeArquivos *)(data->payload);
		this->receiveFile(this->socket, string(fileRecord->nomeArquivo), currentPath);
	}
	else if(data->type == TYPE_NOTHING_TO_SEND)
		std::cout << "Arquivo não existe." << endl;
}

void Cliente::list_client(){
	printFileList(this->fileRecords);
}

void Cliente::get_sync_dir(){

	this->createSyncDir();
	
	DIR * dir;
    struct dirent *ent;
    if ((dir = opendir (this->getSyncDirPath().c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            if(ent->d_type == 0x8) {
                deleteFile(this->getSyncDirPath()+string(ent->d_name));
            }
        }
        closedir(dir);
	    cout << "Removido todos os arquivos locais." << endl;
    } 
	Pacote packet = make_packet(TYPE_LIST_SERVER, 1, 1, -1, "list_server");
	socket->send(&packet);
	this->fileRecords = this->receiveFileList(this->getSocket());
	for(RegistroDeArquivos record : this->fileRecords){
		Pacote request = make_packet(TYPE_REQUEST_DOWNLOAD, 1 , 1, -1, record.nomeArquivo);
		this->socket->send(&request);
		Pacote *data = this->socket->receive(TIMEOUT_OFF);
		if(data->type == TYPE_SEND_FILE) {
			this->receiveFile(this->socket, record.nomeArquivo, this->getSyncDirPath());
		}
	}
	this->printFileList(this->fileRecords);
	cout << "Recebido todos os arquivos do servidor." << endl;
}

void Cliente::exit(){
	Pacote message = make_packet(EXIT, 1, 1, -1, "exit");
	this->socket->send(&message);
}

void Cliente::requestServerFileList() {

	Pacote packet = make_packet(TYPE_LIST_SERVER, 1, 1, -1, "list_server");
	socket->send(&packet);
	vector<RegistroDeArquivos> files = this->receiveFileList(this->getSocket());
	this->printFileList(files);
}