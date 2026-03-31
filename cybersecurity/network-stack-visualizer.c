#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define BUFFER_SIZE 256
#define BACKLOG     5

void reaper(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

typedef struct {
    unsigned long long rsp;
    unsigned long long rbp;
    unsigned long long rip;
} Registers;

static Registers get_registers(void) {
    Registers r;
    asm volatile ("mov %%rsp, %0" : "=r"(r.rsp));
    asm volatile ("mov %%rbp, %0" : "=r"(r.rbp));
    asm volatile ("lea (%%rip), %0" : "=r"(r.rip));
    return r;
}

static void print_stack_info(char *buf, Registers *reg) {
    typedef unsigned long long u64;
    int slots = 10;

    dprintf(1, "\n=== Register Snapshot ===\n");
    dprintf(1, "  RSP : 0x%llx\n", reg->rsp);
    dprintf(1, "  RBP : 0x%llx\n", reg->rbp);
    dprintf(1, "  RIP : 0x%llx\n", reg->rip);

    dprintf(1, "\n=== Stack Layout Around Buffer ===\n");
    dprintf(1, "%-10s %-20s %-20s\n", "Offset", "Address", "Value");
    dprintf(1, "%-10s %-20s %-20s\n", "------", "-------", "-----");

    for (int i = 0; i < slots; i++) {
        u64 *slot  = (u64 *)(buf + i * 8);
        const char *label = "";

        if (i == 0)                    label = "<-- buffer start";
        if (i == BUFFER_SIZE/8 - 1)    label = "<-- buffer end";
        if (i == BUFFER_SIZE/8)        label = "<-- saved RBP";
        if (i == BUFFER_SIZE/8 + 1)    label = "<-- return addr";

        dprintf(1, "+%-9d 0x%-18llx 0x%-18llx %s\n",
                i * 8, (u64)slot, *slot, label);
    }

    dprintf(1, "\n=== Frame Info ===\n");
    dprintf(1, "  buffer @ 0x%llx\n", (u64)buf);
    dprintf(1, "  RSP offset from buffer: %lld bytes\n",
            (long long)(reg->rsp - (u64)buf));
    dprintf(1, "  RBP offset from buffer: %lld bytes\n",
            (long long)(reg->rbp - (u64)buf));
    dprintf(1, "=================================\n\n");
}

static int handle_client(const char *client_ip) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    Registers reg = get_registers();

    dprintf(1, "\n[+] Connection from %s\n", client_ip);
    dprintf(1, "[+] Child PID: %d\n", getpid());
    dprintf(1, "\n--- Stack Frame Visualizer ---\n");
    dprintf(1, "Buffer is at: 0x%llx  (size: %d bytes)\n\n",
            (unsigned long long)buffer, BUFFER_SIZE);

    dprintf(1, "Enter text (max %d chars): ", BUFFER_SIZE - 1);

    int i = 0;
    int ret;
    while (i < BUFFER_SIZE - 1) {
        ret = read(0, &buffer[i], 1);
        if (ret <= 0 || buffer[i] == '\n') break;
        i++;
    }
    buffer[i] = '\0';

    dprintf(1, "You entered: \"%s\"\n", buffer);

    print_stack_info(buffer, &reg);

    dprintf(1, "Connection closed.\n");
    return 0;
}

static int start_server(int port) {
    int sockfd, newsockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int opt = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(port);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(sockfd, BACKLOG) < 0) {
        perror("listen");
        return 1;
    }

    signal(SIGCHLD, reaper);

    printf("[*] Stack Frame Visualizer Server\n");
    printf("[*] Listening on port %d ...\n\n", port);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (newsockfd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        char *client_ip = inet_ntoa(client_addr.sin_addr);
        printf("[*] Accepted connection from %s\n", client_ip);

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(newsockfd);
            continue;
        }

        if (pid == 0) {
            close(sockfd);
            dup2(newsockfd, STDIN_FILENO);
            dup2(newsockfd, STDOUT_FILENO);
            close(newsockfd);
            exit(handle_client(client_ip));
        }

        close(newsockfd);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3 || strcmp(argv[1], "-p") != 0) {
        fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[2]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %s\n", argv[2]);
        return 1;
    }

    return start_server(port);
}