#ifndef PUNTUACION_HPP
#define PUNTUACION_HPP

#include <string>
#include <vector>

#include "Datos.hpp"

namespace juego {

// ============================================================================
//  Puntuacion
//  Lleva la cuenta de puntos, nivel y lineas, con la regla clasica:
//  los valores base se multiplican por el nivel vigente. Ademas es la duena
//  del Guardado (record, historial de las ultimas 5 partidas y paleta de
//  color), persistido en un archivo JSON que el propio programa crea.
// ============================================================================

class Puntuacion {
public:
    Puntuacion() = default;

    void reiniciar();

    // Suma puntos al limpiar n lineas (1..4) con el nivel actual.
    void registrarLineas(int n);

    // Suma puntos por soft drop (una celda descendida voluntariamente).
    void registrarSoftDrop();

    // Recalcula el nivel a partir del total de lineas (sube cada 10).
    void actualizarNivel();

    int  puntos() const { return puntos_; }
    int  nivel()  const { return nivel_; }
    int  lineas() const { return lineas_; }
    int  record() const { return guardado_.record; }

    // --- Persistencia JSON ---------------------------------------------------
    // cargar: lee el JSON de rutaJson; si no existe, migra el record del
    // antiguo archivo de texto plano rutaLegado.
    void cargar(const std::string& rutaJson, const std::string& rutaLegado);

    // Cerrar una partida terminada: actualiza el record si se bate, anade los
    // puntos al historial (maximo 5, la mas reciente primero) y guarda.
    // Devuelve true si se ha batido el record.
    bool cerrarPartida(const std::string& rutaJson);

    // Guarda el estado actual tal cual (usado al cambiar de paleta).
    bool guardar(const std::string& rutaJson);

    // Reset desde los ajustes: borra record e historial (en memoria y en el
    // JSON), conservando la paleta.
    void reiniciarGuardado(const std::string& rutaJson);

    // --- Paleta de color ------------------------------------------------------
    int  paleta() const { return guardado_.paleta; }
    void fijarPaleta(int p) { guardado_.paleta = p; }

    const std::vector<int>& partidas() const { return guardado_.partidas; }

    // ¿Se ha batido el record en la partida actual? (antes de cerrar)
    bool nuevoRecord() const { return puntos_ > guardado_.record; }

    void forzarRecord(int valor) { guardado_.record = valor; } // util para pruebas

private:
    int     puntos_ = 0;
    int     nivel_  = 1;
    int     lineas_ = 0;
    Guardado guardado_;
};

} // namespace juego

#endif // PUNTUACION_HPP
