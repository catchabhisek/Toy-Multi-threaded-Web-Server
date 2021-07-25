#define _POSIX_C_SOURCE 200112L
#include <pthread.h>
#include "../lib/myOwnthread.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <sys/file.h>
#include <fcntl.h>

static long fd;
static long winning_bid;

mythread_mutex_t mutex;


typedef struct req_node
{
  mythread_t* conn_thread;
  int client_sock;
  struct req_node *next;
}req_node;

typedef struct req_queue {
    req_node *front; 
    req_node *rear; 
} req_queue;

static req_queue* request_queue;

/* Queue Operations for the thread library */
void enqueue_request(req_queue* request_queue, req_node* request)
{
  if (request_queue->rear == NULL) {
    request_queue->front = request;
    request_queue->rear = request;
    return;
  }
  request_queue->rear->next = request;
  request_queue->rear = request;
}

void dequeue_request(req_queue* request_queue)
{
  if (request_queue->front == NULL) 
    return;
  req_node* node = request_queue->front;
  request_queue->front = request_queue->front->next;
  if (request_queue->front == NULL) {
    request_queue->rear = NULL;
  }
  node->next=NULL;
  close(node->client_sock);
  free(node);
}


int queue_empty(req_queue* request_queue)
{
    if((request_queue->rear == NULL) && (request_queue->front == NULL))
        return 1;
    return 0;
}

int queue_size(req_queue* request_queue)
{
    int size = 0;
    req_node* temp = request_queue->front;
    while(temp)
    {
        size++;
        temp=temp->next;
    }
    return size;
}

req_node* request_get(req_queue* request_queue, int client_fd)
{
  req_node* current = request_queue->front;
  while (current != NULL)
  {
    if (current->client_sock == client_fd)
      return current;
    current = current->next;
  }
  return NULL;
}

void delete_node(req_queue* request_queue, int client_fd) 
{ 
  if (request_queue->front == NULL) 
    return;
  req_node* temp = request_queue->front;

  if (temp->client_sock == client_fd) 
  {
    if (temp->next = NULL)
    {
      request_queue->front = NULL;
      request_queue->rear = NULL;
      free(temp);
      return;
    }
    request_queue->front = temp->next;
    free(temp);
  }
  else
  {
    while((temp->next !=NULL) && (temp->next->client_sock != client_fd))
    {
      temp=temp->next;
    }
    if(temp->next == NULL)
    {
      return;
    }
    req_node* next = temp->next->next;
    free(temp->next);
    temp->next = next;
  }
}


typedef struct {
  	char status_code[4];
   	char status_reason[50];
} http1_status;

http1_status http_status[] ={
	{"200\0", "OK"},
	{"201\0", "Created"},
	{"202\0", "Accepted"},
	{"204\0", "No Content"},
	{"400\0", "Bad Request"},
	{"401\0", "Unauthorized"},
	{"403\0", "Forbidden"},
	{"404\0", "Not Found"},
};

typedef struct http1_request {
    char* request_uri;
    char* method;
    char* http_version;
    char* form_data;
    char* origin;
} http1_request;

typedef struct http1_response {
    char* http_version;
    http1_status status_code;
    char* server;
    long content_length;
    char* content_type;
    char* body;
} http1_response;


int get_listener_socket(char *port)
{
  int server_fd, new_socket, valread; 
  struct sockaddr_in address; 
  int opt = 1; 
  int addrlen = sizeof(address); 
       
  // Creating socket file descriptor 
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
  { 
    perror("socket failed"); 
    exit(0); 
  } 

  // Forcefully attaching socket to the port 8080 
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))) 
  { 
    perror("setsockopt"); 
    exit(0); 
  }
 
  address.sin_family = AF_INET; 
  address.sin_addr.s_addr = INADDR_ANY; 
  address.sin_port = htons(atoi(port)); 
       
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) 
  { 
    perror("bind failed"); 
    exit(0); 
  } 
  if (listen(server_fd, 3) < 0) 
  { 
    perror("listen"); 
    exit(0); 
  }

  return server_fd;
}

