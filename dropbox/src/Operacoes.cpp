#include "../include/Operacoes.hpp"

using namespace std;
using namespace dropbox;

Operacoes::Operacoes() {}

vector<RegistroDeArquivos> Operacoes::getFileList(string dirPath) {
    vector<RegistroDeArquivos> files;
    struct stat filestatus;
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir (dirPath.c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            if(ent->d_type == 0x8) {
				stat((dirPath + ent->d_name).c_str(), &filestatus);
                RegistroDeArquivos registroDeArquivos = criarRegistro(ent->d_name, filestatus.st_ctim.tv_sec, filestatus.st_atim.tv_sec, filestatus.st_mtim.tv_sec, filestatus.st_size);
				files.push_back(registroDeArquivos);
            }
        }
        closedir(dir);
    }
    return files;
}

void Operacoes::sendFileList(FuncoesSocket *socket, vector<RegistroDeArquivos> files){
    if(files.empty()){
        Pacote packet = make_packet(TYPE_NOTHING_TO_SEND, 1, 1, -1, "nothing_to_send");
        socket->send(&packet);
        return;
    }
    int seq = 1;
    for(RegistroDeArquivos record : files) {
        Pacote packet = make_packet(TYPE_DATA, seq, files.size(), sizeof(RegistroDeArquivos), (char*)&record);
        socket->send(&packet);
        seq++;
    }
}

void Operacoes::receiveFile(FuncoesSocket *socket, string filename, string dirPath) {
    string filePath = dirPath + filename;
		
    ofstream newFile;
		newFile.open(filePath, ofstream::trunc | ofstream::binary);
		if(!newFile.is_open()) {
			return;
    }
    int seqNumber, totalPackets;
    do{
        Pacote *packet = socket->receive(TIMEOUT_OFF);
        seqNumber = packet->seq;
        totalPackets = packet->totalSize;
        newFile.write(packet->payload, packet->len);
    }while(seqNumber != totalPackets);
    
    newFile.close();
}

RegistroDeArquivos Operacoes::getRecord(vector<RegistroDeArquivos> files, string filename) {
	for(RegistroDeArquivos file : files) {
		if (string(file.nomeArquivo) == filename)
			return file;
	}
}

void Operacoes::sendFile(FuncoesSocket *socket, string filePath, RegistroDeArquivos fileRec, string username){

	string filename = filePath.substr(filePath.find_last_of("/\\") + 1);
	struct stat buffer;   
  	if(stat(filePath.c_str(), &buffer) != 0){
		  Pacote failed = make_packet(TYPE_NOTHING_TO_SEND, 1, 1, -1, "", username.c_str());
		  socket->send(&failed);
		  return;
	}
	Pacote packet = make_packet(TYPE_SEND_FILE, 1, 1, sizeof(RegistroDeArquivos), (char *)&fileRec, username.c_str());
	socket->send(&packet);

	ifstream file;
	file.open(filePath, ifstream::in | ifstream::binary);
	file.seekg(0, file.end);
	int fileSize = file.tellg();
	file.seekg(0, file.beg);
	int totalPackets = 1 + ((fileSize - 1) / MESSAGE_LEN); // ceil(x/y)
	int lastPacketSize = fileSize % MESSAGE_LEN;
	int packetsSent = 0;
	int filePointer = 0;
	int packetSize = MESSAGE_LEN;
	char payload[MESSAGE_LEN];
	cout << "Uploading " << filePath << " Size: " << fileSize << " NumPackets: " << totalPackets << endl;

	while (packetsSent < totalPackets){
		file.seekg(filePointer, file.beg);
		filePointer += MESSAGE_LEN;
		if(packetsSent == totalPackets - 1) packetSize = lastPacketSize;
		file.read(payload, packetSize);
		Pacote packet = make_packet(TYPE_DATA, packetsSent + 1, totalPackets, packetSize, payload, username.c_str());
		socket->send(&packet);
		packetsSent++;
	}
	file.close();
}

