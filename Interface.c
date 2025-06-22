#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>

// Función para buscar en el archivo .csv
char *buscar_en_csv(const char *criterio1, const char *criterio2, const char *criterio3) {
    FILE *archivo = fopen("datos.csv", "r");
    if (!archivo) {
        g_print("No se pudo abrir el archivo\n");
        return NULL;
    }

    char linea[1024];
    char *resultado = NULL;

    while (fgets(linea, sizeof(linea), archivo)) {
        char *columna1 = strtok(linea, ",");
        char *columna2 = strtok(NULL, ",");
        char *columna3 = strtok(NULL, ",");

        if (columna1 && columna2 && columna3 &&
            strcmp(columna1, criterio1) == 0 &&
            strcmp(columna2, criterio2) == 0 &&
            strcmp(columna3, criterio3) == 0) {
            resultado = malloc(strlen(linea) + 1);
            strcpy(resultado, linea);
            break;
        }
    }

    fclose(archivo);
    return resultado;
}

// Función para crear la interfaz gráfica
void crear_interfaz(const char *titulo, void (*callback)(GtkWidget *, gpointer), gpointer data) {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *entry1, *entry2, *entry3;
    GtkWidget *button;
    GtkWidget *text_view;
    GtkWidget *label1 = gtk_label_new("Escriba el primer criterio de búsqueda:");
    GtkWidget *label2 = gtk_label_new("Escriba el segundo criterio de búsqueda:");
    GtkWidget *label3 = gtk_label_new("Escriba el tercer criterio de búsqueda:");

    // Crear ventana
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), titulo);
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 500);

    // Crear grid para organizar widgets
    grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    // Establecer espacio entre filas y columnas
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

    // Crear campos de entrada
    entry1 = gtk_entry_new();
    entry2 = gtk_entry_new();
    entry3 = gtk_entry_new();

    // Crear área de texto
    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);

    // Crear botón
    button = gtk_button_new_with_label("Enviar");

    // Crear un contenedor para pasar los widgets al callback
    GObject *data_container = g_object_new(G_TYPE_OBJECT, NULL);
    g_object_set_data(data_container, "entry1", entry1);
    g_object_set_data(data_container, "entry2", entry2);
    g_object_set_data(data_container, "entry3", entry3);
    g_object_set_data(data_container, "text_view", text_view);

    // Conectar el botón con el callback
    g_signal_connect(button, "clicked", G_CALLBACK(callback), data_container);

    // Agregar etiquetas y campos de entrada al grid
    gtk_grid_attach(GTK_GRID(grid), label1, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry1, 1, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), label2, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry2, 1, 1, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), label3, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry3, 1, 2, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), button, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), text_view, 0, 4, 2, 1);

    // Mostrar todo
    gtk_widget_show_all(window);

    // Conectar señal para cerrar la ventana
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
}

// Callback para la búsqueda
static void buscar_callback(GtkWidget *widget, gpointer data) {
    GtkWidget *entry1 = g_object_get_data(G_OBJECT(data), "entry1");
    GtkWidget *entry2 = g_object_get_data(G_OBJECT(data), "entry2");
    GtkWidget *entry3 = g_object_get_data(G_OBJECT(data), "entry3");
    GtkWidget *text_view = g_object_get_data(G_OBJECT(data), "text_view");

    const char *criterio1 = gtk_entry_get_text(GTK_ENTRY(entry1));
    const char *criterio2 = gtk_entry_get_text(GTK_ENTRY(entry2));
    const char *criterio3 = gtk_entry_get_text(GTK_ENTRY(entry3));

    char *resultado = buscar_en_csv(criterio1, criterio2, criterio3);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    if (resultado) {
        gtk_text_buffer_set_text(buffer, resultado, -1);
        free(resultado);
    } else {
        gtk_text_buffer_set_text(buffer, "No se encontró ningún registro que cumpla con los criterios.", -1);
    }
}

// Función principal
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    crear_interfaz("Buscador CSV", buscar_callback, NULL);

    gtk_main();

    return 0;
}