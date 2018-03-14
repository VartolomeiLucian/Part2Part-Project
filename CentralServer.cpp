/*SERVERUL CENTRAL P2P */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <cstddef>
#include <algorithm>
#include <iterator>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <map>
#include <iostream>
#include <utility>
#include <fstream>
#include <vector>
#include <math.h>
#include <string>
#include <glob.h>

/*PORTUL SERVERULUI CENTRAL */
#define PORT 3500

using namespace std;

int sd, client; /* descriptorii de socket */
fd_set readfds; /* descriptori de citire */
fd_set actfds;  /* descriptori activi */
int optval = 1; /* optiunea pentru setsockopt */
int fd; /* descriptor pentru parcurgerea listelor de descriptori*/
int maxfds; /* valoarea fd-ului maxim */
int len; /* lungimea structurii sockaddr_in */
struct sockaddr_in server; /* structura foloista de server */
struct sockaddr_in from;  /* structura folosita de client */
struct sockaddr_storage adresaClient;
struct addrinfo hints, *servInfo, *p;
int clients = 0; 
socklen_t adresalen;
int nFd; /* noul descriptor -> pentru conexiune */
int opt;
map<string, vector<int> > dataBase;
map<int,pair<string,int> > clientInfo;
int clientsNr=0;


void *returnAddress(struct sockaddr *sa)
{
    if(sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);
}

int returnPort(struct sockaddr *sa)
{
    return ntohs(((struct sockaddr_in*)sa)->sin_port);
}



