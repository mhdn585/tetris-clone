#ifndef CONSTANTES_HPP
#define CONSTANTES_HPP

#include <string>

// ============================================================================
//  constantes.hpp
//  Parametros globales del juego: dimensiones, colores, velocidades y puntuacion.
//  Todas las demas clases dependen de este archivo. Sin dependencias externas.
// ============================================================================

namespace juego {

// --- Geometria del tablero ---------------------------------------------------
inline constexpr int ANCHO_TABLERO = 10;
inline constexpr int ALTO_TABLERO  = 20;

// Ancho en caracteres que ocupa cada bloque de una pieza (ej. "[]").
inline constexpr int ANCHO_BLOQUE = 2;

// Altura reservada sobre el tablero visible para el area de aparicion (spawn).
// Las piezas nacen aqui arriba y aun no cuentan como "arriba" del todo.
inline constexpr int ALTO_SPAWN = 2;

// --- Geometria de la preview del panel lateral --------------------------------
// Fila del panel donde empieza el area de la proxima pieza y filas que ocupa.
// Limpiar estas filas antes de dibujar evita que queden bloques "fantasma" de
// la pieza anunciada anteriormente cuando llega una mas pequena.
// FILA_PREVIEW = 11 deja la fila 10 en blanco como separador entre la palabra
// SIGUIENTE (fila 9) y la pieza: antes quedaban pegadas.
inline constexpr int FILA_PREVIEW  = 11;
inline constexpr int ALTO_PREVIEW  = 5;

// --- Colores (ANSI 256) ------------------------------------------------------
// Cada tetramino posee un color unico, fiel a la paleta clasica de Tetris.
namespace color {
inline constexpr int RESETEAR = 0;   // color por defecto de la terminal
inline constexpr int VACIO    = 240; // celdas vacias del tablero (gris tenue)
inline constexpr int BORDE    = 245; // marcos y separadores
inline constexpr int TEXTO    = 231; // blanco roto para etiquetas
inline constexpr int I        = 51;  // cian
inline constexpr int O        = 226; // amarillo
inline constexpr int T        = 201; // magenta
inline constexpr int S        = 46;  // verde
inline constexpr int Z        = 196; // rojo
inline constexpr int J        = 21;  // azul
inline constexpr int L        = 208; // naranja
inline constexpr int FANTASMA = 238; // (reservado) proyeccion de sombra
} // namespace color

// Genera la secuencia ANSI que activa un color de primer plano de 256.
inline std::string ansi(int codigo) {
    return "\x1b[38;5;" + std::to_string(codigo) + "m";
}
inline const std::string ANSI_RESET = "\x1b[0m";

// --- Representacion de un bloque --------------------------------------------
inline const std::string BLOQUE = "[]";

// --- Curva de velocidad (tabla Nintendo, en milisegundos por caida) ----------
// Indices: nivel de juego (1..MAX_NIVEL). La caida se acelera al subir nivel.
inline constexpr int MAX_NIVEL = 15;
inline const long VELOCIDAD_NIVEL[MAX_NIVEL + 1] = {
    /*0 (sin usar)*/ 1000,
    /*1*/  800, // 48 frames / 60 fps
    /*2*/  717, // 43
    /*3*/  633, // 38
    /*4*/  550, // 33
    /*5*/  467, // 28
    /*6*/  383, // 23
    /*7*/  300, // 18
    /*8*/  217, // 13
    /*9*/  160, // 10 (aceleracion suave al final de la tabla NES)
    /*10*/ 120, // 8
    /*11*/ 100, // 6
    /*12*/ 83,  // 5
    /*13*/ 66,  // 4
    /*14*/ 50,  // 3
    /*15*/ 33   // 2
};

// Lineas necesarias para subir un nivel.
inline constexpr int LINEAS_POR_NIVEL = 10;

// --- Puntuacion (regla clasica moderna, multiplicada por el nivel) ------------
// indice = numero de lineas limpiadas de golpe (0..4).
inline constexpr int PUNTOS_LINEA[5] = { 0, 100, 300, 500, 800 };

// Puntos otorgados por cada celda descendida con soft drop.
inline constexpr int PUNTOS_SOFT_DROP = 1;

// --- Paletas de color ---------------------------------------------------------
// CLASICA: la paleta ANSI original del juego (un color por pieza).
// MATRIX : todo caracter en un unico verde fosforo (#00FF41, terminal 15IE).
enum class Paleta : int {
    CLASICA = 1,
    MATRIX  = 2
};

// Verde fosforo "matrix" en truecolor, usado por la paleta MATRIX.
inline const std::string VERDE_MATRIZ = "\x1b[38;2;0;255;65m";

// --- Archivos de datos ---------------------------------------------------------
// JSON gestionado por el programa: record, historial de partidas y paleta.
inline const std::string ARCHIVO_GUARDADO      = ".tetris_puntos.json";
inline const std::string ARCHIVO_RECORD_LEGADO = ".tetris_record"; // texto plano (migracion)
inline constexpr int MAX_PARTIDAS_GUARDADAS    = 5;

} // namespace juego

#endif // CONSTANTES_HPP
