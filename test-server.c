#include<stdio.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>

const int MAX_IP_STR_LEN = 16;// length of ip and null char of end
const int MAX_PORT_STR_LEN = 6;//length of port and null char of end
const int MAX_MS_LEN = 128;
const int MAX_PENDING = 5;

void remove_new_line(char *str){
	for(int i = 0 ; i < strlen(str) ; i++){
		if(str[i] == '\n'){
			str[i] = '\0';
			return;
		}
	}
}

void client_serve(int client_socket){
    char buffer[MAX_MS_LEN];
    ssize_t bytes_rcv = recv(client_socket, buffer, MAX_MS_LEN, 0);
    if(bytes_rcv < 0){
        perror("recv failed");
        exit(EXIT_FAILURE);
    }
    while(bytes_rcv > 0){
        ssize_t byets_send = send(client_socket, buffer, bytes_rcv, 0);
        if(bytes_rcv < 0){
            perror("send failed");
            exit(EXIT_FAILURE);
        }
        else if(bytes_rcv != byets_send){
            fputs("inexpected number of byets were send", stderr);
            exit(EXIT_FAILURE);
        }
        bytes_rcv = recv(client_socket, buffer, MAX_MS_LEN, 0);
        if(bytes_rcv < 0){
            perror("recv failed");
            exit(EXIT_FAILURE);
        }
    }
}

int main(){
    char *port_str = malloc(sizeof(char) * MAX_PORT_STR_LEN);

	if(port_str == NULL){
		fputs("memory allocation failed.(port)", stderr);
		exit(EXIT_FAILURE);
	}

	memset(port_str, '\0', MAX_PORT_STR_LEN);
    puts("server_port: ");
	fgets(port_str, MAX_PORT_STR_LEN + 1, stdin);

	//puts(port_str);
	remove_new_line(port_str);

    in_port_t server_port = atoi(port_str);

	int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(server_socket < 0){
		perror("socket() failed.");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    server_address.sin_addr.s_addr = htons(INADDR_ANY);

    if(bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) < 0){
        perror("bind failed.");
        exit(EXIT_FAILURE);
    }
    if(listen(server_socket, MAX_PENDING) < 0){
        perror("listen failed.");
        exit(EXIT_FAILURE);
    }
    for(;;){
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);

        int client_socket = accept(server_socket, (struct  sockaddr *) &client_address, &client_address_len);
        if(client_socket < 0){
            perror("accept failed.");
            exit(EXIT_FAILURE);
        }

        char client_name[INET_ADDRSTRLEN];
        if(inet_ntop(AF_INET, &client_address.sin_addr.s_addr, client_name, sizeof(client_address))){
            printf("saving [%s:%d]\n", client_name, ntohs(client_address.sin_port));
        }
        else{
            puts("unable to get client name");
        }
        client_serve(client_socket);
    }

    return 0;
}