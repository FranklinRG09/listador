#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <getopt.h>
#include <inttypes.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef HAVE_STRDUP
// Implementación personalizada de strdup si no está disponible
char *mi_strdup(const char *cadena) {
    size_t longitud = strlen(cadena) + 1;
    char *puntero = malloc(longitud);
    if (puntero) memcpy(puntero, cadena, longitud);
    return puntero;
}
#define strdup mi_strdup
#endif

// Estructura de opciones de ejecución del programa
typedef struct {
    int mostrar_todos;       // -a: mostrar archivos ocultos
    int formato_largo;       // -l: mostrar detalles (tipo ls -l)
    int tamano_humano;       // -H: mostrar tamaños legibles
    int seguir_enlaces;      // -L: seguir enlaces simbólicos
    int profundidad_maxima;  // -d: limitar profundidad de recursión
} Opciones;

// Estructura para evitar recorrer directorios ya visitados (evita ciclos)
typedef struct Visitado {
    dev_t dispositivo;
    ino_t inodo;
    struct Visitado *siguiente;
} Visitado;

// Declaración de funciones
void mostrar_uso(const char *programa);
void listar_directorio(const char *ruta, Opciones *opciones, int profundidad, Visitado **cabeza_visitados);
int ya_visitado(Visitado *cabeza, dev_t dispositivo, ino_t inodo);
int agregar_visitado(Visitado **cabeza, dev_t dispositivo, ino_t inodo);
void liberar_visitados(Visitado *cabeza);
void imprimir_permisos(mode_t modo);
void formato_tamano_humano(off_t tamano, char *buffer, size_t tamano_buffer);
int comparar_nombres(const void *a, const void *b);

// Función principal
int main(int argc, char *argv[]) {
    Opciones opciones = {0, 0, 0, 0, -1};
    int argumento;

    // Procesar argumentos con getopt
    while ((argumento = getopt(argc, argv, "alHLd:")) != -1) {
        switch (argumento) {
            case 'a': opciones.mostrar_todos = 1; break;
            case 'l': opciones.formato_largo = 1; break;
            case 'H': opciones.tamano_humano = 1; break;
            case 'L': opciones.seguir_enlaces = 1; break;
            case 'd': opciones.profundidad_maxima = atoi(optarg); break;
            default: mostrar_uso(argv[0]); return 1;
        }
    }

    // Validaciones de coherencia entre opciones
    if (opciones.tamano_humano && !opciones.formato_largo) {
        fprintf(stderr, "Error: la opción -H requiere usar también -l.\n");
        return 1;
    }

    if (opciones.profundidad_maxima < -1) {
        fprintf(stderr, "Error: la profundidad (-d) debe ser -1 o un número mayor o igual a 0.\n");
        return 1;
    }

    // Directorio inicial (por defecto ".")
    const char *ruta_inicial = ".";
    if (optind < argc) ruta_inicial = argv[optind];

    Visitado *visitados = NULL;
    listar_directorio(ruta_inicial, &opciones, 0, &visitados);
    liberar_visitados(visitados);
    return 0;
}

// Mostrar mensaje de ayuda
void mostrar_uso(const char *programa) {
    fprintf(stderr, "Uso: %s [-a] [-l] [-H] [-L] [-d profundidad] [directorio]\n", programa);
    fprintf(stderr, "Opciones disponibles:\n");
    fprintf(stderr, "  -a  Muestra archivos ocultos\n");
    fprintf(stderr, "  -l  Muestra formato largo (permisos, usuario, tamaño, fecha)\n");
    fprintf(stderr, "  -H  Tamaños legibles (requiere -l)\n");
    fprintf(stderr, "  -L  Sigue enlaces simbólicos\n");
    fprintf(stderr, "  -d  Limita la profundidad de recursión (-1 = sin límite)\n");
}

// Verificar si un directorio ya fue visitado
int ya_visitado(Visitado *cabeza, dev_t dispositivo, ino_t inodo) {
    for (; cabeza; cabeza = cabeza->siguiente)
        if (cabeza->dispositivo == dispositivo && cabeza->inodo == inodo) return 1;
    return 0;
}

// Agregar un directorio visitado a la lista
int agregar_visitado(Visitado **cabeza, dev_t dispositivo, ino_t inodo) {
    if (ya_visitado(*cabeza, dispositivo, inodo)) return 0;
    Visitado *nuevo = malloc(sizeof(Visitado));
    if (!nuevo) return -1;
    nuevo->dispositivo = dispositivo;
    nuevo->inodo = inodo;
    nuevo->siguiente = *cabeza;
    *cabeza = nuevo;
    return 1;
}

// Liberar memoria de la lista de visitados
void liberar_visitados(Visitado *cabeza) {
    while (cabeza) {
        Visitado *temp = cabeza;
        cabeza = cabeza->siguiente;
        free(temp);
    }
}

