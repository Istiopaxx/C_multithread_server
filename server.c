
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#define ROOT "/export/home/stud21/18101279" // unix 서버의 경로
#define BYTES 1024                          // 파일에서 읽을 바이트 수
#define PORT_NUM 8888                       // 서버를 열 포트 번호


// counter : 초기값은 0이고, 현재 처리하고 있는 요청의 수를 뜻합니다.
int counter = 0;

void *connection_handler(void *socket_desc)
{
    counter++; // 카운터를 올립니다.
    int rcvd;
    char msg[99999], data_to_send[1024], path[99999];
    char *reqline;

    int fd, bytes_read;               // 파일 디스크립터 선언
    int sock = *((int *)socket_desc); // 소켓 디스크립터 얻기
    memset((void *)msg, (int)'\0', 99999);

    // 소켓 디스크립터로 소켓에서 대기중인 요청을 읽어 msg 변수에 저장
    rcvd = recv(sock, msg, 99999, 0);

    // 현재 처리중인 요청의 갯수와 요청 본문을 터미널에 출력합니다.
    printf("Thread Counter : %d , Incoming Request. \n%s", counter, msg);

    if (rcvd < 0) // 요청 읽기 실패
        fprintf(stderr, ("recv() error\n"));
    else if (rcvd == 0) // 소켓 닫힘
        fprintf(stderr, "Client disconnected upexpectedly.\n");
    else
    {
        // 요청 본문 첫줄을 가져와 reqline에 저장
        reqline = strtok(msg, "\r\n");
        char *status_line;
        char *filename;

        // 요청 라우트에 따라 status line 변수와 응답할 filename 변수를 설정합니다.
        if (strncmp(reqline, "GET / HTTP/1.1\0", 14) == 0)
        {
            status_line = "HTTP/1.1 200 OK\r\n\r\n";
            filename = "/index.html";
        }
        else if (strncmp(reqline, "GET /sleep HTTP/1.1\0", 19) == 0)
        {
            sleep(5);
            status_line = "HTTP/1.1 200 OK\r\n\r\n";
            filename = "/sleep.html";
        }
        else
        {
            status_line = "HTTP/1.1 404 Not Found\r\n\r\n";
            filename = "/404.html";
        }

        strcpy(path, ROOT);                    // 서버 디렉토리를 path에 복사합니다.
        strcpy(&path[strlen(ROOT)], filename); // Filename을 path에 붙혀줍니다.

        // 소켓에 먼저 status line을 씁니다.
        send(sock, status_line, strlen(status_line), 0);
        // 정의된 path에 따라 응답으로 보낼 index.html 혹은 404.html을 파일 디스크립터에 저장합니다.
        fd = open(path, O_RDONLY);
        // 소켓에 파일 내용을 쓰고, 보냅니다.
        while ((bytes_read = read(fd, data_to_send, BYTES)) > 0)
            write(sock, data_to_send, bytes_read);
    }

    // 응답을 보내고 나면 요청을 처리했으므로 카운터를 낮춥니다.
    counter--;

    // 리소스 해제
    free(socket_desc);
    shutdown(sock, SHUT_RDWR);
    close(sock);
    sock = -1;
    return 0;
}

int main(int argc, char *argv[])
{
    // 소켓 디스크립터, 요청을 받을 소켓 디스크립터, 
    // 소켓 addr 사이즈, 쓰레드에 넘겨줄 소켓 디스크립터 포인터 선언
    int socket_desc, new_socket, sockaddr_size, *new_sock;
    // socker address 구조체 선언
    struct sockaddr_in server, client;

    // 소켓 디스크립터를 얻습니다.
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1)
    {
        puts("Could not create socket");
        return 1;
    }

    // socker address 서버 구조체의 설정변수를 저장합니다.
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT_NUM); // 포트 설정

    // 소켓 디스크립터와 포트를 설정한 서버 소켓addr 구조체를 바인딩해줍니다.
    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        puts("Binding failed");
        return 1;
    }

    // 서버 오픈
    puts("Waiting for incoming connections...");
    listen(socket_desc, 10);
    sockaddr_size = sizeof(struct sockaddr_in);

    // 서버로 들어오는 요청에 대해 소켓 디스크립터 생성
    while ((new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&sockaddr_size)))
    {

        puts("Connection accepted ");

        pthread_t thread_id;
        new_sock = malloc(1);
        *new_sock = new_socket;

        // 새 쓰레드를 생성하여 요청을 핸들링하는 함수와, 그 함수에 인자로 전달될 
        // 요청을 받은 소켓 디스크립터를 인자로 넘겨줍니다.
        if (pthread_create(&thread_id, NULL, connection_handler, (void *)new_sock) < 0)
        {
            puts("Could not create thread");
            return 1;
        }

        puts("Handler assigned");
    }

    return 0;
}
