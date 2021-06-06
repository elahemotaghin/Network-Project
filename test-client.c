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

void remove_new_line(char *str){
	for(int i = 0 ; i < strlen(str) ; i++){
		if(str[i] == '\n'){
			str[i] = '\0';
			return;
		}
	}
}

int main(){
	char *server_addr_str = malloc(sizeof(char) * MAX_IP_STR_LEN);
	char *port_str = malloc(sizeof(char) * MAX_PORT_STR_LEN);

	if(server_addr_str == NULL){
		fputs("memory allocation failed.(ip)", stderr);
		exit(EXIT_FAILURE);
	}
	if(port_str == NULL){
		fputs("memory allocation failed.(port)", stderr);
		exit(EXIT_FAILURE);
	}

	memset(server_addr_str, '\0', MAX_IP_STR_LEN);
	memset(port_str, '\0', MAX_PORT_STR_LEN);

	//get inputs
	puts("server_ip: [x.x.x.x]");
	fgets(server_addr_str, MAX_IP_STR_LEN + 1, stdin);
	puts("server_port: ");
	fgets(port_str, MAX_PORT_STR_LEN + 1, stdin);

	//puts(server_addr_str);
	//puts(port_str);
	remove_new_line(port_str);
	remove_new_line(server_addr_str);

	in_port_t server_port = atoi(port_str);

	int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(server_socket < 0){
		perror("socket() failed.");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));

	server_address.sin_family = AF_INET;
	int numerical_address = inet_pton(AF_INET, server_addr_str, &server_address.sin_addr);

	if(numerical_address == 0){
		fputs("invalid IP", stderr);
		exit(EXIT_FAILURE);
	}
	if(numerical_address < 0){
		fputs("printable to nummeric failed.", stderr);
		exit(EXIT_FAILURE);
	}

	server_address.sin_port = htons(server_port);

	if(connect(server_socket, (const struct sockaddr*) &server_address, sizeof(server_address)) < 0){
		perror("connection failed.");
		exit(EXIT_FAILURE);
	}

	for(;;){
		char *outgoing_msg = malloc(sizeof(char) * MAX_MS_LEN);
		if(server_addr_str == NULL){
			fputs("memory allocation failed.(msg)", stderr);
			exit(EXIT_FAILURE);
		}
		memset(outgoing_msg, '\0', MAX_MS_LEN);
		puts("Enter your desired msg: (max_len = 128, rest will be discarded.");
		scanf("%128s", outgoing_msg);

		size_t msg_len = strlen(outgoing_msg);
		ssize_t bytes = send(server_socket, outgoing_msg, msg_len, 0);
		if(bytes < 0){
			perror("send failed.");
			exit(EXIT_FAILURE);
		}
		else if(bytes != msg_len){
			fputs("unexpected number of bytes were send.", stderr);
			exit(EXIT_FAILURE);
		}
		unsigned int tptal_rcv_bytes = 0;

		puts("recived from server:");
		while(tptal_rcv_bytes < msg_len){
			char buffer[128] = {'\0'};
			bytes = recv(server_socket, buffer, MAX_MS_LEN-1, 0);

			if(bytes < 0){
				perror("recv failed.");
				exit(EXIT_FAILURE);
			} else if(bytes == 0){
				perror("server was terminated.");
				exit(EXIT_FAILURE);
			}

			tptal_rcv_bytes += bytes;
			buffer[bytes] = '\0';

			printf("%s", buffer);
			puts("");
		}

	}

	close(server_socket);

	return 0;
}
