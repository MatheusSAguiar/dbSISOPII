#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <boost/algorithm/string.hpp>
#include "include/FuncoesSocket.hpp"
#include "include/Pacote.hpp"
#include "include/Constantes.hpp"
#include "include/Cliente.hpp"

using namespace std;


string retornaIp() {
    struct ifaddrs *ifAddrStruct = nullptr;
    getifaddrs(&ifAddrStruct);
    for (struct ifaddrs *ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr != nullptr && ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *addr = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
            const char* localIP = inet_ntoa(addr->sin_addr);
            freeifaddrs(ifAddrStruct);
            return localIP;
        }
    }
    freeifaddrs(ifAddrStruct);
    return "";
}

vector<string> getArguments(){
	string entrada;
	cout << "> ";
	getline(cin, entrada);
	entrada.erase(std::remove(entrada.begin(), entrada.end(), '\n'), entrada.end());
	vector<string> parametros;
	boost::split(parametros, entrada, [](char c){return c == ' ';});
	return parametros;
}

int main(int argc, char *argv[])
{
	bool fimDaSessao = false;
	
	if(argc != 4){
        cout << "Número de parâmetros incorretos.\nExperado: ./client <nome_usuario> <endereço_ip_servidor> <porta_conexão>" << endl;
        return -1;
	}
	string username(argv[1]);
	string serverIP(argv[2]);
	string porta_servidor(argv[3]);

	int porta_Servidor = std::stoi(porta_servidor);
	string localIp = retornaIp();

	dropbox::Cliente client(username, serverIP, porta_Servidor, localIp);

	while(!fimDaSessao){
		
		vector<string> arguments = getArguments();
		if(arguments.size() > 0 && arguments.size() < 3){
			client.lockMutex();
			
			string comando = arguments[0];
			boost::to_upper(comando);
			if (comando == "UPLOAD"){
				if(arguments.size() != 2) puts("Tamanho do comando errado -- upload <file path>");
				else {
					client.sendFile(client.getSocket(), arguments[1], client.getUsername());
				}

			}else if (comando == "DOWNLOAD"){
				if(arguments.size() != 2) puts("Tamanho do comando errado -- download <file path>");
				else {
					client.download(arguments[1]);
				}
			}else if (comando == "DELETE"){
				if(arguments.size() != 2) puts("Tamanho do comando errado -- delete <file path>");
				else {
					client.sendDeleteFile(client.getSocket(), arguments[1], client.getUsername());
				}

			}else if (comando == "LIST_SERVER"){
				if(arguments.size() != 1) puts("Tamanho do comando errado -- list_server");
				else {
					client.requestServerFileList();
				}

			}else if (comando == "LIST_CLIENT"){
				if(arguments.size() != 1) puts("Tamanho do comando errado -- list_client");
				else {
					client.list_client();
				}

			}else if (comando == "GET_SYNC_DIR"){
				if(arguments.size() != 1) puts("Tamanho do comando errado -- get_sync_dir");
				else { 
					client.get_sync_dir();
				}

			}else if(comando == "EXIT"){
				if(arguments.size() != 1) puts("Tamanho do comando errado -- exit");
				else {
					client.exit();
					fimDaSessao = true;
				}

			} else {
				puts("Comando não reconhecido!");
			}

			client.unlockMutex();
		}
	}

	return 0;
}