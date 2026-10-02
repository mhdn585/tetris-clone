#ifndef TABLERO_HPP
#define TABLERO_HPP

#include <array>
#include <vector>

#include "Tetramino.hpp"
#include "constantes.hpp"

// ============================================================================
//  Tablero
//  Pozo rectangular de 10x20 celdas. Almacena que color tiene cada celda
//  ocupada y ofrece las operaciones de reglas del juego: colisiones, fijar
//  piezas, detectar y eliminar lineas completas.
// ============================================================================

namespace juego {

class Tablero {
public:
    static constexpr int VACIA = -1; // valor de celda sin bloque

    Tablero() { reiniciar(); }

    void reiniciar();

    // Area de juego y limites. Una celda por encima de la fila 0 es libre
    // (permite spawn y kicks hacia arriba); paredes y suelo bloquean.
    bool libre(int x, int y) const;

    // ¿Pueden situarse ahi estas cuatro celdas?
    bool cabe(const std::array<Celda, 4>& celdas) const;
    bool cabe(const Tetramino& pieza) const { return cabe(pieza.celdas()); }

    // Fija la pieza en el tablero. Devuelve false si algun bloque quedo por
    // encima de la zona visible (condicion de "tope" / game over clasico).
    bool fijar(const Tetramino& pieza);

    // Filas (0 = arriba) completamente rellenas.
    std::vector<int> lineasCompletas() const;

    // Elimina las filas indicadas desplazando el contenido hacia abajo.
    void eliminarFilas(const std::vector<int>& filas);

    // Color ANSI guardado en una celda ocupada (o color::VACIO si esta libre).
    int colorEn(int x, int y) const;

    // ¿La fila superior esta ocupada? (visualizacion del "tope")
    bool topeAlcanzado() const;

    // Acceso de solo lectura para el renderizador.
    static constexpr int ancho() { return ANCHO_TABLERO; }
    static constexpr int alto() { return ALTO_TABLERO; }
    int celdaEn(int x, int y) const { return celdas_[y][x]; }

private:
    std::array<std::array<int, ANCHO_TABLERO>, ALTO_TABLERO> celdas_{};
};

} // namespace juego

#endif // TABLERO_HPP
