#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/time.h>

struct timeval tstart, tend;
long double exectime1=0;
long double exectime2=0;

int main()
{
  int client1, client;
  int clients[30];
  struct sockaddr_in serverAddr;
  socklen_t addr_size;
  char  server_reply[2000];
  int opt=1;
  
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(8888);
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

  addr_size = sizeof serverAddr;

  char http_1_msg[100] = "HEAD /test_kb.txt HTTP/1.0\r\n";
  char http_1_1_msg[100] = "HEAD /test_kb.txt HTTP/1.1\r\n";

  /*for(int i=0; i<20; i++)
  {
    client = socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(client, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))) 
    { 
      perror("setsockopt"); 
    }
    connect(client, (struct sockaddr *) &serverAddr, addr_size);
    if(send(client, http_1_1_msg , strlen(http_1_1_msg) , 0) < 0)
    {
      puts("Send failed");
      return 1;
    }
    
    //Receive a reply from the server
    if(recv(client, server_reply , 2000 , 0) < 0)
    {
      puts("recv failed");
      break;
    }
    printf("Hi\n");
    puts(server_reply);
  }*/

  client1 = socket(AF_INET, SOCK_STREAM, 0);
  connect(client1, (struct sockaddr *) &serverAddr, addr_size);

  /*for(int i=0; i<40; i++)
  {
    gettimeofday(&tstart, NULL);
    if(send(client1, http_1_1_msg , strlen(http_1_1_msg) , 0) < 0)
    {
      puts("Send failed");
      break;
    }
    
    //Receive a reply from the server
    if(recv(client1, server_reply , 2000 , 0) < 0)
    {
      puts("recv failed");
      break;
    }
    gettimeofday(&tend, NULL );
    puts(server_reply);
    exectime1 += (((tend.tv_usec - tstart.tv_usec)/1000000.0) + (tend.tv_sec - tstart.tv_sec));
    //exectime1 +=((tend.tv_usec - tstart.tv_usec)/1000.0);
  }*/

  for(int i=0; i<40; i++)
  {
    client= socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(client, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))) 
    {
      perror("setsockopt");  
    }
    connect(client, (struct sockaddr *) &serverAddr, addr_size);
    gettimeofday(&tstart, NULL);
    if(send(client, http_1_msg , strlen(http_1_msg) , 0) < 0)
    {
      puts("Send failed");
      break;
    }

    //Receive a reply from the server
    if(recv(client, server_reply , 2000 , 0) < 0)
    {
      puts("recv failed");
      break;
    }
    gettimeofday(&tend, NULL );
    //exectime2 += (((tend.tv_usec - tstart.tv_usec)/1000000.0) + (tend.tv_sec - tstart.tv_sec));
    exectime2 += (((tend.tv_usec - tstart.tv_usec)/1000000.0) + (tend.tv_sec - tstart.tv_sec));
    //printf("%Lf\n", exectime2);
  }

  printf("exectime1 %Lf \n", exectime1);
  printf("exectime2 %Lf \n", exectime2);
}
