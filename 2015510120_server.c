#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <stdbool.h>
//hocam komutların başına - işareti koyarak çalıştırdım ben ödev kağıdında öyleydi diye mesela -msg alsdhlahsdklahsdh gibi
//bir de benim -pcreate fonksiyonumda hata alıyorum hatanın sebebini bulamadım normal create yapmaktan tek farkı odanın private bool unu true
//yapmak ama -pcreate yapınca biriki comut sonra ya segmentation fault veriyor yada illegal adress diye hata veriyor onu anlamadım diğer komutlarım çalışıyor
#define BUFFER_SZ 2048 //mesaj boyutu
#define MAX_ROOM 10//oda sayısını 10 da sınırladık

void str_parser(char msg[],char code[],char yedekMesaj[]);//clienttan gelen mesajı parçalar(commend kısmı ile body kısmını )
void *all_process(void * c);//mesaja göre işlem yapan ana fonksiyonumuz mainde thread tarafından kullanılıyor


static int room_number=0;//serverdaki oda sayısını tutuyor
static int uid = 10;
pthread_t tid;
   
//clientlarımız ı struct olarak oluşturduk
typedef struct{
	struct sockaddr_in address;
	int sockfd;
	int uid;
  bool have_a_room;//odası var mı diye bakıyor varsa oda yaratmasına ve başka odaya girmesine engel olmak için
	char name[32];
  char client_room_name[30];//bazı noktalarda clientın hangi oda da olduğunu öğrenmemiz gerekiyor

} client_t;
//odalarımızı struct olarak tuttuk
typedef struct{
	bool is_private;//odanın durumunu belirler
  client_t *clients_of_room[10];//odanın içindeki clientsları tutar max 10 clients olabilir bi oda da
  char room_name[30];//oda ismini tutar
  int client_num_of_room;//oda da ki mevcut client saıyısını tutar 
  
} room_t;
room_t *all_of_rooms[MAX_ROOM];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void msg_room_clients(client_t * cli,char msg[BUFFER_SZ]){//bir odadaki clientlara mesaj göndermek için kullandığımız fonksiyon
   
              char all_msg[BUFFER_SZ];//isimle birleşmiş mesajı tutuyor
              bzero(all_msg,BUFFER_SZ);//all_msg strigimizi sıfırlıyoruz
               strcat(all_msg,"message->");
               strcat(all_msg,cli->name);//client ismini mesajın başına ekliyoruz
               strcat(all_msg,": ");
               strcat(all_msg,msg);
               strcat(all_msg,"\n");

	int i=0;
  for(i=0;i<room_number;i++)
  {
    if(strcmp(all_of_rooms[i]->room_name,cli->client_room_name)==0)//mesaj gönderilecek clientların odası seçiliyor
    {
      room_t *r=all_of_rooms[i];//yazı yükünü azaltmak için
      int j=0;
      for(j=0;j<r->client_num_of_room;j++)
      {
        if(r->clients_of_room[j]!=cli)//mesaj atan client hariç odadaki diğer clientlara mesaj gönderiliyor
        {
         	if(write(r->clients_of_room[j]->sockfd, all_msg, strlen(all_msg)) < 0){
					perror("write to descriptor failed");
					break;
				}
        }
      }
    }
  }
  
}
void send_current_cli(client_t * cli,char *s){//listeleme esnasında istek yapan client a mesaj göndermek için üretilmiş bir fonksiyondur
	pthread_mutex_lock(&clients_mutex);     
   	if(write(cli->sockfd, s, strlen(s)) < 0){
					perror("ERROR: write to descriptor failed");
     }

	pthread_mutex_unlock(&clients_mutex);
}

int main(int argc, char **argv){
    char buff[25];
	int port = 3205;
	int option = 1;
	int listenfd = 0, connfd = 0;
  struct sockaddr_in serv_addr;
  struct sockaddr_in cli_addr;
  pthread_t tid;

  /* Socket settings */
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port);


	if(setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0){
		perror("setsockopt failed");
    return -1;
	}

	/* Bind */
  if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("Socket binding failed");
    return -1;
  }

  /* Listen */
  if (listen(listenfd, 10) < 0) {
    perror("Socket listening failed");
    return -1;
	}

	printf("**********WELCOME THE SERVER***********\n");

	while(1){
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		/* Client settings */
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
    cli->have_a_room=false;
		cli->uid = uid++;
        
        
  

    char name[32];
	
	if(recv(cli->sockfd, name, 32, 0) <= 0 ){
		printf("Didn't enter the name.\n");
	
	} else{
	    strcpy(cli->name, name);
        printf("%s has joined\n", name);
	}
   	bzero(name, strlen(name));

     
       pthread_create(&tid, NULL, &all_process, (void*)cli);     
	}

	return 0;
}

