#include "Puntuacion.hpp"

#include "constantes.hpp"

namespace juego {

void Puntuacion::reiniciar() {
    puntos_ = 0;
    nivel_  = 1;
    lineas_ = 0;
}

void Puntuacion::registrarLineas(int n) {
    if (n < 0 || n > 4) return;
    puntos_ += PUNTOS_LINEA[n] * nivel_;
    lineas_ += n;
    actualizarNivel();
}

void Puntuacion::registrarSoftDrop() {
    puntos_ += PUNTOS_SOFT_DROP;
}

void Puntuacion::actualizarNivel() {
    int calculado = 1 + lineas_ / LINEAS_POR_NIVEL;
    if (calculado > MAX_NIVEL) calculado = MAX_NIVEL;
    nivel_ = calculado;
}

void Puntuacion::cargar(const std::string& rutaJson, const std::string& rutaLegado) {
    guardado_ = cargarGuardado(rutaJson, rutaLegado);
}

bool Puntuacion::cerrarPartida(const std::string& rutaJson) {
    const bool superado = puntos_ > guardado_.record;
    if (superado) guardado_.record = puntos_;

    guardado_.partidas.insert(guardado_.partidas.begin(), puntos_);
    if (static_cast<int>(guardado_.partidas.size()) > MAX_PARTIDAS_GUARDADAS)
        guardado_.partidas.resize(MAX_PARTIDAS_GUARDADAS);

    guardar(rutaJson);
    return superado;
}

bool Puntuacion::guardar(const std::string& rutaJson) {
    return guardarGuardado(rutaJson, guardado_);
}

void Puntuacion::reiniciarGuardado(const std::string& rutaJson) {
    guardado_.record = 0;
    guardado_.partidas.clear();
    guardar(rutaJson);
}

} // namespace juego
