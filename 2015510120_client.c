#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>

#define MSG_SIZE 2048//mesaj char arrayimin boyutu
#define port_no 3205


bool ex_flag=false;//send mesajda exit komutu gelirse receivede ki döngüyüde kırmak
void send_message(int sock);//mesaj gönderecek fonksiyon parametre olarak socketin send bağlantısını alıyor
void get_message(int sock) ;
void fix_message(char msg[MSG_SIZE])//enter a bastımızda mesaj servera gindce \n bu işaretide yolluyor bunu engellemek için
{
  int i=0;
  while(msg[i]!='\n')
  {
    i++;
    if(i+1>=MSG_SIZE)//eğer mesajda hiç '\n' bu işaretten yoksa sonsuz döngüde kalmamak için
    {
      break;
    }
  }
  msg[i]='\0';
}
int main(int argc, char *argv[])
{
    int socket_desc;
    struct sockaddr_in server;
     
    //Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1)
    {
        puts("Could not create socket");
        return 1;
    }
         
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_family = AF_INET;
    server.sin_port = htons(port_no);
    //Connect to remote server
    if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
         puts("Connection error");
         return 1;
    }
   ///////////////////////////////////////////////////////////////////////////////////////// 
    char name[30];
    fflush(stdout);
    printf("isminizi yazınız: ");
    fgets(name,1024,stdin);
    printf("Dont fogert you cant create the same name room\n");
    fix_message(name);
    int send_m=send(socket_desc,name,strlen(name),0);
    if(send_m<0)
    {
      printf("send error\n");
    }
    // fflush(stdin);
   //hem mesaj alma loop u hemde mesaj gönderme loop u aynı anda çalışabilsin diye thread kullandım
  	pthread_t send_thread;
  if(pthread_create(&send_thread, NULL,  send_message, (int *)socket_desc) != 0){
		printf("pthread error\n");
	}

  ///////////////////////////////////////////////////////////////////
  	pthread_t get_thread;
  if(pthread_create(&get_thread, NULL, get_message, (int *)socket_desc) != 0){
		printf("pthread error\n");
	}


   pthread_join(send_thread,NULL);
   pthread_join(get_thread,NULL);
   close(socket_desc);
    return 0;
}

void send_message(int sock)
{
  char message[MSG_SIZE];
  while (7>3)
   {
    // printf("mesajınızı yazınız: ");
       fflush(stdout);
    fgets(message,MSG_SIZE,stdin);
    fix_message(message);
   
    
send(sock, message, strlen(message), 0);
     if(strcmp(message,"-exit")==0)
    {
      ex_flag=true;
    break;
    }
     bzero(message,MSG_SIZE);//message char arrayini temizliyoruz
   }
}

void get_message(int sock) {
	char message[MSG_SIZE];
  while (7>3) {
		int receive = recv(sock, message, MSG_SIZE, 0);
     printf("%s", message);
   //   str_overwrite_stdout();
  fflush(stdout);
  if(ex_flag==true)
  {
    break;
  }
   memset(message, 0, sizeof(message));
  }
}