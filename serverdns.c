#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define MAX_BUFFER_SIZE 1024
#define MAX_DOMAIN_LENGTH 255
#define MAX_RECORDS 1000

// Estructura para almacenar los registros de recursos de DNS
typedef struct {
    char domain[MAX_DOMAIN_LENGTH];
    char type[MAX_DOMAIN_LENGTH];
    char value[MAX_BUFFER_SIZE];
} dns_record;

// Función para leer los registros de recursos de un archivo de zona
void load_zone_file(const char *filename, dns_record *records, int *num_records) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: No se pudo abrir el archivo de zona %s\n", filename);
        return;
    }

    char line[MAX_BUFFER_SIZE];
    int record_count = 0;

    while (fgets(line, sizeof(line), file)) {
        char domain[MAX_DOMAIN_LENGTH];
        char type[MAX_DOMAIN_LENGTH];
        char value[MAX_BUFFER_SIZE];

        if (sscanf(line, "%s %s %s", domain, type, value) == 3) {
            strcpy(records[record_count].domain, domain);
            strcpy(records[record_count].type, type);
            strcpy(records[record_count].value, value);
            record_count++;
        }
    }

    fclose(file);
    *num_records = record_count;
}

// Función para buscar un registro de recursos de DNS
dns_record *lookup_record(const char *domain, dns_record *records, int num_records) {
    for (int i = 0; i < num_records; i++) {
        if (strcmp(domain, records[i].domain) == 0) {
            return &records[i];
        }
    }
    return NULL;
}

// Estructura para pasar argumentos al hilo
typedef struct {
    dns_record *records;
    int num_records;
    char buffer[MAX_BUFFER_SIZE];
    size_t buffer_size;
    struct sockaddr_in client_address;
    socklen_t client_address_len;
    int server_socket;
} thread_args_t;

// Función para manejar una solicitud DNS
void *handle_dns_request(void *arg) {
    thread_args_t *args = (thread_args_t *)arg;
    char domain[MAX_DOMAIN_LENGTH];
    sscanf(args->buffer, "%*s %s", domain);

    printf("Buscando registro para el dominio: %s\n", domain);

    dns_record *record = lookup_record(domain, args->records, args->num_records);

    if (record != NULL) {
        printf("Registro encontrado: %s %s %s\n", record->domain, record->type, record->value);
        char response[MAX_BUFFER_SIZE];
        snprintf(response, sizeof(response), "%s %s %s", record->domain, record->type, record->value);
        sendto(args->server_socket, response, strlen(response), 0, (struct sockaddr *)&args->client_address, args->client_address_len);
    } else {
        printf("Dominio no encontrado\n");
        const char *error_message = "Domain not found";
        sendto(args->server_socket, error_message, strlen(error_message), 0, (struct sockaddr *)&args->client_address, args->client_address_len);
    }

    free(args);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <ip> <port> </path/zones/midominio.com> </path/log.log>\n", argv[0]);
        return 1;
    }

    char *ip_address = argv[1];
    int port = atoi(argv[2]);
    const char *zone_file = argv[3];
    const char *log_file = argv[4];

    printf("Argumentos de línea de comandos procesados correctamente\n");
    printf("Dirección IP: %s\n", ip_address);
    printf("Puerto: %d\n", port);
    printf("Archivo de zona: %s\n", zone_file);
    printf("Archivo de registro: %s\n", log_file);

    dns_record records[MAX_RECORDS];
    int num_records = 0;
    load_zone_file(zone_file, records, &num_records);

    printf("Registros de recursos de DNS cargados desde el archivo de zona\n");
    printf("Número de registros: %d\n", num_records);

    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket == -1) {
        perror("Error al crear el socket");
        return 1;
    }

    printf("Socket UDP creado correctamente\n");

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(ip_address);
    server_address.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Error al vincular el socket");
        close(server_socket);
        return 1;
    }

    printf("Socket vinculado correctamente a %s:%d\n", ip_address, port);

    printf("Servidor DNS iniciado en %s:%d\n", ip_address, port);

    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);

        char buffer[MAX_BUFFER_SIZE];

        printf("Esperando solicitud DNS...\n");
        ssize_t bytes_received = recvfrom(server_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_address, &client_address_len);

        if (bytes_received > 0) {
            printf("Solicitud DNS recibida de %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

            thread_args_t *args = malloc(sizeof(thread_args_t));
            if (args == NULL) {
                perror("Error al asignar memoria para argumentos del hilo");
                continue;
            }
            args->records = records;
            args->num_records = num_records;
            memcpy(args->buffer, buffer, bytes_received);
            args->buffer_size = bytes_received;
            memcpy(&args->client_address, &client_address, client_address_len);
            args->client_address_len = client_address_len;
            args->server_socket = server_socket;

            pthread_t thread;
            if (pthread_create(&thread, NULL, handle_dns_request, args) != 0) {
                perror("Error al crear el hilo");
                free(args);
            } else {
                pthread_detach(thread);
            }
        } else if (bytes_received == -1) {
            perror("Error al recibir la solicitud");
        }
    }

    close(server_socket);
    return 0;
}