http1_request* http1_request_parser(char* request_content)
{
  http1_request* request = malloc(sizeof(http1_request));
  char *request_line = NULL;
  char *request_header = NULL;
  char *formdata = NULL;
  char *method = NULL;
  char *request_uri = NULL;
  char *http_version = NULL;
  char *form_data = NULL;
  char *origin = NULL;
  char *header = NULL;
  char *opt = NULL;
  char *crlf = "\r\n";   // delimiters are CRLF

  formdata = strrchr(request_content, '\n');
  request_line = strtok(request_content, crlf);
  for(int i=0; i<9; i++)
  {
    char* header = strtok(NULL, crlf);
    if(header == NULL)
      break;
    if((strncmp(header, "Origin:", 7) == 0) || (strncmp(header, "Host:", 5) == 0))
      origin = strrchr(header, ' ') + 1;
  }
  if(origin == NULL)
    origin = "http://127.0.0.1:8888";
  request_header = strtok(NULL, crlf);
  method = strtok(request_line, " ");
  request_uri = strtok(NULL, " ");
  http_version = strtok(NULL, " ");

  request->method = malloc((strlen(method) * sizeof(char)) + 1);
  strcpy(request->method, method);
  request->request_uri = malloc((strlen(request_uri) * sizeof(char)) + 1);
  strcpy(request->request_uri, request_uri);
  request->http_version = malloc((strlen(http_version) * sizeof(char)) + 1);
  strcpy(request->http_version, http_version);
  request->origin = malloc((strlen(origin) * sizeof(char)) + 1);
  strcpy(request->origin, origin);

  if(strcmp(method, "POST") == 0)
  {
    request->form_data = malloc(strlen(formdata) * sizeof(char) + 1);
    strcpy(request->form_data, formdata);
  }
  else
    request->form_data = NULL;

  return request;
}


int send_response(http1_response* http_response, int client_fd, char* type)
{
  const long  request_buffer_size = 200000;
  char response[request_buffer_size];
  memset(response, '\0', sizeof(response));
	long response_length;
  sprintf(response, 
		"%s %s %s \r\n"
		"Content-Length: %ld \r\n"
		"Content-Type: %s \r\n"
    "Accept: */* \r\n"
    "Accept-Language: en,hi;q=0.9 \r\n"
    "Access-Control-Request-Headers: * \r \n"
    "Access-Control-Allow-Origin: %s \r\n"
    "Access-Control-Allow-Methods: GET, DELETE, HEAD \r\n"
    "Access-Control-Allow-Credentials: true \r\n"
		"\r\n"
		"%s", http_response->http_version, 
		http_response->status_code.status_code, 
		http_response->status_code.status_reason,
		http_response->content_length,
		http_response->content_type,
    http_response->server,
		(strcmp(type, "GET") == 0) ? http_response->body : "");
	response_length = strlen(response);
  
	int rv = send(client_fd, response, response_length, 0);

	if (rv < 0) {
		perror("send");
	}
  return rv;
}

void urldecode(char *str)
{
  for (int i = 0; str[i] != 0; i++) {
    char c = str[i];
    if (c == '+') {
      str[i] = ' ';
    }
  }
}

