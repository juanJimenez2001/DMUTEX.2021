/* DMUTEX (2009) Sistemas Operativos Distribuidos
 * C�digo de Apoyo
 *
 * ESTE C�DIGO DEBE COMPLETARLO EL ALUMNO:
 *    - Para desarrollar las funciones de mensajes, reloj y
 *      gesti�n del bucle de tareas se recomienda la implementaci�n
 *      de las mismas en diferentes ficheros.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <pthread.h>
#include <fcntl.h>

enum { MSG, LOCK, OK };

typedef struct puerto{
  char proceso[20];
  int puerto;
}puerto;

typedef struct mensaje{
  int tipo;
  char proceso[20];
  int reloj[10];
}mensaje;

int main(int argc, char* argv[]){
  int port;
  char line[80],proc[80],accion[20];
  struct sockaddr_in dir;
  socklen_t addr = sizeof(dir);
  int n=0;
  int IDproc;
  struct puerto listPuerto[10];
  int s;
  int puerto_udp;

  if(argc<2){
    fprintf(stderr,"Uso: proceso <ID>\n");
    return 1;
  }
  /* Establece el modo buffer de entrada/salida a l�nea */
  setvbuf(stdout,(char*)malloc(sizeof(char)*80),_IOLBF,80);
  setvbuf(stdin,(char*)malloc(sizeof(char)*80),_IOLBF,80);
  if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("Error creando socket.\n");
    return -1;
  }
  dir.sin_addr.s_addr=INADDR_ANY;
  dir.sin_port=htons(0);
  dir.sin_family=AF_INET;
  if (bind(s, (struct sockaddr *)&dir, sizeof(dir)) < 0) {
    perror("Error en bind.\n");
    close(s);
    return -1;
  }
  if (getsockname(s, (struct sockaddr *)&dir, &addr) < 0){
    fprintf(stderr, "Error en getsockname.\n");
    close(s);
    return -1;
  }

  puerto_udp=ntohs(dir.sin_port);
  fprintf(stdout,"%s: %d\n",argv[1],puerto_udp);

  for(;fgets(line,80,stdin);){
    if(!strcmp(line,"START\n"))
      break;
    sscanf(line,"%[^:]: %d",proc,&port);
    /* Habra que guardarlo en algun sitio */
    listPuerto[n].puerto=port;
    strcpy(listPuerto[n].proceso, proc);
    if(!strcmp(proc,argv[1])){
       /* Este proceso soy yo */
      IDproc=n;
    }
    n++;
  }

  /* Inicializar Reloj */
  int reloj[n];
  for(int i=0; i<n; i++)
    reloj[i]=0;

  /* Procesar Acciones */
  for(;fgets(line,80,stdin);){

    sscanf(line, "%s %s", accion, proc);
    struct mensaje m;
    struct sockaddr_in dir1;
    socklen_t addr1 = sizeof(dir1);

    if(!strcmp(line,"EVENT\n")){
      reloj[IDproc]++;
      printf("%s: TICK\n", listPuerto[IDproc].proceso);
    }

    else if(!strcmp(line,"GETCLOCK\n")){
      printf("%s: LC[",listPuerto[IDproc].proceso);
      for(int i=0; i<n-1; i++){
        printf("%d,",reloj[i]);
      }
      printf("%d]\n",reloj[n-1]);
    }

    else if(!strcmp(accion,"MESSAGETO")){
      reloj[IDproc]++;
      for(int i = 0; i < n; i++){
        if(!strcmp(listPuerto[i].proceso, proc)){
          dir1.sin_family = AF_INET;
          dir1.sin_addr.s_addr = INADDR_ANY;
          dir1.sin_port = htons(listPuerto[i].puerto);
          break;
        }
      }
      strcpy(m.proceso, listPuerto[IDproc].proceso);
      m.tipo = MSG;
      for(int i = 0; i < n; i++){
        m.reloj[i] = reloj[i];
      }
      if(m.tipo==MSG){
        sendto(s, &m, sizeof(struct mensaje), 0, (struct sockaddr *)&dir1, addr1);
        printf("%s: TICK|SEND(MSG,%s)\n",listPuerto[IDproc].proceso, proc);
      } 
    }

    else if(!strcmp(accion,"RECEIVE")){
      if(recv(s, &m, sizeof(struct mensaje), 0)<0){
        perror("Error en el recv.\n");
        return -1;
      }
      for(int i=0; i<n; i++){
        if (i != IDproc) {
          if (m.reloj[i] > reloj[i]) {
            reloj[i] = m.reloj[i];
          }
        } else{
            reloj[IDproc]++;
        }
      }
      if(m.tipo==MSG){
        printf("%s: RECEIVE(MSG,%s)|TICK\n",listPuerto[IDproc].proceso, m.proceso);
      }
    }

    else if(!strcmp(line,"FINISH\n")){
      break;
    }

  }
  return 0;
}



