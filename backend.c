#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "common.h"

#define TAMANO_TABLA 1000000 // Tamaño de la tabla hash en memoria

// Nodo para la tabla hash en memoria (manejo de colisiones)
typedef struct HashNode {
    IndexEntry entry;
    struct HashNode *siguiente;
} HashNode;

HashNode **tabla_hash = NULL;
FILE *f_data = NULL;

// Función Hash
unsigned int hash(long id, int ano) {
    unsigned long hash_val = (unsigned long)id * 31 + (unsigned long)ano;
    return hash_val % TAMANO_TABLA;
}

// Insertar en la tabla hash en memoria
void insertar_indice(IndexEntry entry) {
    unsigned int indice = hash(entry.id, entry.ano);
    HashNode *nuevo = malloc(sizeof(HashNode));
    if (!nuevo) {
        perror("Fallo al alocar HashNode");
        exit(EXIT_FAILURE);
    }
    nuevo->entry = entry;
    nuevo->siguiente = tabla_hash[indice];
    tabla_hash[indice] = nuevo;
}

// Buscar en la tabla hash en memoria
long long buscar_offset(long id, int ano) {
    unsigned int indice = hash(id, ano);
    HashNode *actual = tabla_hash[indice];
    while (actual) {
        if (actual->entry.id == id && actual->entry.ano == ano) {
            return actual->entry.offset;
        }
        actual = actual->siguiente;
    }
    return -1; // No encontrado
}

// Cargar el archivo de índice en la tabla hash
void cargar_indice() {
    tabla_hash = calloc(TAMANO_TABLA, sizeof(HashNode *));
    if (!tabla_hash) {
        perror("Fallo al crear tabla hash");
        exit(EXIT_FAILURE);
    }
    
    FILE *f_index = fopen(INDEX_FILE, "rb");
    if (!f_index) {
        fprintf(stderr, "Error: No se pudo abrir %s. Ejecuta el indexador primero.\n", INDEX_FILE);
        exit(EXIT_FAILURE);
    }

    IndexEntry entry;
    long count = 0;
    while (fread(&entry, sizeof(IndexEntry), 1, f_index) == 1) {
        insertar_indice(entry);
        count++;
    }
    fclose(f_index);
    printf("[BACKEND] Índice cargado. %ld entradas en la tabla hash.\n", count);
}

// Liberar memoria de la tabla hash
void liberar_tabla() {
    for (int i = 0; i < TAMANO_TABLA; i++) {
        HashNode *actual = tabla_hash[i];
        while (actual) {
            HashNode *temp = actual;
            actual = actual->siguiente;
            free(temp);
        }
    }
    free(tabla_hash);
}

// Procesar una solicitud de búsqueda
void procesar_solicitud(char *buffer, int fd_res) {
    long id;
    int ano;
    char linea_datos[MAX_LINEA];

    // GET id año
    if (sscanf(buffer, "GET %ld %d", &id, &ano) == 2) {
        long long offset = buscar_offset(id, ano);
        if (offset != -1) {
            fseeko(f_data, offset, SEEK_SET); // Usar fseeko para offsets grandes
            size_t len;
            fread(&len, sizeof(size_t), 1, f_data);
            fread(linea_datos, sizeof(char), len, f_data);
            write(fd_res, linea_datos, len);
        } else {
            write(fd_res, "NOT_FOUND", strlen("NOT_FOUND") + 1);
        }
    }
    // GET_ALL id
    else if (sscanf(buffer, "GET_ALL %ld", &id) == 1) {
        bool encontrado = false;
        for (int a = PRIMER_ANO; a <= ULTIMO_ANO; a++) {
            long long offset = buscar_offset(id, a);
            if (offset != -1) {
                encontrado = true;
                fseeko(f_data, offset, SEEK_SET);
                size_t len;
                fread(&len, sizeof(size_t), 1, f_data);
                fread(linea_datos, sizeof(char), len, f_data);
                
                // // --- INICIO DE LA LÍNEA AGREGADA ---
                // printf("[BACKEND] Enviando GET_ALL: ID %ld, Año %d, Datos: %s\n", id, a, linea_datos);
                // // --- FIN DE LA LÍNEA AGREGADA ---
                char respuesta[MAX_LINEA + 20];
                snprintf(respuesta, sizeof(respuesta), "%d:%s\n", a, linea_datos);
                write(fd_res, respuesta, strlen(respuesta));
            }
        }
        if (!encontrado) {
            write(fd_res, "NOT_FOUND", strlen("NOT_FOUND") + 1);
        }
    }
}

