#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "common.h" // Incluimos las definiciones comunes

// Función para procesar una línea del CSV y escribirla en los archivos binarios
void procesar_y_escribir(char *linea, int ano, long num_linea, FILE *f_data, FILE *f_index) {
    if (!linea || linea[0] == '\0') return;

    char *original_linea = strdup(linea); // Copia para guardar la línea completa
    if (!original_linea) {
        perror("Fallo al duplicar línea");
        return;
    }
    
    char *campos[MAX_CAMPOS];
    char *saveptr;
    char *token = strtok_r(linea, ",", &saveptr);
    int num_campos = 0;
    
    while (token && num_campos < MAX_CAMPOS) {
        campos[num_campos++] = token;
        token = strtok_r(NULL, ",", &saveptr);
    }

    if (num_campos >= 2) {
        char *endptr;
        long id = strtol(campos[1], &endptr, 10);

        if (endptr != campos[1] && *endptr == '\0' && id >= 0) {
            // Obtener el offset actual en el archivo de datos
            long long offset = ftell(f_data);
            
            // Crear la entrada del índice
            IndexEntry entry = { .id = id, .ano = ano, .offset = offset };
            
            // Escribir la entrada del índice en index.bin
            fwrite(&entry, sizeof(IndexEntry), 1, f_index);
            
            // Escribir la línea original (registro completo) en data.bin
            // Primero escribimos la longitud de la línea, luego la línea misma
            size_t len = strlen(original_linea) + 1; // +1 para el terminador nulo
            fwrite(&len, sizeof(size_t), 1, f_data);
            fwrite(original_linea, sizeof(char), len, f_data);

        } else {
            fprintf(stderr, "[WARN] Línea %ld: ID inválido '%s'\n", num_linea, campos[1]);
        }
    }
    free(original_linea);
}

int main() {
    printf("Iniciando proceso de indexación...\n");

    FILE *f_data = fopen(DATA_FILE, "wb");
    if (!f_data) {
        fprintf(stderr, "Error al crear %s: %s\n", DATA_FILE, strerror(errno));
        return EXIT_FAILURE;
    }

    FILE *f_index = fopen(INDEX_FILE, "wb");
    if (!f_index) {
        fprintf(stderr, "Error al crear %s: %s\n", INDEX_FILE, strerror(errno));
        fclose(f_data);
        return EXIT_FAILURE;
    }

    long total_registros = 0;

    for (int ano = PRIMER_ANO; ano <= ULTIMO_ANO; ano++) {
        char nombre_csv[100];
        snprintf(nombre_csv, sizeof(nombre_csv), "Checkouts_By_Title_Data_Lens_%d.csv", ano);
        
        FILE *f_csv = fopen(nombre_csv, "r");
        if (!f_csv) {
            fprintf(stderr, "[WARN] No se pudo abrir %s, saltando...\n", nombre_csv);
            continue;
        }

        printf("Procesando %s...\n", nombre_csv);

        char linea[MAX_LINEA];
        long num_linea = 0;
        
        // Omitir cabecera
        if(fgets(linea, sizeof(linea), f_csv)) {
            num_linea++;
        }

        while (fgets(linea, sizeof(linea), f_csv)) {
            num_linea++;
            linea[strcspn(linea, "\r\n")] = '\0'; // Limpiar saltos de línea
            if (strlen(linea) > 0) {
                procesar_y_escribir(linea, ano, num_linea, f_data, f_index);
                total_registros++;
            }
        }
        fclose(f_csv);
    }
    
    fclose(f_data);
    fclose(f_index);

    printf("\nIndexación completada.\n");
    printf("Total de registros procesados: %ld\n", total_registros);
    printf("Archivos generados: %s y %s\n", DATA_FILE, INDEX_FILE);

    return EXIT_SUCCESS;
}
