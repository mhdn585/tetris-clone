#ifndef BOLSA_HPP
#define BOLSA_HPP

#include <random>
#include <vector>

#include "Tetramino.hpp"

// ============================================================================
//  Bolsa
//  Generador aleatorio "7-bag": reparte las 7 piezas barajadas y, al
//  agotarse, vuelve a barajar otra tanda completa. Es el metodo del Tetris
//  moderno: evita secuencias injustas (p. ej. muchas I seguidas o ninguna).
// ============================================================================

namespace juego {

class Bolsa {
public:
    Bolsa() : gen_(std::random_device{}()) { rellenar(); }

    // Extrae la siguiente pieza de la tanda actual.
    Tipo extraer();

    // Vuelve a empezar la tanda (nueva partida).
    void reiniciar();

    // Permite fijar la semilla (reproducibilidad en pruebas).
    void fijarSemilla(unsigned semilla);

private:
    void rellenar();

    std::vector<Tipo> tanda_;
    std::size_t       posicion_ = 0;
    std::mt19937      gen_;
};

} // namespace juego

#endif // BOLSA_HPP
