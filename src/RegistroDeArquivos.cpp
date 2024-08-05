#include "../include/RegistroDeArquivos.hpp"

using namespace std;
using namespace dropbox;

RegistroDeArquivos dropbox::criarRegistro(const char *nomeArquivo, time_t mTime, time_t aTime, time_t cTime, int tamanho) {
    
    RegistroDeArquivos registro;
    strcpy(registro.nomeArquivo, nomeArquivo);
    registro.mTime = mTime;
    registro.aTime = aTime;
    registro.cTime = cTime;
    registro.tamanho = tamanho;

    return registro;
}