void serverConnection(int fd)
{
    if(fd == sd)
    {
        /*acceptam o conexiune noua */
        adresalen=sizeof(adresaClient);

        nFd = accept (sd,(struct sockaddr*)&adresaClient, &adresalen);
        char ipAddr[200];

        /* Facem convertirea adresei din binar in text */
        inet_ntop(adresaClient.ss_family,returnAddress((struct sockaddr *)&adresaClient), ipAddr, sizeof ipAddr);


       
        printf("[SERVER]: S-a conectat clientul cu portul: %d si adresa: %s \n", returnPort((struct sockaddr *)&adresaClient), ipAddr);
        if(nFd < 0)
        {
            perror("[SERVER]: Eroare la accept()! \n");
            exit(1);
        }
        else
        {

            FD_SET(nFd,&actfds);

          
            if(nFd > maxfds)
                maxfds = nFd;

        }

        if(send(nFd,"Bine ati venit in Sistemul P2P!", strlen("Bine ati venit in Sistemul P2P!"),0) < 0)
        {
            printf("[SERVER] Eroare la send() !");
           
        }
        return;
    }
    else
    {    

        char ipAddr[400];
        inet_ntop(adresaClient.ss_family, returnAddress((struct sockaddr *)&adresaClient), ipAddr, sizeof ipAddr);
        int portNumber = returnPort((struct sockaddr *)&adresaClient);

        char bufferCopy[5000];
        char buffer[5000];
        memset(&bufferCopy,'\0',sizeof(bufferCopy));
        memset(&buffer,'\0',sizeof(buffer));


       /* eliminam informatiile unui client cand se deconecteaza */

        if((opt = recv(fd, buffer, sizeof buffer, 0)) <= 0)
        {

            if(opt == 0) /* Conexiunea s-a inchis */
            {  
               printf("[SERVER] Un client s-a deconectat!\n");
               
               clients--;
            }

            if(opt < 0)
            {
                perror("[SERVER] Eroare la recv()! \n");
                exit(1);
            }

            close(fd);
            FD_CLR(fd, &actfds);


           map<string, vector<int> >::iterator it;

           for(it = dataBase.begin(); it != dataBase.end(); ++it)
            {

                vector<int>::iterator delit = find((it->second).begin(), (it->second).end(), fd);
                if(delit != (it->second).end())
                    (it->second).erase(delit); 

            }

            clientInfo.erase(fd);

            if (clients == 0)
            {
              dataBase.clear();
              printf("Nu exista fisiere in baza de date a serverului! \n");
            }
        }
        else
        {
            strcpy(bufferCopy, buffer);
            memset(buffer,0,strlen(buffer));


            vector<string> Files; /* cream un vector de string-uri pentru a stoca numele fisierelor */
            char *p=strtok(bufferCopy," ");

            while(p)
            {
                string aux=p;
                Files.push_back(aux);
                p=strtok(NULL, " ");
            }

            memset(bufferCopy,0,strlen(bufferCopy));


            /* 8 - stocam ip-ul si portul */

            if(*(Files.begin()) == "8")
            {
                
                clients++; // crestem numarul de clienti

                string IP = Files.at(1); // IP-ul
                string portNumb = Files.at(2); //Portul

                pair <string, int> clientIpPort (IP, atoi(portNumb.c_str()));
                clientInfo.insert( pair<int, pair<string, int> >(fd, clientIpPort));

            
             // printf("Clienti conectati: %d clienti\n", clients);
              cout<<"Clienti conectati: "<<clients<<endl<<endl;
              cout<<"---------------------------"<<endl;
              cout<< "FD"<<" "<<"ADRESA"<<"     "<<"Port"<<endl<<endl;

             for(map<int, pair<string, int> >::iterator it = clientInfo.begin(); it != clientInfo.end(); ++it)
             {
                cout<< it->first<<"  "<< it->second.first <<"  "<< it->second.second << endl;
              
             }

             cout<<"---------------------------"<<endl<<endl;
               

             }

            else
                if(*(Files.begin()) == "1") // publicam numele fisierelo in server
                {

                    cout<<"---------------------------"<<endl;
                    cout<<"NUME FISIER: "<<"   "<<"FD: "<<endl<<endl;

                    vector<string>::iterator it;
                    
                    for(it=Files.begin()+1; it!=Files.end(); ++it)
                    {
                        /* adaug numele fisierelor si id-ul socket-ului in baza de date */
                        dataBase[*it].push_back(fd);
                    }

                    

                    /* Afiseaza numele fisierelor si descriptorul socketului */

                    map<string, vector<int> >::iterator it1;

                    for(it1=dataBase.begin(); it1!=dataBase.end(); ++it1)
                    {
                     // cout<< it1->first<<"  "<<'\t';
                       size_t position = it1->first.find('-');
                       cout<<it1->first.substr(0,position)<<"   ";

                      for(vector<int>::iterator it2 = it1-> second.begin(); it2 != it1->second.end(); ++it2)
                        cout<<'\t'<<*it2;

                        cout<<endl;
                    }

                    cout<<"---------------------------"<<endl<<endl;

                }

            else
                if(*(Files.begin()) == "2")
                {
                    // Fisierul care este cautat
                    
                    string fileToSearch = Files.at(1);
                    int find=0;
                   // printf("[server] fisierul cautat este: %s\n", fileToSearch.c_str()); //test

                     int pos = fileToSearch.rfind(".");
                               
                     if(!(pos == string::npos))
                      find = 1;

                    
                      map<string, vector<int> >::iterator it = dataBase.begin();
                      vector<int> fdClients;


                // Verific daca exista fisierul
                 if(find == 1)
                 {
                    if(it != dataBase.end())
                    {

                      char *type = new char[fileToSearch.length() + 1];
                        strcpy(type, fileToSearch.c_str());
                       
                        string::size_type n;
                        map<string, vector<int> >::iterator it = dataBase.begin();
                        string stringType;
                        

                        if(it != dataBase.end())
                        {
                             map<string, vector<int> >::iterator it1;
                             for(it1=dataBase.begin(); it1!=dataBase.end(); ++it1)
                             {
                               // cout<< it1->first<<"  "<<'\t';
                                string copy = it1->first; 

                                n = copy.rfind(type);
                                //cout<<"n: "<<n<<endl;

                                if(!(n==string::npos))
                                {
                                    //cout<<"Gasit! "<<endl;
                                    
                                     vector<int> inVect =(*it1).second;
                                  for(int i=0; i<inVect.size(); i++)
                                  {
                                     fdClients.push_back(inVect[i]);
                                    
                                     clientsNr = inVect.size();
                                    }
                                    cout<<endl;
                                }

                              }
                        }

                        int clientSel=0;
                        
                   
                        // Selectez random un client care are fisierul cautat
                       
                        if(clientsNr > 0) 
                        {
                           clientSel = rand() % clientsNr;
                        
                        // Stochez fd-ul clientului care are fisierul cautat

                        int sfdClient = fdClients.at(clientSel);

                        map<int, pair<string, int> >::iterator iter = clientInfo.find(sfdClient);

                        char portClient[2000];
                        string adresaSearch;
                        snprintf(portClient, sizeof(portClient), "%d", iter->second.second); // Selectez portul din map
                        adresaSearch = iter->second.first+" "+ portClient;
                      //  printf("[server] search: %s\n", adresaSearch.c_str()); // test
                        send(fd, adresaSearch.c_str(), strlen(adresaSearch.c_str()), 0);
                        }
                        else
                      {
                        // Daca fisierul nu exista

                        send(fd, "Fisierul cautat nu exista in sistem!", strlen("Fisierul cautat nu exista in sistem!"), 0);
                        clientsNr=0;
                      }

                      clientsNr=0;
                      
                    }
                }
                else
                {
                        // Daca fisierul nu exista

                     send(fd, "Fisierul cautat nu exista in sistem!", strlen("Fisierul cautat nu exista in sistem!"), 0);
                }
              }
                else
                    if(*(Files.begin()) == "4")
                    {

                        // CAUTARE DUPA NUME --> Sugestii 

                        string fileToSearch = Files.at(1);

                        vector<string> filesType;

                        char *type = new char[fileToSearch.length() + 1];
                        strcpy(type, fileToSearch.c_str());

                        string::size_type n;
                        map<string, vector<int> >::iterator it = dataBase.begin();
                        string stringType;
                        int exist=0;

                        if(it != dataBase.end())
                        {
                             map<string, vector<int> >::iterator it1;
                             for(it1=dataBase.begin(); it1!=dataBase.end(); ++it1)
                             {
                               
                                string copy = it1->first; 

                                n = copy.rfind(type);
                                   

                                if(!(n==string::npos))
                                {
                                    size_t position = copy.find('-');
                                    
                                    filesType.push_back(copy.substr(0,position));
                                    exist++;
                                   // cout<<"Ce se adauga: "<<copy.substr(0,position)<<endl;
                                }
                              }
                        }


                        if(exist > 0)
                        {
                             vector<string>::iterator it3;
                             for(it3 = filesType.begin(); it3 != filesType.end(); ++it3)
                             {
                                //cout<<"Fisierele stocate in filesType: "<<*it3<<endl;

                                 stringType = stringType + " " + *it3;

                             }

                             send(fd, stringType.c_str(), strlen(stringType.c_str()), 0);
                        }
                        else
                        {
                           send(fd, "Nu exista in sistem fisiere cu acest nume!", strlen("Nu exista in sistem fisiere cu acest nume!"), 0);
                        }

                    }
                    else
                    if(*(Files.begin()) == "5")
                    {

                        // CAUTARE DUPA TIPUL DE FISIER

                        string fileToSearch = Files.at(1);

                        vector<string> filesType;

                        char *type = new char[fileToSearch.length() + 1];
                        strcpy(type, fileToSearch.c_str());
                        //cout<< "Type: "<<type<<endl;

                        string::size_type n;
                        map<string, vector<int> >::iterator it = dataBase.begin();
                        string stringType;
                        int exist=0;

                        if(it != dataBase.end())
                        {
                             map<string, vector<int> >::iterator it1;
                             for(it1=dataBase.begin(); it1!=dataBase.end(); ++it1)
                             {
                              
                                string copy = it1->first; // fiecare fisier pe rand

                                n = copy.rfind(type);
                              
                                  
                                if(!(n==string::npos))
                                {
                                   
                                    size_t position = copy.find('-');
                                    
                                    filesType.push_back(copy.substr(0,position));

                                    exist++;
                                }
                              }
                        }


                        if(exist > 0)
                        {
                             vector<string>::iterator it3;
                             for(it3 = filesType.begin(); it3 != filesType.end(); ++it3)
                             {
                        
                                 stringType = stringType + " " + *it3;

                             }

                             send(fd, stringType.c_str(), strlen(stringType.c_str()), 0);
                        }
                        else
                        {
                           send(fd, "Nu exista in sistem fisiere de acest tip!", strlen("Nu exista in sistem fisiere de acest tip!"), 0);
                        }

                    }
                    else
                      if(*(Files.begin()) == "6")
                      {   

                        // CAUTARE DUPA DIMENSIUNE



                        string sizeToSearch = Files.at(1);

                            vector<string> fileSize;

                            char *size = new char[sizeToSearch.length() + 1];
                            strcpy(size, sizeToSearch.c_str());
                            //cout<< "Dimensiunea: "<<size<<endl;

                            string::size_type n;
                            map<string, vector<int> >::iterator it = dataBase.begin();
                            string stringSize;
                            int exist;

                            if(it != dataBase.end())
                            {
                                 map<string, vector<int> >::iterator it1;
                                 for(it1=dataBase.begin(); it1!=dataBase.end(); ++it1)
                                 {
                                    
                                    string copy = it1->first; 

                                    n = copy.rfind(size);
                                  
                                  
                                    if(!(n==string::npos))
                                    {
                                        size_t position = copy.find('-');
                                      
                                        fileSize.push_back(copy.substr(0,position));
                                        exist++;
                                    }
                                  }
                            }

                            if(exist > 0)
                            {
                             vector<string>::iterator it3;
                             for(it3 = fileSize.begin(); it3 != fileSize.end(); ++it3)
                             {
                            

                                 stringSize = stringSize + " " + *it3;

                             }

                             send(fd, stringSize.c_str(), strlen(stringSize.c_str()), 0);
                            }
                            else
                            {
                           send(fd, "Nu exista in sistem fisiere cu aceasta dimensiune!", strlen("Nu exista in sistem fisiere cu aceasta dimensiune!"), 0);
                            }
                        }
       
         }

    }

}