void fix_message(char msg[BUFFER_SZ])//enter a bastımızda mesaj servera gindce \n bu işaretide yolluyor bunu engellemek için
{
  int i=0;
  while(msg[i]!='\n')
  {
    i++;
    if(i+1>=BUFFER_SZ)//eğer mesajda hiç '\n' bu işaretten yoksa sonsuz döngüde kalmamak için
    {
      break;
    }
  }
  msg[i]='\0';
}

void list_room(client_t *cli);//odaları ve gerekirse oda içindeki kişileri listeleyen fonksiyon
void enter_room(client_t *cli,char msg[30]);//odaya girmek için kullandığım fonksiyon
void quit_room(client_t *cli,char msg[30]);//oda dan çıkmak için
void private_create_room(client_t * cli,char msg[50]);//private oda oluşturmak 
void create_room(client_t * cli,char msg[50]);//oda yaratmak için 
void close_empty_room(room_t * r);//bu fonksiyonla odada kimse kalmadıysa bu odayı oda dizisinden atıyor

void *all_process(void * c)//burada alınan mesaj parçalanıyor ve içeriğine göre ne yapılması gerekiyorsa yapılıyor
{
  	client_t *cli = (client_t *)c;

    char buff_out[BUFFER_SZ];
	char name[32];
	int leave_flag = 0;

              char code[30]= "";//mesajı iki kısma böldüm ilk kısım code dediğim görevin olduğu kısım
             char  msg[BUFFER_SZ]="";//ikinci parametre oda ismi yada mesajın body si
     while(1){
		if (leave_flag) {
			break;
		}

		int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
		if (receive > 0){
			if(strlen(buff_out) > 0){
				
              fix_message(buff_out);//mesajda enter yzüzünden '\n' bu işaret varsa onu atmak için 

             str_parser(msg,code,buff_out);//burada alınan mesajın code ve body kısmı ayrılıyor
            //mesajın başındaki code göre if elselere giriyoruz
              if(strcmp(code,"-list")==0)
              {         
                if(cli->have_a_room==false)
                {
               list_room(cli);
                }   
                else 
                {
                  send_current_cli(cli,"You cant see room list because you are already in a room\n");
                }   
                                 
              }
              else if(strcmp(code,"-create")==0 && cli->have_a_room==false)
              {
                  bool cont=true;
                   int i=0;
              for(i=0;i<room_number;i++)//burada odaları dolaşıyoruz ve eğer aynı isimde oda varsa bu odanın oluşturulmasına izin vermiyoruz
                {
           if(strcmp(all_of_rooms[i]->room_name,msg)==0)
                {
               cont=false;
                break;
                 }
                }
                  if(cont==true)
                  {
                   create_room(cli,msg); 
                  }
                  else
                  {
                    send_current_cli(cli,"this room name already exist,please try again\n");
                  }
                  
              }
             else if(strcmp(code,"-pcreate")==0 && cli->have_a_room==false)
              {
                	 bool cont=true;
                   int i=0;
              for(i=0;i<room_number;i++)//burada odaları dolaşıyoruz ve eğer aynı isimde oda varsa bu odanın oluşturulmasına izin vermiyoruz
                {
           if(strcmp(all_of_rooms[i]->room_name,msg)==0)
                {
               cont=false;
                break;
                 }
                }
                  if(cont==true)
                  {
                   private_create_room(cli,msg); 
                  }
                   else
                  {
                    send_current_cli(cli,"this room name already exist,please try again\n");
                  }
              }
             else if(strcmp(code,"-enter")==0)
              {
                if(cli->have_a_room==false)
                {
                  enter_room(cli,msg);
                  msg_room_clients(cli,"I entered room\n");//odaya girdimizi diğer clientlara mesaj atıyoruz
                }
                
              }
             else if(strcmp(code,"-quit")==0)
             {
                msg_room_clients(cli,"I am lefting from room\n");//odadan ayrıldığımızı diğer clientlara mesaj atıyoruz
                quit_room(cli,msg);
             }
             else if(strcmp(code,"-msg")==0)
             {
               msg_room_clients(cli,msg);
             }
            else if (strcmp(code, "-exit") == 0){//eğer -exit komutu gelirse client serverdan çıkıyor
            msg_room_clients(cli,"I am lefting from server\n");//serverdan ayrıldığımızı diğer clientlara mesaj atıyoruz
            quit_room(cli,cli->client_room_name);//ismi görünmesin diye client structını room daki client dizisinden çıkartıyoruz
      printf("%s left from the server\n", cli->name);
		   }

			}
     
		else {
			printf("ERROR: -1\n");
			leave_flag = 1;
		}

		bzero(buff_out, BUFFER_SZ);//burada kullandığımız stringleri bzero komutu ile sıfırlıyoruz yoksa her mesajda mesajlar birleşerek gidiyor
    bzero(code, 30);
		bzero(msg, BUFFER_SZ);

	}
    }
    //  pthread_detach(pthread_self());

}
void private_create_room(client_t * cli,char msg[50])//private oda oluşturmak için kullanıyoruz
{
  room_t *room;//bir oda structı oluşturuyoruz
  strcpy(room->room_name,msg);//clienttan aldığımı name i oda nami yapıyoruz
  room->is_private=true;//bu rada private oda olduğundan private bool unu true yapıyoruz
  room->client_num_of_room=0;//client sayısını önce sıfır diyoruz
                    
  room->clients_of_room[room->client_num_of_room]=cli;//daha sonra clientı oluşturduğu odaya atıyoruz
                    
  room->client_num_of_room=room->client_num_of_room+1;//client sayısını bir arttırıyoruz
  all_of_rooms[room_number]=room;//oluşturduğumuz room u all od room a atıyoruz 
  room_number++;//serverdaki oda sayısını bir arttırıyoruz
  cli->have_a_room=true;//clientın have a room unu true yapıyoruz
  strcpy(cli->client_room_name,room->room_name);//ve clientın oda ismini odanın ismini veriyoruz
}
void create_room(client_t * cli,char msg[50])//normal oda creati için kullanıyoruz bu fonksiyonu
{
	room_t *room;//bir oda structı oluşturuyoruz
  strcpy(room->room_name,msg);//clienttan aldığımı name i oda nami yapıyoruz
  room->is_private=false;//bu rada normal oda olduğundan private bool unu false yapıyoruz
  room->client_num_of_room=0;//client sayısını önce sıfır diyoruz
                    
  room->clients_of_room[room->client_num_of_room]=cli;//daha sonra clientı oluşturduğu odaya atıyoruz
                    
  room->client_num_of_room=room->client_num_of_room+1;//client sayısını bir arttırıyoruz
  all_of_rooms[room_number]=room;//oluşturduğumuz room u all od room a atıyoruz 
  room_number++;//serverdaki oda sayısını bir arttırıyoruz
  cli->have_a_room=true;//clientın have a room unu true yapıyoruz
  strcpy(cli->client_room_name,room->room_name);//ve clientın oda ismini odanın ismini veriyoruz
}
void quit_room(client_t *cli,char msg[30])//odadan çıkma için kullandığımız fonksiyon
{
       int i=0;
       
     for(i=0;i<room_number;i++)//tüm odaları tarıyoruz
      {
        if(strcmp(all_of_rooms[i]->room_name,msg)==0 && cli->have_a_room==true)//oda yı bulunca işlemlere başlıyoruz
        {
          room_t *r=all_of_rooms[i];//yazı yükünü azaltmak için 
          int j=0;
          int k=0;//k j den bir eksik dönüyor böylece çıkmak isteyen clianta ulaşınca onu tekrardan diziye atmıyoruz
          for(int j=0;j<r->client_num_of_room;j++)
          {
              if(cli!=r->clients_of_room[j])//çıkan clienttan sonraki clientları kaydırıyoruz böylece arrayimizde boşluk kalmamış oluyor
              {
                r->clients_of_room[k]=r->clients_of_room[j];//clientlar tekradan diziye atılıyor çıkmak isteyen client hariç
                k++;
              }
              else
              {
                cli->have_a_room=false;//clientın odası olmadığını belirtmek için
                strcpy( cli->client_room_name,"");//client odadan çıkınca oda ismini boş atıyoruz                             
              }
          }
          r->client_num_of_room=r->client_num_of_room-1;//odadai client sayısını turtan sayacı azaltıyoruz azaltıryoruz 
          close_empty_room(r);//eğer odadaki son kişi de çıktıysa(bunu konturolü fonksiyonda yapılıyor )odayı kapatacak foksiyonu çağırıyoruz 
          break;
           
        }
      }

}
void enter_room(client_t *cli,char msg[30])//rooma giriş için
{
     int i=0;
     for(i=0;i<room_number;i++)//odaları tarırıyoruz
      {
        if(strcmp(all_of_rooms[i]->room_name,msg)==0)//istenen odayı bulunca client ı o odaya gönderiyoruz
        {
           cli->have_a_room=true;//clientın oda bool unu true yapıyoruz
           strcpy(cli->client_room_name,all_of_rooms[i]->room_name); //odanın ismini oda name yapıyoruz         
           all_of_rooms[i]->clients_of_room[all_of_rooms[i]->client_num_of_room]=cli;//clientı odaya atıyoruz
           all_of_rooms[i]->client_num_of_room=all_of_rooms[i]->client_num_of_room+1;//odadaki client sayısını arttırıyoruz
           break;
        }
      }
      

      
}
void list_room(client_t *cli)//oda isimleri ve client isimlerini istek yapan clianta göndermek için
{
  
  char buffer[2048]="";
   int i=0;
                 for(i=0;i<room_number;i++)//odaları teker teker geziyoruz
                 {
                 
                   strcat(buffer,"\nroom_name: ");
                   strcat(buffer,all_of_rooms[i]->room_name);
                   strcat(buffer,"\n");
             
                   if(all_of_rooms[i]->is_private==false)//burada oda privete değilse odadaki kişileri bastıracazğız
                   {
                     
                       strcat(buffer,"****Room Customers****\n");
                    
                     int j=0;
                     for(j=0;j<all_of_rooms[i]->client_num_of_room;j++)
                     {                    
                       strcat(buffer,all_of_rooms[i]->clients_of_room[j]->name);//ikinci forla ilk forda ulaşılan odanın içindeki kişilerin isimlerini bastırıyoruz
                       strcat(buffer,"\n");
                    }
                   
                     
                   }
                   
                 }
                 send_current_cli(cli,buffer);//oluşturulan buffer mesajını send_current_cli fonksiyonunu kullanarak ona mesaj olarak atıyoruz
  
}
void str_parser(char msg[],char code[],char yedekMesaj[])//burada clientlardan aldığımız mesajı code ve msg (mesaj bodysi yada room name filan)
{                                                       //iki kısma ayırıyoruz
    bool flag=false;
 
  {    
         int  i=0;
          while(yedekMesaj[i]!='\0')//while mesajın sonuna kadar harf harf dönüyor
          {
             if(flag==false)//boşluk görene kadar flag false da kalıyor ve boşluktan önceki kısım codun içine yazılıyor
             {
              if(yedekMesaj[i]==' ')//boşlukla karşılaşınca flag true ya döner ve boşluk hiçbir yere aktarılmaz
              {
                  flag=true;
              }
             else
              {
                code[i]=yedekMesaj[i];//boşluğa kadar mesajın ilk kısmı code aktarılır harf harf
              }
             }
             else//flag true ya dönünce yani boşlukla karşılaşıldıktan sonraki kısım msg ye aktarılıyor
             {
            msg[i-strlen(code)-1]=yedekMesaj[i];//msg de i eksi codun uzunluğu nu kattık tabi birde boşluğu atmak için bir daha çıkardık            
             }
              i++;
          }     
  }  
}
void close_empty_room(room_t * r)//bu fonksiyonla odada kimse kalmadıysa bu odayı oda dizisinden atıyoruz
{
   if(r->client_num_of_room==0)//oda daki client sayısını konturol ediyoruz eğer sıfırsa o zaman bu odayı all_of_rooms dan siliyoruz
   {
     int i=0;
     int j=0;//forumuz yeni array için bir az dönecek bu onun için
     for(i=0;i<room_number;i++)
     {
       if(all_of_rooms[i]!=r)//kapanacak odamıza geldiğimizde onu diziye yazdırmıyoruz
       {
         all_of_rooms[j]=all_of_rooms[i];
         j++;
         break;
       }
     }
     room_number--;
     all_of_rooms[room_number]=NULL;//elamanlar bir kaydı bu yüzden sonuncuyu sıfırlıyoruz
   }
}