int main() {
    printf("[BACKEND] Iniciando servicio de búsqueda...\n");
    
    cargar_indice();

    f_data = fopen(DATA_FILE, "rb");
    if (!f_data) {
        fprintf(stderr, "Error: No se pudo abrir %s. Ejecuta el indexador primero.\n", DATA_FILE);
        liberar_tabla();
        exit(EXIT_FAILURE);
    }
    
    // Crear tuberías nombradas (FIFO)
    unlink(REQUEST_PIPE);
    unlink(RESPONSE_PIPE);
    if (mkfifo(REQUEST_PIPE, 0666) == -1 || mkfifo(RESPONSE_PIPE, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    printf("[BACKEND] Esperando conexiones en %s\n", REQUEST_PIPE);

    while (true) {
        int fd_req = open(REQUEST_PIPE, O_RDONLY); // Bloquea hasta que un escritor se conecte
    if (fd_req == -1) {
        perror("open request pipe");
        // Si hay un error al abrir el pipe, podrías querer manejarlo mejor
        // Por ahora, 'continue' intenta de nuevo.
        continue;
    }

    char buffer[MAX_LINEA];
    ssize_t bytes_leidos = read(fd_req, buffer, sizeof(buffer) - 1);
    close(fd_req); // Importante cerrar el descriptor después de usarlo

    if (bytes_leidos == -1) { // Error en la lectura
        perror("read from request pipe");
        continue; // Intenta de nuevo en la siguiente iteración
    } else if (bytes_leidos == 0) { // <--- ESTO ES CLAVE
        // El otro extremo del pipe (el escritor, es decir, el frontend) se ha cerrado.
        // Esto indica que el cliente actual ha terminado su comunicación.
        printf("[BACKEND] Cliente anterior se desconectó. Esperando una nueva conexión...\n");
        // No hay necesidad de 'break' aquí si quieres que el backend siga
        // escuchando indefinidamente por nuevos clientes.
        continue; // Vuelve al inicio del bucle para esperar una nueva conexión
    }
    
    // Si llegamos aquí, bytes_leidos > 0, lo que significa que se leyó una solicitud.
    buffer[bytes_leidos] = '\0'; // Null-terminate the string
    
    if (strcmp(buffer, "QUIT") == 0) {
        printf("[BACKEND] Solicitud de cierre 'QUIT' recibida. Terminando.\n");
        break; // El backend se cierra de forma controlada.
    }

    int fd_res = open(RESPONSE_PIPE, O_WRONLY); // Bloquea hasta que un lector se conecte
    if (fd_res == -1) {
        perror("open response pipe");
        // Si el cliente se desconectó antes de que pudieras abrir el pipe de respuesta,
        // o hay otro error, puedes simplemente continuar.
        continue;
    }

    procesar_solicitud(buffer, fd_res);
    close(fd_res);
     // Cierra el descriptor de respuesta
        // int fd_req = open(REQUEST_PIPE, O_RDONLY);
        // if (fd_req == -1) {
        //     perror("open request pipe"); continue;
        // }

        // char buffer[MAX_LINEA];
        // ssize_t bytes_leidos = read(fd_req, buffer, sizeof(buffer) - 1);
        // close(fd_req);
        
        // if (bytes_leidos > 0) {
        //     buffer[bytes_leidos] = '\0';
            
        //     if (strcmp(buffer, "QUIT") == 0) {
        //         printf("[BACKEND] Solicitud de cierre recibida. Terminando.\n");
        //         break;
        //     }

        //     int fd_res = open(RESPONSE_PIPE, O_WRONLY);
        //     if (fd_res == -1) {
        //         perror("open response pipe"); continue;
        //     }

        //     procesar_solicitud(buffer, fd_res);
        //     close(fd_res);
        // }
    }

    // Limpieza
    fclose(f_data);
    liberar_tabla();
    unlink(REQUEST_PIPE);
    unlink(RESPONSE_PIPE);
    printf("[BACKEND] Servicio finalizado.\n");

    return EXIT_SUCCESS;
}
