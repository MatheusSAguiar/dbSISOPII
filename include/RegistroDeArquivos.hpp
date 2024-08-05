#ifndef __REGISTRO_DE_ARQUIVOS_HPP__
#define __REGISTRO_DE_ARQUIVOS_HPP__

#include <string>
#include <string.h>
#include <time.h>

using namespace std;

namespace dropbox {

  struct RegistroDeArquivos
  {
    char nomeArquivo[200];
    time_t mTime;
    time_t aTime;
    time_t cTime;
    int tamanho;
  };

  typedef RegistroDeArquivos RegistroDeArquivos;

  RegistroDeArquivos criarRegistro(const char *nomeArquivo, time_t mTime, time_t aTime, time_t cTime, int tamanho);

}

#endif