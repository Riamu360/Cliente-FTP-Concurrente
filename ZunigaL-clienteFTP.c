#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern int connectTCP(const char *host, const char *service);
extern int errexit(const char *format, ...);

void leer_server(int s, char *texto) {
    memset(texto, 0, 2000);
    int n = recv(s, texto, 1999, 0);
    if (n < 0) {
        printf("Error al recibir datos...\n");
        exit(1);
    }
    printf("%s", texto);
}

int sacar_puerto(char *respuesta, char *ip_final) {
    char *p1 = strchr(respuesta, '(');
    char *p2 = strchr(respuesta, ')');
    
    if (p1 == NULL) return -1;

    int a, b, c, d, port1, port2;
    sscanf(p1 + 1, "%d,%d,%d,%d,%d,%d", &a, &b, &c, &d, &port1, &port2);

    sprintf(ip_final, "%d.%d.%d.%d", a, b, c, d);
    
    int puerto_real = (port1 * 256) + port2;
    return puerto_real;
}

int conectar_datos(int sock_control, char *host) {
    char buf[2000];
    char comando[] = "PASV\r\n";
    
    send(sock_control, comando, strlen(comando), 0);
    
    memset(buf, 0, 2000);
    recv(sock_control, buf, 1999, 0);
    printf("%s", buf);

    if (strncmp(buf, "227", 3) != 0) {
        return -1;
    }

    char ip_temp[100];
    int puerto = sacar_puerto(buf, ip_temp);
    
    if (puerto < 0) return -1;

    char p_str[20];
    sprintf(p_str, "%d", puerto);
    
    return connectTCP(ip_temp, p_str);
}

void bajar(int sock, char *nombre) {
    int f = open(nombre, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (f < 0) {
        printf("No pude crear el archivo local :(\n");
        close(sock);
        exit(0); 
    }

    char buff[2000];
    int leido;
    while ((leido = recv(sock, buff, 2000, 0)) > 0) {
        write(f, buff, leido);
    }

    close(f);
    close(sock);
    exit(0);
}

void subir(int sock, char *nombre) {
    int f = open(nombre, O_RDONLY);
    if (f < 0) {
        printf("El archivo no existe o no se puede leer\n");
        close(sock);
        exit(0);
    }

    char buff[2000];
    int leido;
    while ((leido = read(f, buff, 2000)) > 0) {
        send(sock, buff, leido, 0);
    }

    close(f);
    close(sock);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: ./cliente <IP>\n");
        return 1;
    }

    signal(SIGCHLD, SIG_IGN);

    int s_control = connectTCP(argv[1], "21");
    char buffer[2000];
    char cmd_completo[2000];
    
    leer_server(s_control, buffer);

    printf("Usuario: ");
    fgets(buffer, 100, stdin);
    strtok(buffer, "\n");
    sprintf(cmd_completo, "USER %s\r\n", buffer);
    send(s_control, cmd_completo, strlen(cmd_completo), 0);
    leer_server(s_control, buffer);

    printf("Password: ");
    fgets(buffer, 100, stdin);
    strtok(buffer, "\n");
    sprintf(cmd_completo, "PASS %s\r\n", buffer);
    send(s_control, cmd_completo, strlen(cmd_completo), 0);
    leer_server(s_control, buffer);

    while (1) {
        printf("ftp> ");
        memset(buffer, 0, 2000);
        if (fgets(buffer, 1999, stdin) == NULL) break;
        
        char *accion = strtok(buffer, " \n");
        char *arg = strtok(NULL, "\n");

        if (accion == NULL) continue;

        if (strcmp(accion, "quit") == 0 || strcmp(accion, "exit") == 0) {
            send(s_control, "QUIT\r\n", 6, 0);
            leer_server(s_control, buffer);
            break;
        }
        else if (strcmp(accion, "ls") == 0) {
             int sock_datos = conectar_datos(s_control, argv[1]);
             if (sock_datos < 0) {
                 printf("Error al conectar datos\n");
                 continue;
             }

             send(s_control, "LIST\r\n", 6, 0);
             leer_server(s_control, buffer);

             int pid = fork();
             if (pid == 0) {
                 close(s_control);
                 int n;
                 while((n = recv(sock_datos, buffer, 1999, 0)) > 0) {
                     buffer[n] = '\0';
                     printf("%s", buffer);
                 }
                 close(sock_datos);
                 exit(0);
             } else {
                 close(sock_datos);
                 leer_server(s_control, buffer);
             }
        }
        else if (strcmp(accion, "get") == 0) {
            if (arg == NULL) {
                printf("Falta el nombre del archivo\n");
                continue;
            }
            int sock_datos = conectar_datos(s_control, argv[1]);
            if (sock_datos < 0) continue;

            sprintf(cmd_completo, "RETR %s\r\n", arg);
            send(s_control, cmd_completo, strlen(cmd_completo), 0);
            
            leer_server(s_control, buffer); 

            if (strncmp(buffer, "150", 3) == 0 || strncmp(buffer, "125", 3) == 0) {
                int pid = fork();
                if (pid == 0) {
                    close(s_control);
                    bajar(sock_datos, arg);
                } else {
                    close(sock_datos);
                    leer_server(s_control, buffer);
                }
            } else {
                close(sock_datos);
            }
        }
        else if (strcmp(accion, "put") == 0) {
            if (arg == NULL) {
                printf("Que archivo subo?\n");
                continue;
            }
            int sock_datos = conectar_datos(s_control, argv[1]);
            if (sock_datos < 0) continue;

            sprintf(cmd_completo, "STOR %s\r\n", arg);
            send(s_control, cmd_completo, strlen(cmd_completo), 0);

            leer_server(s_control, buffer);

            if (strncmp(buffer, "150", 3) == 0 || strncmp(buffer, "125", 3) == 0) {
                if (fork() == 0) {
                    close(s_control);
                    subir(sock_datos, arg);
                } else {
                    close(sock_datos);
                    leer_server(s_control, buffer);
                }
            } else {
                close(sock_datos);
            }
        }
        else if (strcmp(accion, "mkdir") == 0) {
            if (arg) {
                sprintf(cmd_completo, "MKD %s\r\n", arg);
                send(s_control, cmd_completo, strlen(cmd_completo), 0);
                leer_server(s_control, buffer);
            }
        }
        else if (strcmp(accion, "cd") == 0) {
            if (arg) {
                sprintf(cmd_completo, "CWD %s\r\n", arg);
                send(s_control, cmd_completo, strlen(cmd_completo), 0);
                leer_server(s_control, buffer);
            }
        }
        else if (strcmp(accion, "pwd") == 0) {
            send(s_control, "PWD\r\n", 5, 0);
            leer_server(s_control, buffer);
        }
        else if (strcmp(accion, "del") == 0) {
            if (arg) {
                sprintf(cmd_completo, "DELE %s\r\n", arg);
                send(s_control, cmd_completo, strlen(cmd_completo), 0);
                leer_server(s_control, buffer);
            }
        }
        else {
            printf("Comando no reconocido o no implementado :(\n");
        }
    }

    close(s_control);
    return 0;
}