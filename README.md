# Proyecto LSR — Explorador de Directorios en C

Este proyecto implementa un programa similar al comando `ls` de Linux, llamado **`lsr`**, que permite listar el contenido de directorios mostrando información detallada sobre archivos y carpetas, incluyendo permisos, propietarios, tamaño y fechas.  

El programa fue desarrollado en lenguaje **C**, empleando llamadas al sistema y estructuras propias de Unix/Linux como `stat`, `dirent` y `pwd`.  

---

## Funcionalidad Principal

El programa permite:

- Mostrar el contenido de un directorio de forma detallada (`-l`).
- Incluir archivos ocultos (`-a`).
- Mostrar los tamaños en formato legible para humanos (`-H`).
- Listar recursivamente el contenido de subdirectorios (`-R`).
- Combinar opciones como `-alH` o `-Rl`.

## Compilación

Compila el programa con el siguiente comando:

```bash
gcc -std=c11 -Wall -Wextra -o lsr lsr.c
```

Luego ejecútalo con:

```bash
./lsr [opciones] [ruta]
```

## Ejemplos de uso

Listado simple:

```bash
./lsr
```

Listado detallado con archivos ocultos:

```bash
./lsr -al
```

Tamaños legibles y recursivo

```bash
./lsr -RH /home/usuario 
```

Combinación de opciones:

```bash
./lsr -alRH /etc
```

## Requisitos

- Sistema operativo Linux o basado en Unix.

- Compilador GCC.

- Permisos de lectura sobre los directorios a listar.