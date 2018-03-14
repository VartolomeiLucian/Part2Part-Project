
/* CLIENTUL P2P */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <iostream>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <string>
#include <dirent.h>
#include <vector>
#include <sys/stat.h>
#include <time.h>
#include <sys/param.h>

using namespace std;
/* codul de eroare returnat de anumite apeluri */
extern int errno;
struct sockaddr_in server;  /*structura folosita de server */
int sd;                     /* descriptorul de socket */
fd_set readfds; /* multimea descriptorilor de citire */
fd_set actfds;  /* multimea descriptorilor activi */
int maxfds; /* valoarea fd-ului maxim */
struct sockaddr_storage adresaClient;
int port; /* portul de conectare la server*/
char *clientServerPort = NULL;
socklen_t adresalen;
int nFd;
int receive;
struct addrinfo hints,*servInfo,*p;
int serverDis = 0; // verificam daca serverul esti deschis 
string ipVerif, portVerif, fileNameVerif;
int lostConnection;


void *returnAdresa(struct sockaddr *sa)
{
  if(sa->sa_family == AF_INET)
    return &(((struct sockaddr_in*)sa)->sin_addr);
}

int returnPort(struct sockaddr *sa)
{
  return ntohs(((struct sockaddr_in*)sa)->sin_port);
}