void serve_post_req(http1_request* request, int client_fd)
{
  char file_path[256];
  strcpy(file_path, "./data/chat.history");
  char* request_uri = request->request_uri;

  http1_response* http_response = malloc(sizeof(http1_response));
  const char* html_file = strrchr(request_uri, '/') + 1;
  http_response->server = request->origin;

  if(strcmp(html_file, "messages") == 0)
  {
    mythread_mutex_lock(&mutex);
    FILE *f = fopen(file_path, "a+");

    char *sender_data;
    char *message;

    urldecode(request->form_data);
    char* request_content = request->form_data;
    sender_data = strrchr(strtok(request_content, "&"), '=') + 1;
    message = strrchr(strtok(NULL, "&"), '=') + 1;
    strcat(sender_data, " : ");
    strcat(sender_data, message);
    fprintf(f, "%s\n", sender_data);
    fclose(f);
    mythread_mutex_unlock(&mutex);
    char response[1024];
    memset(response, '\0', sizeof(response));
    long response_length;
    sprintf(response, 
      "%s %s %s \r\n"
      "Content-Length: %d \r\n"
      "Content-Type: %s \r\n"
      "Access-Control-Request-Headers: *"
      "Accept: */* \r\n"
      "Accept-Language: en,hi;q=0.9 \r\n"
      "Access-Control-Request-Headers: * \r \n"
      "Access-Control-Allow-Origin: %s \r\n"
      "Access-Control-Allow-Methods: GET, DELETE, HEAD \r\n"
      "Access-Control-Allow-Credentials: true \r\n"
      "\r\n", request->http_version, 
        http_status[1].status_code, 
        http_status[1].status_reason,
        0,
        "text/plain",
        http_response->server);
    response_length = strlen(response);
  
    int rv = send(client_fd, response, response_length, 0);

    if (rv < 0) {
      perror("send");
    }
  }

  if(strcmp(html_file, "game") == 0)
  {
    mythread_mutex_lock(&mutex);

    char* request_content = request->form_data;
    char* string;
    long bid_value = strtol(strrchr(strtok(request_content, "&"), '=') + 1, &string, 10);

    if(bid_value > winning_bid)
    {
      winning_bid = bid_value;
    }

    mythread_mutex_unlock(&mutex);

    char response[1024];
    memset(response, '\0', sizeof(response));
    long response_length;
    sprintf(response, 
      "%s %s %s \r\n"
      "Content-Length: %d \r\n"
      "Content-Type: %s \r\n"
      "Access-Control-Request-Headers: *"
      "Accept: */* \r\n"
      "Accept-Language: en,hi;q=0.9 \r\n"
      "Access-Control-Request-Headers: * \r \n"
      "Access-Control-Allow-Origin: %s \r\n"
      "Access-Control-Allow-Methods: GET, DELETE, HEAD \r\n"
      "Access-Control-Allow-Credentials: true \r\n"
      "\r\n", request->http_version, 
        http_status[1].status_code, 
        http_status[1].status_reason,
        0,
        "text/plain",
        http_response->server);
    response_length = strlen(response);
  
    int rv = send(client_fd, response, response_length, 0);

    if (rv < 0) {
      perror("send");
    }
  }

}