int main(int argc, char const *argv[])
{
    FD_ZERO(&actfds);   //initializam fd activi
    FD_ZERO(&readfds);  //initializam fd pentru citire

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int ipPno;

    if(ipPno = getaddrinfo(argv[1],argv[2], &hints, &servInfo) < 0) //apelam getaddrinfo cu argv[1]: IP-ul si argv[2]: numarul Portului
    {
        printf("[SERVER] Eroare la adresa IP sau la numarul portului !! %s\n", gai_strerror(ipPno));
        return 1;
    }


    for(p=servInfo; p != NULL; p=p->ai_next)
    {
        if((sd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0 )
            continue;

        if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) < 0)
        {
            perror("[SERVER] Eoare la setsockopt %s\n");
            return errno;
        }


        if(bind(sd, p->ai_addr, p->ai_addrlen) < 0)
        {
            perror("[SERVER] Eroare la bind() !\n");
            continue;
        }

        break;

        if(p == NULL)
        {
            printf("[SERVER] Eroare la bind () !\n");
            return errno;
        }

    }


    printf("<-----------------------Part2Part Server----------------------->\n\n");
    


    freeaddrinfo(servInfo);

    if(listen(sd, 15) < 0)
    {
        perror("[SERVER] Eroare la listen() !%s\n");
        return errno;
    }

    FD_SET(sd,&actfds);
    maxfds = sd;


    /* serverul asteapta noi clienti */
    for(;;)
    {
        readfds = actfds;

        if(select(maxfds+1, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("[SERVER] Eroare la select() !\n");
            return errno;
        }

        for(fd = 0; fd <= maxfds; fd++) /* parcurgem multimea de descriptori */
        {
            if(FD_ISSET(fd, &readfds))
            {
                serverConnection(fd);
            }
        }
    }

   

    return 0;
}
