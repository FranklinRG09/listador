# Proyecto LSR — Explorador de Directorios en C

Este proyecto implementa un programa similar al comando `ls` de Linux, llamado **`lsr`**, que permite listar el contenido de directorios mostrando información detallada sobre archivos y carpetas, incluyendo permisos, propietarios, tamaño y fechas.  

El programa fue desarrollado en lenguaje **C**, empleando llamadas al sistema y estructuras propias de Unix/Linux como `stat`, `dirent` y `pwd`.  

---

## Funcionalidad Principal

El programa permite:

- Mostrar el contenido de un directorio de forma detallada (`-l`).
- Incluir archivos ocultos (`-a`).
- Mostrar los tamaños en formato legible para humanos (`-H`).
- Seguir enlaces simbólicos (`-L`).
- Limitar la profundidad de exploración con `-d [nivel]`.

Además, realiza validaciones automáticas para evitar combinaciones de comandos incorrectas, garantizando un comportamiento consistente con el comando ls estándar de Linux.

## Validaciones implementadas

El programa verifica los siguientes casos:

- Si se usa -H sin -l, muestra un error: La opción -H requiere usar también -l.

- Si se especifica una profundidad negativa (con -d), muestra un error: La profundidad debe ser un número mayor o igual a -1.

- Si no se especifica ninguna opción, lista el contenido simple del directorio actual.

- Todas las combinaciones válidas de opciones siguen el formato ./lsr -[opciones] [ruta].

## Recomendación importante

Para garantizar el acceso completo a todos los archivos del sistema, se recomienda instalar o ejecutar el programa en la raíz del directorio del usuario.

## Compilación

Compila el programa con el siguiente comando:

```bash
gcc -std=c11 -Wall -Wextra -o lsr lsr.c
```

Luego ejecútalo con:

```bash
./lsr [opciones] [ruta]
```

## Combinaciones posibles 

### Opciones individuales

Listado simple del directorio actual.
```bash
./lsr
```

Muestra todos los archivos, incluidos los ocultos (los que empiezan con .).
```bash
./lsr -a
```

Formato largo: permisos, número de enlaces, propietario, grupo, tamaño, fecha y nombre.
```bash
./lsr -l
```


Sigue enlaces simbólicos (usa stat() en lugar de lstat()).
```bash
./lsr -L
```

Limita la profundidad de recursión a n.
```bash
./lsr -d <n>
```

- -d -1 → sin límite (valor por defecto en tu código).

- -d 0 → solo el directorio indicado (no entra en subdirectorios).

- -d 1 → un nivel de subdirectorios, etc.

**Nota:** La opción -H no es válida por sí sola (ver más abajo).

### Opciones combinadas

Listado largo incluyendo archivos ocultos.
```bash
./lsr -al o ./lsr -la
```

Listado largo con tamaños legibles (K, M, G).

**Requisito:** -H requiere -l. Si usas -H sin -l, el programa emite un error.
```bash
./lsr -lH o ./lsr -Hl
```

Listado largo y seguir enlaces simbólicos (muestra metadatos del destino del enlace).
```bash
./lsr -lL
```

Archivos ocultos + listado largo + tamaños legibles.
```bash
./lsr -alH
```

Archivos ocultos + listado largo + seguir enlaces simbólicos.
```bash
./lsr -alL
```

Listado largo limitado a 1 nivel de profundidad.
```bash
./lsr -l -d 1 o ./lsr -ld 1
```

Listado completo: ocultos, largo, tamaños legibles, sigue enlaces y profundidad hasta 2 niveles.
```bash
./lsr -alHL -d 2
```

## Requisitos

- Sistema operativo Linux o basado en Unix.

- Compilador GCC.

- Permisos de lectura sobre los directorios a listar.