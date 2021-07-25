#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/time.h>

struct timeval tstart, tend;
long double exectime2=0;

int main()
{
  int client1, client;

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

  for(int i=0; i<10; i++)
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
    puts(server_reply);
    exectime2 += (((tend.tv_usec - tstart.tv_usec)/1000000.0) + (tend.tv_sec - tstart.tv_sec));
  }
  printf("exectime2 %Lf \n", exectime2);
}
