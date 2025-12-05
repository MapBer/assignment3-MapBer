#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <signal.h>

#define PORT 9000
#define BACKLOG 5
#define DATAFILE "/var/tmp/aesdsocketdata"

static int sockfd = -1;
static int clientfd = -1;
static volatile sig_atomic_t exit_requested = 0;

void signal_handler(int signo)
{
    if (signo == SIGINT || signo == SIGTERM) {
        syslog(LOG_INFO, "Caught signal, exiting");
        exit_requested = 1;
    }
}

int main(int argc, char *argv[])
{
    // ---- XỬ LÝ THAM SỐ DÒNG LỆNH AN TOÀN ----
    int run_as_daemon = 0;

    // Chỉ so sánh argv[1] nếu argc > 1
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        run_as_daemon = 1;
    }

    if (run_as_daemon) {
        if (daemon(0, 0) != 0) {
            perror("daemon");
            return -1;
        }
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char recv_buf[1024];
    ssize_t bytes;

    openlog("aesdsocket", LOG_PID, LOG_USER);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        syslog(LOG_ERR, "sigaction(SIGINT) failed: %s", strerror(errno));
        closelog();
        return -1;
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        syslog(LOG_ERR, "sigaction(SIGTERM) failed: %s", strerror(errno));
        closelog();
        return -1;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        syslog(LOG_ERR, "socket error: %s", strerror(errno));
        closelog();
        return -1;
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        syslog(LOG_ERR, "setsockopt error: %s", strerror(errno));
        close(sockfd);
        closelog();
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        syslog(LOG_ERR, "bind error: %s", strerror(errno));
        close(sockfd);
        closelog();
        return -1;
    }

    if (listen(sockfd, BACKLOG) < 0) {
        syslog(LOG_ERR, "listen error: %s", strerror(errno));
        close(sockfd);
        closelog();
        return -1;
    }

    syslog(LOG_INFO, "Server started on port %d", PORT);

    while (!exit_requested) {

        client_len = sizeof(client_addr);
        clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (clientfd < 0) {
            if (errno == EINTR && exit_requested) {
                break;  // bị ngắt bởi signal, thoát vòng lặp chính
            }
            syslog(LOG_ERR, "accept error: %s", strerror(errno));
            close(sockfd);
            closelog();
            return -1;
        }

        char client_ip[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        char   *packet_buf  = NULL;
        size_t  packet_size = 0;
        int     discarding_packet = 0;

        while (!exit_requested) {
            bytes = recv(clientfd, recv_buf, sizeof(recv_buf), 0);
            if (bytes < 0) {
                if (errno == EINTR && exit_requested) {
                    break;
                }
                syslog(LOG_ERR, "recv error: %s", strerror(errno));
                free(packet_buf);
                close(clientfd);
                close(sockfd);
                closelog();
                return -1;
            } else if (bytes == 0) {
                // client đóng kết nối
                break;
            }

            size_t start = 0;

            for (ssize_t i = 0; i < bytes; i++) {
                if (recv_buf[i] == '\n') {
                    // có 1 packet hoàn chỉnh [start..i]

                    if (!discarding_packet) {
                        size_t segment_len = (i - start + 1);
                        char *new_buf = realloc(packet_buf, packet_size + segment_len + 1);
                        if (!new_buf) {
                            syslog(LOG_ERR, "realloc failed, discarding current packet");
                            free(packet_buf);
                            packet_buf = NULL;
                            packet_size = 0;
                            discarding_packet = 1;
                        } else {
                            packet_buf = new_buf;
                            memcpy(packet_buf + packet_size, &recv_buf[start], segment_len);
                            packet_size += segment_len;
                            packet_buf[packet_size] = '\0';

                            // 1️⃣ Ghi packet vào file
                            FILE *fp = fopen(DATAFILE, "a+");
                            if (!fp) {
                                syslog(LOG_ERR, "Error opening %s: %s", DATAFILE, strerror(errno));
                                free(packet_buf);
                                close(clientfd);
                                close(sockfd);
                                closelog();
                                return -1;
                            }

                            size_t written = fwrite(packet_buf, 1, packet_size, fp);
                            if (written != packet_size) {
                                syslog(LOG_ERR, "Error writing to %s: %s", DATAFILE, strerror(errno));
                                fclose(fp);
                                free(packet_buf);
                                close(clientfd);
                                close(sockfd);
                                closelog();
                                return -1;
                            }

                            // 2️⃣ Gửi lại toàn bộ nội dung file cho client
                            fflush(fp);
                            if (fseek(fp, 0, SEEK_SET) != 0) {
                                syslog(LOG_ERR, "fseek error on %s: %s", DATAFILE, strerror(errno));
                                fclose(fp);
                                free(packet_buf);
                                close(clientfd);
                                close(sockfd);
                                closelog();
                                return -1;
                            }

                            char send_buf[1024];
                            size_t rbytes;
                            while ((rbytes = fread(send_buf, 1, sizeof(send_buf), fp)) > 0) {
                                size_t total_sent = 0;
                                while (total_sent < rbytes) {
                                    ssize_t s = send(clientfd,
                                                     send_buf + total_sent,
                                                     rbytes - total_sent,
                                                     0);
                                    if (s < 0) {
                                        if (errno == EINTR && exit_requested) {
                                            // bị signal, thoát sạch sẽ
                                            break;
                                        }
                                        syslog(LOG_ERR, "send error: %s", strerror(errno));
                                        fclose(fp);
                                        free(packet_buf);
                                        close(clientfd);
                                        close(sockfd);
                                        closelog();
                                        return -1;
                                    }
                                    total_sent += s;
                                }
                                if (exit_requested) break;
                            }

                            fclose(fp);

                            // reset packet buffer cho gói kế tiếp
                            free(packet_buf);
                            packet_buf  = NULL;
                            packet_size = 0;

                            if (exit_requested) {
                                break;
                            }
                        }
                    } else {
                        // đang discard packet dài quá, '\n' kết thúc packet đó
                        discarding_packet = 0;
                    }

                    start = i + 1;
                }
            }

            if (exit_requested) break;

            // phần còn lại sau '\n' cuối cùng (partial packet)
            if (start < (size_t)bytes && !discarding_packet) {
                size_t segment_len = bytes - start;
                char *new_buf = realloc(packet_buf, packet_size + segment_len + 1);
                if (!new_buf) {
                    syslog(LOG_ERR, "realloc failed while appending partial packet, discarding current packet");
                    free(packet_buf);
                    packet_buf = NULL;
                    packet_size = 0;
                    discarding_packet = 1;
                } else {
                    packet_buf = new_buf;
                    memcpy(packet_buf + packet_size, &recv_buf[start], segment_len);
                    packet_size += segment_len;
                    packet_buf[packet_size] = '\0';
                }
            }
        }

        free(packet_buf);
        packet_buf = NULL;

        syslog(LOG_INFO, "Closed connection from %s", client_ip);
        close(clientfd);
        clientfd = -1;
    }

    if (clientfd != -1) {
        close(clientfd);
    }
    if (sockfd != -1) {
        close(sockfd);
    }

    if (remove(DATAFILE) != 0) {
        syslog(LOG_ERR, "Error removing %s: %s", DATAFILE, strerror(errno));
    }

    closelog();
    return 0;
}
