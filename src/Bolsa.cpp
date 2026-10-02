#include "Bolsa.hpp"

#include <algorithm>

namespace juego {

void Bolsa::rellenar() {
    tanda_.clear();
    for (int i = 0; i < static_cast<int>(Tipo::CANTIDAD); ++i)
        tanda_.push_back(static_cast<Tipo>(i));
    std::shuffle(tanda_.begin(), tanda_.end(), gen_);
    posicion_ = 0;
}

Tipo Bolsa::extraer() {
    if (posicion_ >= tanda_.size()) rellenar();
    return tanda_[posicion_++];
}

void Bolsa::reiniciar() {
    rellenar();
}

void Bolsa::fijarSemilla(unsigned semilla) {
    gen_ = std::mt19937(semilla);
    rellenar();
}

} // namespace juego
