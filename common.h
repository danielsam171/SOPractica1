#ifndef COMMON_H
#define COMMON_H

// --- Nombres de los archivos y tuberías ---
#define DATA_FILE "checkouts.bin"
#define INDEX_FILE "index.bin"
#define REQUEST_PIPE "/tmp/checkout_req_pipe"
#define RESPONSE_PIPE "/tmp/checkout_res_pipe"

// --- Constantes ---
#define MAX_LINEA 4096      // Tamaño máximo de un registro serializado
#define MAX_CAMPOS 20
#define PRIMER_ANO 2005
#define ULTIMO_ANO 2017

// --- Estructuras de Datos ---

// Estructura que se guarda en el archivo de índice (index.bin)
// Contiene la clave (id, ano) y el desplazamiento (offset) en el archivo de datos.
typedef struct {
    long id;
    int ano;
    long long offset; // Usamos long long para offsets en archivos grandes
} IndexEntry;

// --- Protocolo de Comunicación (Mensajes para la tubería) ---
// Formato de solicitud: "TIPO arg1 arg2 ..."
//   - "GET id año" -> Buscar un registro específico
//   - "GET_ALL id" -> Buscar un id en todos los años
//   - "QUIT"       -> Terminar el backend

#endif // COMMON_H

