#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "common.h"
#include <stdbool.h>


void limpiar_buffer_stdin() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Envía una solicitud y espera una respuesta
void enviar_y_recibir(const char *solicitud) {
    // Enviar solicitud
    int fd_req = open(REQUEST_PIPE, O_WRONLY);
    if (fd_req == -1) {
        perror("Error al abrir tubería de solicitud. ¿El backend está corriendo?");
        return;
    }
    write(fd_req, solicitud, strlen(solicitud) + 1);
    close(fd_req);

    // Esperar y recibir respuesta
    int fd_res = open(RESPONSE_PIPE, O_RDONLY);
    if (fd_res == -1) {
        perror("Error al abrir tubería de respuesta");
        return;
    }

    char linea_respuesta[MAX_LINEA + 20]; // Buffer para una sola línea de respuesta (incluye año: y \n)
    ssize_t bytes_leidos;
    bool encontrado_alguno = false; // Para saber si se encontró al menos un resultado
    
    printf("\n>> Resultado(s):\n");

    printf("Año:BibNumber,ItemBarcode,ItemType,Collection,CallNumber,CheckoutDateTime\n");

    // Leer en un bucle hasta que el backend cierre su extremo de escritura
    while ((bytes_leidos = read(fd_res, linea_respuesta, sizeof(linea_respuesta) - 1)) > 0) {
        linea_respuesta[bytes_leidos] = '\0'; // Null-terminate la línea leída

        // Verificar si es la señal "NOT_FOUND" o un resultado real
        if (strcmp(linea_respuesta, "NOT_FOUND") == 0) {
            if (!encontrado_alguno) { // Si no se encontró ninguno antes
                printf(">> Registro no encontrado.\n");
            }
            // Si ya se encontraron algunos resultados antes de un NOT_FOUND (ej. por error en backend)
            // entonces no imprimimos NOT_FOUND, solo los resultados encontrados.
            break; // Salir del bucle de lectura si se recibe NOT_FOUND
        } else {
            printf("%s", linea_respuesta); // Imprime la línea, ya contiene el \n
            encontrado_alguno = true;
        }
    }
    
    close(fd_res); // Cerrar el pipe de respuesta

    if (bytes_leidos == -1) { // Si hubo un error en read
        perror("Error al leer de la tubería de respuesta");
    } else if (bytes_leidos == 0 && !encontrado_alguno) { // Si read devolvió 0 y no se encontró nada
        printf(">> No se recibió respuesta del backend o registro no encontrado.\n");
    }
    // Si bytes_leidos == 0 y encontrado_alguno es true, significa que se imprimieron resultados y luego el backend cerró el pipe.
}
// void limpiar_buffer_stdin() {
//     int c;
//     while ((c = getchar()) != '\n' && c != EOF);
// }

// // Envía una solicitud y espera una respuesta
// void enviar_y_recibir(const char *solicitud) {
//     // Enviar solicitud
//     int fd_req = open(REQUEST_PIPE, O_WRONLY);
//     if (fd_req == -1) {
//         perror("Error al abrir tubería de solicitud. ¿El backend está corriendo?");
//         return;
//     }
//     write(fd_req, solicitud, strlen(solicitud) + 1);
//     close(fd_req);

//     // Esperar y recibir respuesta
//     int fd_res = open(RESPONSE_PIPE, O_RDONLY);
//     if (fd_res == -1) {
//         perror("Error al abrir tubería de respuesta");
//         return;
//     }

//     char buffer[MAX_LINEA * (ULTIMO_ANO - PRIMER_ANO + 1)]; // Buffer grande para GET_ALL
//     ssize_t bytes_leidos = read(fd_res, buffer, sizeof(buffer) - 1);
//     close(fd_res);

//     if (bytes_leidos > 0) {
//         buffer[bytes_leidos] = '\0';
//         if (strcmp(buffer, "NOT_FOUND") == 0) {
//             printf("\n>> Registro no encontrado.\n");
//         } else {
//             printf("\n>> Resultado(s):\n%s\n", buffer);
//         }
//     } else {
//          printf("\n>> No se recibió respuesta del backend.\n");
//     }
// }

void buscar_por_id_ano() {
    long id;
    int ano;
    char solicitud[100];

    printf("\nAño (2005-2017): ");
    if (scanf("%d", &ano) != 1 || ano < PRIMER_ANO || ano > ULTIMO_ANO) {
        printf("Año no válido.\n");
        limpiar_buffer_stdin();
        return;
    }
    
    printf("ID a buscar: ");
    if (scanf("%ld", &id) != 1) {
        printf("ID no válido.\n");
        limpiar_buffer_stdin();
        return;
    }
    limpiar_buffer_stdin();

    snprintf(solicitud, sizeof(solicitud), "GET %ld %d", id, ano);
    enviar_y_recibir(solicitud);
}

void buscar_en_todos_anos() {
    long id;
    char solicitud[100];
    printf("\nID a buscar en todos los años: ");
    if (scanf("%ld", &id) != 1) {
        printf("ID no válido.\n");
        limpiar_buffer_stdin();
        return;
    }
    limpiar_buffer_stdin();

    snprintf(solicitud, sizeof(solicitud), "GET_ALL %ld", id);
    enviar_y_recibir(solicitud);
}

void menu_principal() {
    char opcion;
    do {
        printf("\n--- MENU PRINCIPAL ---\n");
        printf("1) Buscar por ID y año\n");
        printf("2) Buscar ID en todos los años\n");
        printf("3) Salir\n");
        printf("Opción: ");
        
        scanf(" %c", &opcion);
        limpiar_buffer_stdin();

        switch (opcion) {
            case '1': buscar_por_id_ano(); break;
            case '2': buscar_en_todos_anos(); break;
            case '3': 
                printf("Enviando señal de cierre al backend...\n");
                enviar_y_recibir("QUIT");
                printf("Saliendo...\n");
                break;
            default: printf("Opción no válida\n");
        }
    } while (opcion != '3');
}

int main() {
    printf("\n=== Cliente del Sistema de Búsqueda de Checkouts ===\n");
    printf("Asegúrate de que el proceso 'backend' esté en ejecución.\n");
    
    menu_principal();
    
    printf("Programa finalizado.\n");
    return EXIT_SUCCESS;
}
