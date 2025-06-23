#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define CRITERIOS_PIPE "/tmp/criterios_pipe"
#define RESULTADOS_PIPE "/tmp/resultados_pipe"

// Función para lanzar el buscador de forma independiente (doble fork)
void lanzar_buscador() {
    pid_t pid = fork();
    if (pid == 0) { // Primer hijo
        if (setsid() < 0) exit(1);
        pid = fork();
        if (pid == 0) { // Nieto
            // El nieto ejecuta el buscador
            execl("./Buscador", "Buscador", NULL);
            exit(1); // Salir si execl falla
        }
        exit(0); // El primer hijo termina
    }
    // El padre original no espera, simplemente continúa
}

// Callback para enviar criterios y recibir resultados
static void buscar_callback(GtkWidget *widget, gpointer data) {
    GtkWidget *entry1 = g_object_get_data(G_OBJECT(data), "entry1");
    GtkWidget *entry2 = g_object_get_data(G_OBJECT(data), "entry2");
    GtkWidget *text_view = g_object_get_data(G_OBJECT(data), "text_view");

    const char *criterio1 = gtk_entry_get_text(GTK_ENTRY(entry1)); // Año
    const char *criterio2 = gtk_entry_get_text(GTK_ENTRY(entry2)); // ID

    // Crear las tuberías FIFO
    mkfifo(CRITERIOS_PIPE, 0666);
    mkfifo(RESULTADOS_PIPE, 0666);

    // Lanzar el proceso buscador
    lanzar_buscador();

    // Enviar los criterios al buscador
    int fd_criterios = open(CRITERIOS_PIPE, O_WRONLY);
    dprintf(fd_criterios, "%s,%s\n", criterio1, criterio2);
    close(fd_criterios);

    // Recibir los resultados del buscador
    int fd_resultados = open(RESULTADOS_PIPE, O_RDONLY);
    char buffer[1024];
    int n = read(fd_resultados, buffer, sizeof(buffer) - 1);
    buffer[n] = '\0';
    close(fd_resultados);

    // Mostrar el resultado en el GtkTextView
    GtkTextBuffer *buffer_text = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer_text, buffer, -1);

    // Limpiar las tuberías FIFO
    unlink(CRITERIOS_PIPE);
    unlink(RESULTADOS_PIPE);
}

// Función para crear la interfaz gráfica
void crear_interfaz(const char *titulo) {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *entry1, *entry2;
    GtkWidget *button;
    GtkWidget *text_view;
    GtkWidget *label1 = gtk_label_new("Escriba el año:");
    GtkWidget *label2 = gtk_label_new("Escriba el ID:");

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

    // Crear área de texto
    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);

    // Crear botón
    button = gtk_button_new_with_label("Buscar");

    // Crear un contenedor para pasar los widgets al callback
    GObject *data_container = g_object_new(G_TYPE_OBJECT, NULL);
    g_object_set_data(data_container, "entry1", entry1);
    g_object_set_data(data_container, "entry2", entry2);
    g_object_set_data(data_container, "text_view", text_view);

    // Conectar el botón con el callback
    g_signal_connect(button, "clicked", G_CALLBACK(buscar_callback), data_container);

    // Agregar etiquetas y campos de entrada al grid
    gtk_grid_attach(GTK_GRID(grid), label1, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry1, 1, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), label2, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry2, 1, 1, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), button, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), text_view, 0, 3, 2, 1);

    // Mostrar todo
    gtk_widget_show_all(window);

    // Conectar señal para cerrar la ventana
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
}

// Función principal
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    crear_interfaz("Buscador CSV");
    gtk_main();
    return 0;
}