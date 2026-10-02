#include "Tablero.hpp"

#include <algorithm>

namespace juego {

void Tablero::reiniciar() {
    for (auto& fila : celdas_) fila.fill(VACIA);
}

bool Tablero::libre(int x, int y) const {
    if (x < 0 || x >= ANCHO_TABLERO) return false; // paredes laterales
    if (y >= ALTO_TABLERO) return false;           // suelo
    if (y < 0) return true;                        // zona de spawn: siempre libre
    return celdas_[y][x] == VACIA;
}

bool Tablero::cabe(const std::array<Celda, 4>& celdas) const {
    for (const auto& c : celdas)
        if (!libre(c.x, c.y)) return false;
    return true;
}

bool Tablero::fijar(const Tetramino& pieza) {
    bool todoVisible = true;
    for (const auto& c : pieza.celdas()) {
        if (c.y < 0) { todoVisible = false; continue; }
        if (c.y >= ALTO_TABLERO || c.x < 0 || c.x >= ANCHO_TABLERO) continue;
        celdas_[c.y][c.x] = Tetramino::colorDe(pieza.tipo());
    }
    return todoVisible;
}

std::vector<int> Tablero::lineasCompletas() const {
    std::vector<int> filas;
    for (int y = 0; y < ALTO_TABLERO; ++y) {
        bool llena = true;
        for (int x = 0; x < ANCHO_TABLERO; ++x)
            if (celdas_[y][x] == VACIA) { llena = false; break; }
        if (llena) filas.push_back(y);
    }
    return filas;
}

void Tablero::eliminarFilas(const std::vector<int>& filas) {
    if (filas.empty()) return;
    std::vector<bool> borrar(ALTO_TABLERO, false);
    for (int f : filas) borrar[f] = true;

    // Reescribe el tablero compactando las filas supervivientes hacia abajo.
    std::array<std::array<int, ANCHO_TABLERO>, ALTO_TABLERO> destino;
    for (auto& fila : destino) fila.fill(VACIA);

    int escritor = ALTO_TABLERO - 1;
    for (int y = ALTO_TABLERO - 1; y >= 0; --y) {
        if (borrar[y]) continue;
        destino[escritor--] = celdas_[y];
    }
    celdas_ = destino;
}

int Tablero::colorEn(int x, int y) const {
    if (x < 0 || x >= ANCHO_TABLERO || y < 0 || y >= ALTO_TABLERO)
        return color::VACIO;
    return celdas_[y][x];
}

bool Tablero::topeAlcanzado() const {
    for (int x = 0; x < ANCHO_TABLERO; ++x)
        if (celdas_[0][x] != VACIA) return true;
    return false;
}

} // namespace juego
