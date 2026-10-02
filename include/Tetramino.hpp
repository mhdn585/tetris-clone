#ifndef TETRAMINO_HPP
#define TETRAMINO_HPP

#include <array>

#include "constantes.hpp"

// ============================================================================
//  Tetramino
//  Pieza del tablero: tipo, estado de rotacion y posicion de su caja envolvente.
//  Usa el sistema SRS (Super Rotation System): cuatro estados de rotacion por
//  pieza y tablas de "wall kicks" para que girar junto a paredes o huecos
//  funcione como en el Tetris moderno.
//  La clase es datos puros: quien valida colisiones es el Tablero.
// ============================================================================

namespace juego {

class Tablero; // declaracion anticipada: evita el ciclo de includes

enum class Tipo { I, O, T, S, Z, J, L, CANTIDAD };

// Coordenada dentro de la caja envolvente de la pieza (x derecha, y abajo).
struct Celda {
    int x;
    int y;
    bool operator==(const Celda& o) const { return x == o.x && y == o.y; }
};

class Tetramino {
public:
    static constexpr int ESTADOS       = 4;                    // 0 = spawn, 1 = R, 2 = 180, 3 = L
    static constexpr int CAJA_I_O      = 4;                    // caja 4x4 para I y O
    static constexpr int CAJA_JLSTZ    = 3;                    // caja 3x3 para el resto
    static constexpr int PRUEBAS_KICK  = 5;                    // patadas por rotacion (SRS)

    Tetramino(Tipo tipo, int xCaja, int yCaja);

    // --- Datos de la pieza ---------------------------------------------------
    Tipo tipo() const { return tipo_; }
    int  x() const { return x_; }
    int  y() const { return y_; }
    int  estado() const { return estado_; }

    // Celdas absolutas que ocupa la pieza en su estado y posicion actuales.
    std::array<Celda, 4> celdas() const;

    // Celdas absolutas para un estado/posicion dados (para probar kicks).
    std::array<Celda, 4> celdasCon(int estado, int x, int y) const;

    // --- Transformaciones (sin validez de colision: la comprueba el juego) ---
    void desplazarse(int dx, int dy) { x_ += dx; y_ += dy; }
    // Fija estado y posicion de golpe (usado para deshacer una rotacion o
    // aplicar una patada valida).
    void fijar(int estado, int x, int y) { estado_ = estado; x_ = x; y_ = y; }
    void rotarLocal(int sentido);

    // --- Informacion estatica por tipo ----------------------------------------
    static int cajaDe(Tipo tipo);                     // lado de la caja envolvente
    static const std::array<Celda, 4>& celdasBase(Tipo tipo, int estado);
    static const char* nombreCorto(Tipo tipo);        // "I", "O", ...
    static int colorDe(Tipo tipo);                    // indice ANSI 256

    // Tabla de wall kicks SRS para la transicion desde->hacia (convencion de
    // pantalla: dy positivo = hacia ABAJO en la terminal).
    static const std::array<Celda, PRUEBAS_KICK>& patadasDe(Tipo tipo, int desde, int hacia);

    // Rota la pieza aplicando la primera patada valida contra el tablero
    // (SRS completo con wall/floor kicks). Devuelve true si pudo rotar.
    static bool rotarConKick(Tetramino& pieza, const Tablero& tablero, int sentido);

private:
    Tipo  tipo_;
    int   x_;       // columna de la esquina superior izquierda de la caja
    int   y_;       // fila de la esquina superior izquierda de la caja
    int   estado_;  // 0..3
};

} // namespace juego

#endif // TETRAMINO_HPP
