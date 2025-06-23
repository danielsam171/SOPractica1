#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>

#define TAMANO_TABLA 100000
#define MAX_LINEA 4096
#define MAX_CAMPOS 20
#define CRITERIOS_PIPE "/tmp/criterios_pipe"
#define RESULTADOS_PIPE "/tmp/resultados_pipe"

typedef struct Nodo {
    int id;
    char **campos;
    short num_campos;
    struct Nodo *siguiente;
} Nodo;

Nodo **tabla;

unsigned int hash(int clave) {
    return (unsigned int)(clave % TAMANO_TABLA);
}

void insertar(int id, char **campos, short num_campos) {
    unsigned int indice = hash(id);
    Nodo *nuevo = malloc(sizeof(Nodo));
    if (!nuevo) {
        perror("Error de memoria");
        exit(EXIT_FAILURE);
    }
    
    nuevo->id = id;
    nuevo->campos = campos;
    nuevo->num_campos = num_campos;
    nuevo->siguiente = tabla[indice];
    tabla[indice] = nuevo;
}

Nodo *buscar(int id) {
    unsigned int indice = hash(id);
    Nodo *actual = tabla[indice];
    while (actual) {
        if (actual->id == id) return actual;
        actual = actual->siguiente;
    }
    return NULL;
}

void liberar_tabla() {
    if (!tabla) return;
    
    for (int i = 0; i < TAMANO_TABLA; i++) {
        Nodo *actual = tabla[i];
        while (actual) {
            Nodo *temp = actual;
            actual = actual->siguiente;
            
            if (temp->campos) {
                for (int j = 0; j < temp->num_campos; j++) {
                    if (temp->campos[j]) free(temp->campos[j]);
                }
                free(temp->campos);
            }
            free(temp);
        }
    }
    free(tabla);
    tabla = NULL;
}

void cargar_csv(const char *nombre_archivo) {
    FILE *archivo = fopen(nombre_archivo, "r");
    if (!archivo) {
        perror("Error al abrir archivo");
        exit(EXIT_FAILURE);
    }

    char linea[MAX_LINEA];
    const char *delimitador = ",";

    while (fgets(linea, sizeof(linea), archivo)) {
        linea[strcspn(linea, "\n")] = '\0';
        if (linea[0] == '\0') continue;

        char **campos = calloc(MAX_CAMPOS, sizeof(char *));
        if (!campos) {
            perror("Error de memoria");
            exit(EXIT_FAILURE);
        }

        short num_campos = 0;
        char *token = strtok(linea, delimitador);
        
        while (token && num_campos < MAX_CAMPOS) {
            campos[num_campos] = strdup(token);
            if (!campos[num_campos]) {
                perror("Error de memoria");
                exit(EXIT_FAILURE);
            }
            num_campos++;
            token = strtok(NULL, delimitador);
        }

        if (num_campos >= 2) {
            int id = atoi(campos[1]);
            insertar(id, campos, num_campos);
        } else {
            for (int i = 0; i < num_campos; i++) {
                if (campos[i]) free(campos[i]);
            }
            free(campos);
        }
    }
    
    fclose(archivo);
}

int main() {
    tabla = calloc(TAMANO_TABLA, sizeof(Nodo *));
    if (!tabla) {
        perror("Error al crear tabla hash");
        return EXIT_FAILURE;
    }

    // Leer los criterios desde la tubería
    int fd_criterios = open(CRITERIOS_PIPE, O_RDONLY);
    char buffer[256];
    read(fd_criterios, buffer, sizeof(buffer) - 1);
    close(fd_criterios);

    // Parsear los criterios
    char *criterio1 = strtok(buffer, ",");
    char *criterio2 = strtok(NULL, ",");

    // Cargar el archivo correspondiente al año
    char nombre_archivo[100];
    snprintf(nombre_archivo, sizeof(nombre_archivo), 
             "Checkouts_By_Title_Data_Lens_%s.csv", criterio1);
    cargar_csv(nombre_archivo);

    // Buscar el ID
    int id_busqueda = atoi(criterio2);
    Nodo *resultado = buscar(id_busqueda);

    // Enviar el resultado a través de la tubería
    int fd_resultados = open(RESULTADOS_PIPE, O_WRONLY);
    if (resultado) {
        for (int i = 0; i < resultado->num_campos; i++) {
            dprintf(fd_resultados, "%s\n", resultado->campos[i]);
        }
    } else {
        dprintf(fd_resultados, "ID %d no encontrado.\n", id_busqueda);
    }
    close(fd_resultados);

    liberar_tabla();
    return EXIT_SUCCESS;
}