// Imprimir permisos en formato rwxr-xr-x
void imprimir_permisos(mode_t modo) {
    char buffer[11] = {'-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '\0'};

    if (S_ISDIR(modo)) buffer[0] = 'd';
    else if (S_ISLNK(modo)) buffer[0] = 'l';

    buffer[1] = (modo & S_IRUSR) ? 'r' : '-';
    buffer[2] = (modo & S_IWUSR) ? 'w' : '-';
    buffer[3] = (modo & S_IXUSR) ? 'x' : '-';
    buffer[4] = (modo & S_IRGRP) ? 'r' : '-';
    buffer[5] = (modo & S_IWGRP) ? 'w' : '-';
    buffer[6] = (modo & S_IXGRP) ? 'x' : '-';
    buffer[7] = (modo & S_IROTH) ? 'r' : '-';
    buffer[8] = (modo & S_IWOTH) ? 'w' : '-';
    buffer[9] = (modo & S_IXOTH) ? 'x' : '-';

    printf("%s", buffer);
}

// Convertir tamaño a formato legible (K, M, G, etc.)
void formato_tamano_humano(off_t tamano, char *buffer, size_t tamano_buffer) {
    const char *sufijos[] = {"B","K","M","G","T"};
    double valor = (double)tamano;
    int indice = 0;
    while (valor >= 1024.0 && indice < 4) { 
        valor /= 1024.0; 
        indice++; 
    }
    snprintf(buffer, tamano_buffer, "%.1f%s", valor, sufijos[indice]);
}

// Comparador para ordenar nombres alfabéticamente
int comparar_nombres(const void *a, const void *b) {
    const char *const *pa = a;
    const char *const *pb = b;
    return strcmp(*pa, *pb);
}

// Función recursiva para listar directorios
void listar_directorio(const char *ruta, Opciones *opciones, int profundidad, Visitado **cabeza_visitados) {
    struct stat estado;
    int resultado_stat = opciones->seguir_enlaces ? stat(ruta, &estado) : lstat(ruta, &estado);
    if (resultado_stat == -1) { perror(ruta); return; }

    // Si es archivo, mostrar y salir
    if (!S_ISDIR(estado.st_mode)) {
        if (opciones->formato_largo) {
            imprimir_permisos(estado.st_mode);
            printf(" %lu", estado.st_nlink);
            struct passwd *usuario = getpwuid(estado.st_uid);
            struct group *grupo = getgrgid(estado.st_gid);
            printf(" %s %s", usuario ? usuario->pw_name : "?", grupo ? grupo->gr_name : "?");

            char buffer_tamano[32];
            if (opciones->tamano_humano)
                formato_tamano_humano(estado.st_size, buffer_tamano, sizeof(buffer_tamano));
            else
                snprintf(buffer_tamano, sizeof(buffer_tamano), "%jd", (intmax_t)estado.st_size);

            char buffer_tiempo[64];
            strftime(buffer_tiempo, sizeof(buffer_tiempo), "%Y-%m-%d %H:%M", localtime(&estado.st_mtime));

            printf(" %7s %s %s\n", buffer_tamano, buffer_tiempo, ruta);
        } else printf("%s\n", ruta);
        return;
    }

    // Evitar ciclos
    if (ya_visitado(*cabeza_visitados, estado.st_dev, estado.st_ino)) {
        printf("(omitido por ciclo): %s\n", ruta);
        return;
    }
    agregar_visitado(cabeza_visitados, estado.st_dev, estado.st_ino);

    // Verificar límite de profundidad
    if (opciones->profundidad_maxima >= 0 && profundidad > opciones->profundidad_maxima)
        return;

    DIR *directorio = opendir(ruta);
    if (!directorio) { perror(ruta); return; }

    struct dirent *entrada;
    size_t cantidad = 0, capacidad = 64;
    char **nombres = malloc(capacidad * sizeof(char*));
    if (!nombres) { perror("malloc"); closedir(directorio); return; }

    // Leer las entradas del directorio
    while ((entrada = readdir(directorio))) {
        if (!opciones->mostrar_todos && entrada->d_name[0] == '.') continue;
        if (cantidad >= capacidad) {
            capacidad *= 2;
            char **temporal = realloc(nombres, capacidad * sizeof(char*));
            if (!temporal) { perror("realloc"); closedir(directorio); return; }
            nombres = temporal;
        }
        nombres[cantidad++] = strdup(entrada->d_name);
    }
    closedir(directorio);

    // Ordenar nombres alfabéticamente
    qsort(nombres, cantidad, sizeof(char*), comparar_nombres);

    printf("\n[%s]\n", ruta);

    // Mostrar cada elemento del directorio
    for (size_t i = 0; i < cantidad; i++) {
        char ruta_completa[PATH_MAX];
        snprintf(ruta_completa, sizeof(ruta_completa), "%s/%s", ruta, nombres[i]);
        struct stat estado2;
        (opciones->seguir_enlaces ? stat(ruta_completa, &estado2) : lstat(ruta_completa, &estado2));

        if (opciones->formato_largo) {
            imprimir_permisos(estado2.st_mode);
            printf(" %lu", estado2.st_nlink);
            struct passwd *usuario = getpwuid(estado2.st_uid);
            struct group *grupo = getgrgid(estado2.st_gid);
            printf(" %s %s", usuario ? usuario->pw_name : "?", grupo ? grupo->gr_name : "?");

            char buffer_tamano[32];
            if (opciones->tamano_humano)
                formato_tamano_humano(estado2.st_size, buffer_tamano, sizeof(buffer_tamano));
            else
                snprintf(buffer_tamano, sizeof(buffer_tamano), "%jd", (intmax_t)estado2.st_size);

            char buffer_tiempo[64];
            strftime(buffer_tiempo, sizeof(buffer_tiempo), "%Y-%m-%d %H:%M", localtime(&estado2.st_mtime));

            printf(" %7s %s %s\n", buffer_tamano, buffer_tiempo, nombres[i]);
        } else printf("%s\n", nombres[i]);

        // Si es directorio, entrar recursivamente
        if (S_ISDIR(estado2.st_mode) && strcmp(nombres[i], ".") && strcmp(nombres[i], "..")) {
            listar_directorio(ruta_completa, opciones, profundidad + 1, cabeza_visitados);
        }
        free(nombres[i]);
    }
    free(nombres);
}