void downloadFile(char* ipAddress, char* port, string fName)
{
    char ipAddr[100];
    strcpy(ipAddr, ipAddress);
    char clientPort[100];
    strcpy(clientPort, port);

    if((!strcmp(ipAddr,ipVerif.c_str())) && (!strcmp(clientPort, portVerif.c_str())) && (!fName.compare(fileNameVerif)))
    {

    char buffer[4000];
    memset(&buffer,0,sizeof(buffer));
    struct addrinfo hints, *servClientInfo, *p;
    int ipPno;
    char s[100];
    int clientSocketFd;
    int error=0;
    int connectError = 0;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    lostConnection = 0;

    ipPno = getaddrinfo(ipAddr, clientPort, &hints, &servClientInfo);
    
    if(ipPno < 0)
    {
        printf("[DOWNLOAD] Eroare la adresa IP sau la PORT !\n");
        
    }


    for(p = servClientInfo; p!= NULL; p = p->ai_next)
    {
        clientSocketFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(clientSocketFd < 0)
        {
            printf("[DOWNLOAD] Eroare la socket ! \n");
            
        }

        if(connect(clientSocketFd, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(clientSocketFd);
            printf("[DOWNLOAD] Eroare la connect! \n");
            printf("[DOWNLOAD] Acest client s-a deconectat! \n");
            lostConnection = 1;
        }
    }

    
    if(lostConnection == 0)
    {

    freeaddrinfo(servClientInfo);

    // Trimit numele fisierului la server
    send(clientSocketFd, fName.c_str(), strlen(fName.c_str()), 0);

    FILE *recvFile;
    int size;
    ssize_t dim; 
    string fLocation = "./ClientFiles/" + fName;

    recvFile = fopen(fLocation.c_str(), "w");

    if(recvFile == NULL)
    {
        printf("[DOWNLOAD] Eroare la deschiderea fisierului !\n");
    }

    char bufferCk[15];
    memset(&bufferCk, '\0', sizeof bufferCk);

    if(fork() == 0)
    {
        // se efectueaza download-ul

        while((dim = recv(clientSocketFd, bufferCk, sizeof(bufferCk), 0)) > 0)
            fwrite(bufferCk, 1, dim, recvFile); 

        fclose(recvFile);
        printf("Statusul fisierului %s : SUCCES \n\n",fName.c_str());
        
    }
  
   close(clientSocketFd);
   maxfds = maxfds -1; // decrementam cu 1

   }
   }
   else
    printf("Va rugam sa introduceti corect datele pentru descarcarea fisierului: %s !!\n", fileNameVerif.c_str());

   lostConnection = 0;
}


void acceptClient(int socketFd)
{
    if(socketFd == sd)
    {
        // accept un nou client
        adresalen = sizeof(adresaClient);
        nFd = accept(sd, (struct sockaddr *)&adresaClient, &adresalen);
        char string[200];

        if(nFd < 0)
        {
            perror("[clientServer]: Eroare la accept()! \n");
            exit(1);
        }
        else
        {
            FD_SET(nFd, &actfds);
            if(nFd > maxfds)
                maxfds = nFd;
        }
        return;
    }
    else
    {
        // daca exista conexiunea citim numele fisierului cautat
        char copyBuffer[5000];
        char buffer[5000];
        memset(&copyBuffer,'\0',sizeof(copyBuffer));
        memset(&buffer,'\0',sizeof(buffer));

        if((receive = recv(socketFd, buffer, sizeof(buffer), 0)) <= 0)
        {
            if(receive == 0) /* Conexiunea s-a inchis */
              printf("[clientServer] Un client a inchis conexiunea!\n");

             if(receive < 0)
             {
                perror("[clientServer] Eroare la recv()! \n");
                exit(1);
             }

              close(socketFd);
              FD_CLR(socketFd, &actfds);
        }
        else
        {
            FILE *file = NULL;
            string fileLoc = "./ClientFiles/" + string(buffer);
            file = fopen(fileLoc.c_str(),"r");

            if(!file)
                fprintf(stderr, "Eroare la deschidere: %s", strerror(errno));

            int sentData = 0;
            char bufferCk[20];
            memset(&bufferCk, 0, sizeof(bufferCk));
            int dim; 

            while((dim = fread(bufferCk, 1, sizeof bufferCk, file)) > 0)
            {
                dim = send(socketFd, bufferCk, dim, 0);
                sentData = sentData + dim;
            }

           
            fclose(file);
            close(socketFd);
            FD_CLR(socketFd, &actfds);

        }

    }
}


void clientServer(const char* serverIp)
{
    FD_ZERO(&actfds);   //initializam fd activi
    FD_ZERO(&readfds);  //initiliazam fd pentru citire

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;


    srand(time(0)); // nr random care sa nu se mai repete
    int portClient = rand();
    portClient = (portClient % 61200)+4335;
    sprintf(clientServerPort,"%d", portClient);
    int ipPno;

    if(ipPno = getaddrinfo(serverIp, clientServerPort, &hints, &servInfo) != 0)
    {
        printf("[clientServer] Eroare la adresa IP sau la numarul portului !! \n");
        exit(1);
    }

    for(p=servInfo; p!= NULL; p=p->ai_next)
    {
        if((sd = socket (p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;

        int optval =1;

        if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) < 0)
        {
            printf("[clientServer] Eoare la setsockopt \n");
        }


        if( bind(sd, p->ai_addr, p->ai_addrlen) < 0)
        {
            printf("[clientServer] Eroare la bind() !\n");
            continue; 
        }

        break;
    }


    if(p == NULL)
    {
        printf("[clientServer] Eroare la bind () !\n");
        exit(3);
    }

    freeaddrinfo(servInfo);
    if(listen(sd, 15) < 0)
    {
        printf("[clientServer] Eroare la listen() !\n");
        exit(4);
    }

    FD_SET(sd, &actfds);
    maxfds = sd;

    for(;;)
    {
       readfds = actfds;

       if(select(maxfds+1, &readfds, NULL, NULL, NULL) < 0)
       {
         printf("[clientServer] Eroare la select() !\n");
         exit(1);
       }

         for(int fd = 0; fd <= maxfds; fd++) /* parcurgem multimea de descriptori */
         {
           if( FD_ISSET(fd, &readfds))
           {
              acceptClient(fd);
           }
         }
    }
}

void SearchFileType(int socketFd)
{
    printf("Cautati fisierul de tipul:   ");
    string fileToSearch;
    cin>>fileToSearch;
    string searchOpt ="5 " + fileToSearch;
    //cout<<"Fisierul cautat este: "<< fileToSearch.c_str() <<endl;

    // Trimit cautarea la server

    bool type = true;
    if(fileToSearch[0] != '.')
    type = false;


    if(type == true)
    {

    send(socketFd, searchOpt.c_str(), strlen(searchOpt.c_str()),0);
    char listFromServer[4000];
    memset(&listFromServer, '\0', sizeof(listFromServer));
    char str[5000];
    memset(&str,'\0',sizeof(str));


    recv(socketFd, listFromServer, sizeof(listFromServer), 0);

  if(strlen(listFromServer))
  {
    if(strcmp(listFromServer, "Nu exista in sistem fisiere de acest tip!"))
    {

      strcpy(str, listFromServer);
      memset(listFromServer, 0, sizeof(listFromServer));

      vector<string> filesFromServer; // cream un vector de string-uri unde vom stoca numele fisierelor primite
      char *p = strtok(str," ");

    while(p)
    {
        string tmp = p;
        filesFromServer.push_back(tmp);
        p = strtok(NULL, " ");
    }

     memset(str,0,strlen(str));

      vector<string>::iterator it;
      for(it=filesFromServer.begin(); it!=filesFromServer.end(); ++it)
     {
         cout<< *it<<"  "<<'\t';

         cout<<endl;
      }

    }
    else
    {
        // Daca nu exista fisierul in sistem
        cout<<listFromServer<<endl;
    }
  }
  else
     {
       printf("SERVERUL este inchis !\n");
       serverDis=1;
     }
  }
  else
  {
    printf("Ati introdus gresit tipul de fisier! (ex.: .txt, .xml) \n");
  }

}

int SearchFileBytes(int socketFd)
{
    printf("Cautati fisiere cu dimensiunea:  ");
    string fileToSearch;
    cin>>fileToSearch;
    string searchOpt ="6 " + fileToSearch;
    cout<<"Dimensiunea dorita este: "<< fileToSearch.c_str() <<endl;

    // Trimit cautarea la server

    bool hasOnlyDigits = true;
    for(size_t i = 0; i <fileToSearch.length(); i++)
    {
      if(!isdigit(fileToSearch[i]))
      {
         hasOnlyDigits = false;
         break;
      }
    }

    if(hasOnlyDigits == true)
    {

    send(socketFd, searchOpt.c_str(), strlen(searchOpt.c_str()),0);
    char listFromServer[4000];
    memset(&listFromServer, '\0', sizeof(listFromServer));
    char buffer[5000];
    memset(&buffer,'\0',sizeof(buffer));


    recv(socketFd, listFromServer, sizeof(listFromServer), 0);

  if(strlen(listFromServer))
  {
    if(strcmp(listFromServer, "Nu exista in sistem fisiere cu aceasta dimensiune!"))
    {

      strcpy(buffer, listFromServer);
      memset(listFromServer, 0, sizeof(listFromServer));

      vector<string> filesFromServer; // cream un vector de string-uri unde vom stoca numele fisierelor primite
      char *p = strtok(buffer," ");

    while(p)
    {
        string tmp = p;
        filesFromServer.push_back(tmp);
        p = strtok(NULL, " ");
    }

     memset(buffer,0,strlen(buffer));

      vector<string>::iterator it;
      printf("Fisierele cu diminesiunea %s gasite: \n", fileToSearch.c_str());
      for(it=filesFromServer.begin(); it!=filesFromServer.end(); ++it)
      {
         cout<< *it<<"  "<<'\t';

         cout<<endl;
      }

    }
    else
    {
        // Daca nu exista fisierul in sistem
        cout<<listFromServer<<endl;
        
    }
  }
  else
  {
      printf("SERVERUL este inchis !\n");
      serverDis=1;
  }
 }
 else
 {
    printf("Introduceti doar cifre! \n");
 }

}




void SearchNameFile(int socketFd)
{
    printf("Introduceti numele fisierul: ");
    string fileToSearch;
    cin>>fileToSearch;
    string searchOpt ="4 " + fileToSearch;
    //cout<<"Fisierul cautat este: "<< fileToSearch.c_str() <<endl;

    // Trimit cautarea la server

    send(socketFd, searchOpt.c_str(), strlen(searchOpt.c_str()),0);
    char listFromServer[4000];
    memset(&listFromServer, '\0', sizeof(listFromServer));
    char buffer[5000];
    memset(&buffer,'\0',sizeof(buffer));


    recv(socketFd, listFromServer, sizeof(listFromServer), 0);

  if(strlen(listFromServer))
  {
    if(strcmp(listFromServer, "Nu exista in sistem fisiere cu acest nume!"))
    {

      strcpy(buffer, listFromServer);
      memset(listFromServer, 0, sizeof(listFromServer));

      vector<string> filesFromServer; // cream un vector de string-uri unde vom stoca numele fisierelor primite
      char *p = strtok(buffer," ");

    while(p)
    {
        string tmp = p;
        filesFromServer.push_back(tmp);
        p = strtok(NULL, " ");
    }

     memset(buffer,0,strlen(buffer));

      vector<string>::iterator it;
      for(it=filesFromServer.begin(); it!=filesFromServer.end(); ++it)
     {
         cout<< *it<<"  "<<'\t';

         cout<<endl;
      }

    }
    else
    {
        // Daca nu exista fisierul in sistem
        cout<<listFromServer<<endl; 
    }
  }
  else
     {
       printf("SERVERUL este inchis !\n");
       serverDis=1;
     }

}

void SearchFile(int socketFd)
{
    printf("Cautati fisierul cu numele: \n");
    string fileToSearch;
    cin>>fileToSearch;
    
    string searchOpt ="2 " + fileToSearch;
    //cout<<"Fisierul cautat este: "<< fileToSearch.c_str() <<endl;

    // Trimit cautarea la server

    send(socketFd, searchOpt.c_str(), strlen(searchOpt.c_str()),0);
    char addressFromServer[1000];
    memset(&addressFromServer, '\0', sizeof(addressFromServer));

    recv(socketFd, addressFromServer, sizeof(addressFromServer), 0);


    if(strlen(addressFromServer))
    {   if(strcmp(addressFromServer, "Fisierul cautat nu exista in sistem!"))
        {
           cout<<"Clientul ce detine fisierul pe care l-ati cautat are urmatoarea adresa: "<< addressFromServer <<endl<<endl;
           
           /* Verificare pentru descarcare */

           vector<string> verifAddress;
           char verifAdd[4000];
           memset(&verifAdd,'\0',sizeof(verifAdd));
           strcpy(verifAdd, addressFromServer);
 
           char *p=strtok(verifAdd," ");

           while(p)
           {
              string tmp = p;

              verifAddress.push_back(tmp);
              p = strtok(NULL, " ");
           }

           memset(verifAdd, 0, strlen(verifAdd));

           vector<string>::iterator it;

           ipVerif = verifAddress.at(0);
           portVerif = verifAddress.at(1);
           fileNameVerif = fileToSearch;
        }
       else
       {
           // Daca nu exista fisierul in sistem
           cout<<addressFromServer<<endl;    
       }
    }
    else
    {
        printf("SERVERUL este inchis !\n");
        serverDis=1;
    }

}


int publish(int socketFd)
{
    DIR *director;
    struct dirent *dir;
    string stringOption ="1"; /* pentru publicarea fisierelor */
    struct stat fileinfo;


    director = opendir("ClientFiles");

    if(director)
    {
        printf("Fisierele publicate in retea sunt : \n\n");
        printf("<----------------------------------------------------------->\n");

        while((dir = readdir(director)) != NULL)
        {
            if(!strcmp(dir->d_name,".") || !strcmp(dir->d_name,".."))
                continue;

          

            char file_path[MAXPATHLEN];
            snprintf(file_path, MAXPATHLEN, "%s/%s", "ClientFiles", dir->d_name);
            if (!stat(file_path, &fileinfo))
            {
               if (S_ISREG(fileinfo.st_mode))
               {
                  printf("Nume fisier:              %s\n", dir->d_name);
                  printf("Dimensiune fisier:        %zu bytes\n", fileinfo.st_size);
                  printf("Ultima accesare:          %s\n", ctime(&fileinfo.st_atime));
               }
            }

            char size[256];
            sprintf(size,"%zu",fileinfo.st_size);
            strcat(dir->d_name, "--->");
            strcat(dir->d_name, size); // atasez si dimensiunea fisierului
            stringOption=stringOption+" "+ dir->d_name;

        }

        send(socketFd, stringOption.c_str(), strlen(stringOption.c_str()),0);
        closedir(director);
    }
    return 1;
}





int main (int argc, char *argv[])
{

  clientServerPort = (char*)mmap(NULL, sizeof *clientServerPort, PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  setpgid(0,0);

  if(fork())
  {
      clientServer(argv[3]);
  }
  else
  {

  int sd, opt;
  char buff[500];
  struct addrinfo hints, *servinfo, *p;
  int ipPno;
  char s[100];
  int publicOpt=0;

  if(argc != 4)
  {
    printf("Numar incorect de argumente! \n");
    exit(1);
  }


   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;

 if((ipPno = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) < 0)
 {
   printf("[SERVER] Eroare la adresa IP sau la numarul portului !!\n");
   return 1;
 }


  for(p = servinfo; p!=NULL; p=p->ai_next)
  {
     if((sd = socket(p->ai_family, p->ai_socktype ,p->ai_protocol)) < 0)
     {
       perror("[CLIENT] Eroare la socket()!\n");
       return errno;
     }

     if(connect(sd, p->ai_addr, p->ai_addrlen) < 0)
     {
        close(sd);
        perror("[CLIENT] Eroare la connect()!\n");
        return errno;
     }

     break;

  }

  if( p == NULL )
  {
    printf("[CLIENT] Eroare la connect() !\n");
    return errno;
  }

  inet_ntop(p->ai_family, returnAdresa((struct  sockaddr *)p->ai_addr),s, sizeof s);
  freeaddrinfo(servinfo);

  string peerServAddr = "8 "+string(argv[3]) +" "+string(clientServerPort);
  send(sd, peerServAddr.c_str(), strlen(peerServAddr.c_str()),0);

  if((opt = recv(sd, buff, 500, 0)) < 0)
  {
    perror("[CLIENT] Eroare la recv() !\n");
    return errno;
  }
  else
  {
    printf("\n");
    printf("ESTI CONECTAT LA SERVERUL CENTRAL!\n\n");
    printf("<----------------------------------------------->\n");
    printf("%s\n",buff);
    LoopBack:
    printf("<----------------------MENIU-------------------->\n\n");
    printf("Tasteaza 1 pentru Publicarea Fisierelor \n");
    printf("Tasteaza 2 pentru Cautare ADRESA si PORT \n");
    printf("Tasteaza 3 pentru Descarcare \n");
    printf("Tasteaza 4 pentru Cautare dupa NUME \n");
    printf("Tasteaza 5 pentru Cautare dupa TIP (ex.: .txt) \n");
    printf("Tasteaza 6 pentru Cautare dupa DIMENSIUNE (in bytes): \n");
    printf("Tasteaza 7 pentru Deconectare \n\n");
    printf("Tasteaza optiunea dorita:   \n");


    int option;

    scanf("%d",&option);
    if(option > 7)
    {
      printf("Nu exista aceasta optiune! \n");
      option = 0;
      goto LoopBack;
    }

    switch(option)
    {

       case 1:
       {
         ++publicOpt;
         if(publicOpt > 1)
         {
            printf("Ati efectuat optiunea de publicare a fisierelor! \n");
            goto LoopBack;
         }
         else
         {
            publish(sd);
            goto LoopBack;
          }
       }
       case 2:
       {
        
           SearchFile(sd);
           if(serverDis == 0)
            goto LoopBack;
           else
           {
             kill(0, SIGINT);
             kill(0, SIGKILL);
             exit(1);
           }   
       }
       case 3:
       {
          char ip[100];
          char port[100];
          string file;

          cout<<"IP: "<<endl;
          cin>>ip;
          cout<<"PORT: "<<endl;
          cin>>port;
          cout<<"Numele fisierului: "<<endl;
          cin>>file;

          downloadFile(ip,port,file);
          goto LoopBack;
       }
       case 4:
       {
          SearchNameFile(sd);
          if(serverDis == 0)
            goto LoopBack;
          else
          {
             kill(0, SIGINT);
             kill(0, SIGKILL);
             exit(1);
          }
       }
       case 5:
       {
          SearchFileType(sd);
          if(serverDis == 0)
            goto LoopBack;
          else
          {
             kill(0, SIGINT);
             kill(0, SIGKILL);
             exit(1);
          }
       }
       case 6:
       {
          SearchFileBytes(sd);
          if(serverDis == 0)
            goto LoopBack;
          else
          {
             kill(0, SIGINT);
             kill(0, SIGKILL);
             exit(1);
          }
       }
       case 7:
       {
          kill(0, SIGINT);
          kill(0, SIGKILL);
          exit(1);
       }
    
   }
  }

  buff[opt] = '\0';
  getchar();
  }
 return 0;
}