void Operacoes::sendFile(FuncoesSocket *socket, string filePath){

	string filename = filePath.substr(filePath.find_last_of("/\\") + 1);
	struct stat buffer;   
  	if(stat(filePath.c_str(), &buffer) != 0){
		  Pacote failed = make_packet(TYPE_NOTHING_TO_SEND, 1, 1, -1, "");
		  socket->send(&failed);
		  return;
	}
	Pacote packet = make_packet(TYPE_SEND_FILE_NO_RECORD, 1, 1, -1, filename.c_str());
	socket->send(&packet);

	ifstream file;
	file.open(filePath, ifstream::in | ifstream::binary);
	file.seekg(0, file.end);
	int fileSize = file.tellg();
	file.seekg(0, file.beg);
	int totalPackets = 1 + ((fileSize - 1) / MESSAGE_LEN); // ceil(x/y)
	int lastPacketSize = fileSize % MESSAGE_LEN;
	int packetsSent = 0;
	int filePointer = 0;
	int packetSize = MESSAGE_LEN;
	char payload[MESSAGE_LEN];
	cout << "Uploading " << filePath << " Size: " << fileSize << " NumPackets: " << totalPackets << endl;

	while (packetsSent < totalPackets){
		file.seekg(filePointer, file.beg);
		filePointer += MESSAGE_LEN;
		if(packetsSent == totalPackets - 1) packetSize = lastPacketSize;
		file.read(payload, packetSize);
		Pacote packet = make_packet(TYPE_DATA, packetsSent + 1, totalPackets, packetSize, payload);
		socket->send(&packet);
		packetsSent++;
	}
	file.close();
}

void Operacoes::sendFile(FuncoesSocket *socket, string filePath, string username){

	string filename = filePath.substr(filePath.find_last_of("/\\") + 1);
	struct stat buffer;   
  	if(stat(filePath.c_str(), &buffer) != 0){
		  Pacote failed = make_packet(TYPE_NOTHING_TO_SEND, 1, 1, -1, "", username.c_str());
		  socket->send(&failed);
		  return;
	}
	Pacote packet = make_packet(TYPE_SEND_FILE_NO_RECORD, 1, 1, -1, filename.c_str(), username.c_str());
	socket->send(&packet);

	ifstream file;
	file.open(filePath, ifstream::in | ifstream::binary);
	file.seekg(0, file.end);
	int fileSize = file.tellg();
	file.seekg(0, file.beg);
	int totalPackets = 1 + ((fileSize - 1) / MESSAGE_LEN); // ceil(x/y)
	int lastPacketSize = fileSize % MESSAGE_LEN;
	int packetsSent = 0;
	int filePointer = 0;
	int packetSize = MESSAGE_LEN;
	char payload[MESSAGE_LEN];
	cout << "Uploading " << filePath << " Size: " << fileSize << " NumPackets: " << totalPackets << endl;

	while (packetsSent < totalPackets){
		file.seekg(filePointer, file.beg);
		filePointer += MESSAGE_LEN;
		if(packetsSent == totalPackets - 1) packetSize = lastPacketSize;
		file.read(payload, packetSize);
		Pacote packet = make_packet(TYPE_DATA, packetsSent + 1, totalPackets, packetSize, payload, username.c_str());
		socket->send(&packet);
		packetsSent++;
	}
	file.close();
}

RegistroDeArquivos Operacoes::sendFileClient(FuncoesSocket *socket, string filePath, string username){

	string filename = filePath.substr(filePath.find_last_of("/\\") + 1);
	struct stat buffer;   
  	if(stat(filePath.c_str(), &buffer) != 0){
		  Pacote failed = make_packet(TYPE_NOTHING_TO_SEND, 1, 1, -1, "", username.c_str());
		  socket->send(&failed);
		  exit(-1);
	}
	Pacote packet = make_packet(TYPE_SEND_FILE_NO_RECORD, 1, 1, -1, filename.c_str(), username.c_str());
	socket->send(&packet);

	ifstream file;
	file.open(filePath, ifstream::in | ifstream::binary);
	file.seekg(0, file.end);
	int fileSize = file.tellg();
	file.seekg(0, file.beg);
	int totalPackets = 1 + ((fileSize - 1) / MESSAGE_LEN); // ceil(x/y)
	int lastPacketSize = fileSize % MESSAGE_LEN;
	int packetsSent = 0;
	int filePointer = 0;
	int packetSize = MESSAGE_LEN;
	char payload[MESSAGE_LEN];
	cout << "Uploading " << filePath << " Size: " << fileSize << " NumPackets: " << totalPackets << endl;

	while (packetsSent < totalPackets){
		file.seekg(filePointer, file.beg);
		filePointer += MESSAGE_LEN;
		if(packetsSent == totalPackets - 1) packetSize = lastPacketSize;
		file.read(payload, packetSize);
		Pacote packet = make_packet(TYPE_DATA, packetsSent + 1, totalPackets, packetSize, payload, username.c_str());
		socket->send(&packet);
		packetsSent++;
	}
	file.close();

	return this->receiveFileRecord(socket);
}