void serve_get_req(http1_request* request, int client_fd)
{
  char file_path[256];
  char file_path_404[256];
  strcpy(file_path_404, "./data/404.html");
  strcpy(file_path, "./data/");

  char* request_uri = request->request_uri;

  http1_response* response = malloc(sizeof(http1_response));
  const char* html_file = strrchr(request_uri, '/') + 1;
  response->server = request->origin;

  if((html_file != NULL) && ((strchr(html_file, '.')) != NULL))
  {
    strcat(file_path, html_file);

    FILE *f = fopen(file_path, "r");

    if(f == NULL)
    {
      f = fopen(file_path_404, "r");
      response->status_code = http_status[7];
    }
    else
    {
      response->status_code = http_status[0];
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

		char *string = (char*)malloc((sizeof(char)*fsize) + 11);
    fread(string, fsize+1, 1, f);
    string[fsize] = '\0';
		fclose(f);

    char *extensions =  strrchr(request_uri, '.') + 1;

    if(strcmp(extensions, "html") == 0)
      response->content_type = "text/html";
    else if(strcmp(extensions, "js") == 0)
      response->content_type = "text/javascript";
    else if(strcmp(extensions, "css") == 0)
      response->content_type = "text/css";
    else if(strcmp(extensions, "png") == 0)
      response->content_type = "image/png";
    else
    {
      char client_id[10];
      sprintf(client_id, "%d", client_fd);
      strcat(string, client_id);
      response->content_type = "text/plain";
    }

    response->http_version = request->http_version;
    response->content_length = strlen(string);
    response->body = string;

    send_response(response, client_fd, request->method);
    free(string);

  }
  else if(html_file != NULL)
  {
    strcpy(file_path, "./data/chat.history");
    if(strcmp(html_file, "messages") == 0)
    {
      mythread_mutex_lock(&mutex);

      FILE *f = fopen(file_path, "r+");

      if(f != NULL)
      {
        response->status_code = http_status[0];
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

        char *string = (char*)malloc((sizeof(char)*fsize) + 1);
        fread(string, fsize+1, 1, f);
        string[fsize] = '\0';
        fclose(f);
        response->http_version = request->http_version;
        response->content_length = strlen(string);
        response->content_type = "text/plain";
        response->body = string;
        send_response(response, client_fd, request->method);
        free(string);
      }
      mythread_mutex_unlock(&mutex);
    }
    else if(strcmp(html_file, "game") == 0)
    {
      char winning_str[256];
      sprintf(winning_str, "%ld", winning_bid);
      response->http_version = request->http_version;
      response->content_length = strlen(winning_str);
      response->content_type = "text/plain";
      response->body = winning_str;
      send_response(response, client_fd, request->method);
    }
    else
    {
      FILE *f = fopen(file_path_404, "r");
      response->status_code = http_status[7];

      fseek(f, 0, SEEK_END);
      long fsize = ftell(f);
      fseek(f, 0, SEEK_SET);

      char *string = (char*)malloc((sizeof(char)*fsize) + 1);
      fread(string, fsize+1, 1, f);
      string[fsize] = '\0';
      fclose(f);

      response->http_version = request->http_version;
      response->content_length = strlen(string);

      char *extensions =  strrchr(request_uri, '.') + 1;

      response->content_type = "text/html";
      response->body = string;

      send_response(response, client_fd, request->method);
      free(string);
    }
  }

  free(response);

}


void *worker(void *client_sock)
{
  int *client_fd = (int *) client_sock;

  while(1)
  {
    const int request_buffer_size = 1024;
    char request_str[request_buffer_size];
    memset(request_str, '\0', sizeof(request_str));
    int rcv_ret = recv(*client_fd, &request_str, sizeof(request_str), 0);
    if(rcv_ret == 0)
    {
      close(*client_fd);
      delete_node(request_queue, *client_fd);
      break;
    }

    if(rcv_ret == -1)
    {
      close(*client_fd);
      delete_node(request_queue, *client_fd);
      break;
    }
    
    http1_request* request = http1_request_parser(request_str);

    if((strcmp(request->method, "GET") == 0) || (strcmp(request->method, "HEAD") == 0))
    {
      serve_get_req(request, *client_fd);
    }

    if(strcmp(request->method, "POST") == 0)
    {
      serve_post_req(request, *client_fd);
    }

    free(request);

    if(strcmp(request->http_version,"HTTP/1.0") == 0)
    {
      close(*client_fd);
      delete_node(request_queue, *client_fd);
      break;
    }
  }
}


int main(int argc, char *argv[])
{
  if (argc != 2) 
  {
    printf("%s Error: The parameters are not provided: <max_connection> \n", argv[0]);
    return -1;
  }
  
  int socket_desc, new_socket, c, *new_sock;
  int newfd;
  struct sockaddr_storage their_addr;
  char s[INET6_ADDRSTRLEN];	
  mythread_attr attr1;

  mythread_attr_init(&attr1);
  attr1.stacksize=300000;
  
  int listenfd = get_listener_socket("8888");

  if (listenfd < 0) {
    fprintf(stderr, "webserver: fatal error getting listening socket\n");
    exit(1);
  }

  request_queue = (struct  req_queue*)malloc(sizeof(struct req_queue));
  request_queue->front = NULL;
  request_queue->rear = NULL;
  mythread_mutex_init(&mutex);
  char *str;
  long max_connection = strtol(argv[1], &str, 10);

  while(newfd = accept(listenfd, (struct sockaddr *)&their_addr, (socklen_t *)&c)) 
  {
    if (newfd == -1)
    {
      perror("accept");
      continue;
    }

    if(queue_size(request_queue) == max_connection)
    {
      printf("Removing Request From Queue\n");
      dequeue_request(request_queue);
    }

    if(request_get(request_queue ,newfd) == NULL)
    {

      new_sock = malloc(sizeof(int));
      *new_sock = newfd;

      mythread_t* request_thread = malloc(sizeof(mythread_t));
      mythread_create(request_thread, &attr1, worker , (void *)new_sock);

      req_node* conn_request = malloc(sizeof(req_node));

      conn_request->conn_thread = request_thread;
      conn_request->client_sock = *new_sock;
      conn_request->next = NULL;
      enqueue_request(request_queue, conn_request);
    }
  }

  mythread_exit(0);

  return 0;
}
