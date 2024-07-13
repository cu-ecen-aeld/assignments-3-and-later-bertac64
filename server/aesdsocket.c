/*assignment 5 part 1 program
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <errno.h>
#include <sys/select.h>
#include <time.h>

#define PORT "9000"
//#define BACKLOG 10
#define FILE_PATH "/var/tmp/aesdsocketdata"
#define BUFFER_SIZE 1024
#define TIMEOUT_SEC 1

int server_socket = -1;
int client_socket = -1;
int file_fd = -1;
volatile sig_atomic_t sig_received = 0;
volatile sig_atomic_t proc_run = 0;

void handle_signal() {
    syslog(LOG_INFO, "Caught signal, exiting");
    sig_received = 1;
    if (!proc_run){
    	if (client_socket != -1) close(client_socket);
		if (server_socket != -1) close(server_socket);
		if (file_fd != -1) close(file_fd);
		remove(FILE_PATH);
		closelog();
		exit(0);
    }
}

void daemonize() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        perror("setsid");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

/*    umask(0);
    if (chdir("/") == -1){
    	perror("chdir");
    	exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);*/
}

void process_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t num_bytes;
    char *packet = NULL;
    size_t packet_len = 0;
    char *newline_pos;
    
	if (!sig_received) {
		proc_run = 1;
		while ((num_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
			buffer[num_bytes] = '\0';
		    packet = realloc(packet, packet_len + num_bytes + 1);
		    if (!packet) {
		        syslog(LOG_ERR, "Error reallocating memory: %s", strerror(errno));
		        return;
		    }
		    memcpy(packet + packet_len, buffer, num_bytes);
		    packet_len += num_bytes;
		    packet[packet_len] = '\0';
//		    printf("packet received: %ld bytes\n", packet_len);

			if (num_bytes < BUFFER_SIZE){ 
				newline_pos = strchr(packet, '\n');
				if (newline_pos != NULL) {
				    *(newline_pos + 1) = '\0';
				}else{
					packet = realloc(packet, packet_len + 1);
					packet_len++;
					packet[packet_len-1]='\n';
					packet[packet_len] = '\0';
					newline_pos = packet + packet_len;
				}
		        size_t complete_packet_len = newline_pos - packet + 1;

		        ssize_t bytes_written = write(file_fd, packet, complete_packet_len);
		        if (bytes_written == -1) {
		            syslog(LOG_ERR, "Error writing to file: %s", strerror(errno));
		            free(packet);
		            return;
		        }

		        // Read back the file and send its contents to the client
		        lseek(file_fd, 0, SEEK_SET);
		        off_t file_size = lseek(file_fd, 0, SEEK_END);
		        lseek(file_fd, 0, SEEK_SET);
		        off_t offset = 0;
		        while (offset < file_size) {
		            num_bytes = pread(file_fd, buffer, BUFFER_SIZE, offset);
		            if (num_bytes == -1) {
		                syslog(LOG_ERR, "Error reading from file: %s", strerror(errno));
		                free(packet);
		                return;
		            }

		            if (send(client_socket, buffer, num_bytes, 0) == -1) {
		                syslog(LOG_ERR, "Error sending data: %s", strerror(errno));
		                free(packet);
		                return;
		            }
		            offset += num_bytes;
		        }
		    }
		}
    	free(packet);
    }
}


int main(int argc, char *argv[]) {
    int status;
    struct addrinfo hints, *res;
    struct sockaddr_storage client_addr;
    socklen_t addr_size;
    char client_ip[INET6_ADDRSTRLEN];

    openlog("aesdsocket", LOG_PID, LOG_USER);

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  // Allow IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;  // Use my IP
    hints.ai_addr = INADDR_ANY;
    hints.ai_protocol = 0;
    
    if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
        syslog(LOG_ERR, "getaddrinfo: %s", gai_strerror(status));
        return -1;
    }
    
    server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_socket == -1) {
        syslog(LOG_ERR, "Error creating socket: %s", strerror(errno));
        freeaddrinfo(res);
        return -1;
    }

    if (bind(server_socket, res->ai_addr, res->ai_addrlen) == -1) {
        syslog(LOG_ERR, "Error binding socket: %s", strerror(errno));
        close(server_socket);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    
    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        daemonize();
    }

	if (listen(server_socket, SOMAXCONN) == -1) {
	    syslog(LOG_ERR, "Error listening on socket: %s", strerror(errno));
	    close(server_socket);
	    return -1;
	}

	file_fd = open(FILE_PATH, O_CREAT | O_APPEND | O_RDWR, 0666);
	if (file_fd == -1) {
	    syslog(LOG_ERR, "Error opening file: %s", strerror(errno));
	    close(server_socket);
	    return -1;
	}

	while (!sig_received){
		addr_size = sizeof(client_addr);
		client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
		if (client_socket == -1) {
		    syslog(LOG_ERR, "Error accepting connection: %s", strerror(errno));
		    continue;
		}

		if (client_addr.ss_family == AF_INET) {
		    struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
		    inet_ntop(AF_INET, &s->sin_addr, client_ip, sizeof(client_ip));
		}

		syslog(LOG_INFO, "Accepted connection from %s", client_ip);

		process_client(client_socket);
		proc_run = 0;

		close(client_socket);
		client_socket = -1;
		syslog(LOG_INFO, "Closed connection from %s", client_ip);
	}

    close(server_socket);
    close(file_fd);
    remove(FILE_PATH);
    closelog();    
    return 0;
}