RegistroDeArquivos Operacoes::receiveFileRecord(FuncoesSocket * socket) {
	Pacote * message = socket->receive(TIMEOUT_OFF);
	RegistroDeArquivos * fileRecord = (RegistroDeArquivos*)message->payload;
	return *fileRecord;
}

vector<RegistroDeArquivos> Operacoes::receiveFileList(FuncoesSocket * socket) {
	Pacote *unconvertedFiles;
	RegistroDeArquivos record;
	vector<RegistroDeArquivos> files;
	do {
		unconvertedFiles = socket->receive(TIMEOUT_OFF);
		if(unconvertedFiles->type == TYPE_NOTHING_TO_SEND) break;
		record = *((RegistroDeArquivos*)unconvertedFiles->payload);
		files.push_back(record);
	} while(unconvertedFiles->seq != unconvertedFiles->totalSize);
	return files;
}

void Operacoes::printFileList(vector<RegistroDeArquivos> fileRecords) {
	for(RegistroDeArquivos record : fileRecords) {
		cout << setw(10);
		cout << record.nomeArquivo << " changed time: " << ctime(&record.cTime) << " 	   last modified: " << ctime(&record.mTime) << " 	   last accessed: " << ctime(&record.aTime)<< endl;
	}
}

void Operacoes::receiveUploadAll(FuncoesSocket * socket, string dirPath) {
	while(1) {
		Pacote * packet = socket->receive(TIMEOUT_OFF);
		if (packet->type == TYPE_SEND_UPLOAD_ALL_DONE ||
			packet->type == TYPE_NOTHING_TO_SEND)
			break;
		this->receiveFile(socket, string(packet->payload), dirPath);
	}
}

void Operacoes::sendUploadAll(FuncoesSocket * socket, string dirPath, vector<RegistroDeArquivos> files, string username) {
	if(files.empty()) {
		this->sendNothing(socket);
        return;
    }
	Pacote packet = make_packet(TYPE_SEND_UPLOAD_ALL, 1, 1, -1, "send_upload_all");
	socket->send(&packet);
    int seq = 1;
    for(RegistroDeArquivos record : files) {
		this->sendFile(socket, dirPath + record.nomeArquivo, record, username);
        seq++;
    }
	packet = make_packet(TYPE_SEND_UPLOAD_ALL_DONE, 1, 1, -1, "send_upload_all_done");
	socket->send(&packet);
}

void Operacoes::sendNothing(FuncoesSocket * socket) {
	Pacote packet = make_packet(TYPE_NOTHING_TO_SEND, 1, 1, -1, "nothing_to_send");
	socket->send(&packet);
}

void Operacoes::sendDeleteFile(FuncoesSocket * socket, string filename){
	Pacote request = make_packet(TYPE_DELETE, 1, 1, -1, filename.c_str());
	socket->send(&request);
}

void Operacoes::sendDeleteFile(FuncoesSocket * socket, string filename, string username){
	Pacote request = make_packet(TYPE_DELETE, 1, 1, -1, filename.c_str(), username.c_str());
	socket->send(&request);
}

void Operacoes::deleteFile(string filepath) {
	if(remove(filepath.c_str()) != 0 )
		cout << filepath << endl;
}

void Operacoes::deleteAll(vector<RegistroDeArquivos> files, string dirPath) {
	for(RegistroDeArquivos file : files) {
		this->deleteFile(dirPath + file.nomeArquivo);
	}
}