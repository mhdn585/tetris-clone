#ifndef JUEGO_HPP
#define JUEGO_HPP

#include <chrono>
#include <optional>
#include <string>

#include "Bolsa.hpp"
#include "Entrada.hpp"
#include "Puntuacion.hpp"
#include "Renderizador.hpp"
#include "Tablero.hpp"
#include "Tetramino.hpp"

// ============================================================================
//  Juego
//  Orquestador: maquina de estados (menu / ayuda / partida / pausa / fin),
//  bucle principal con temporizadores, gravedad con la tabla Nintendo,
//  lock delay, rotacion con wall kicks y eliminacion de lineas.
// ============================================================================

namespace juego {

class Juego {
public:
    Juego();

    // Bucle principal. Devuelve el codigo de salida del proceso.
    int ejecutar();

private:
    enum class Estado { MENU, AYUDA, AJUSTES, JUGANDO, FIN, SALIR };
    using Reloj    = std::chrono::steady_clock;
    using Instante = Reloj::time_point;

    // --- Pantallas -----------------------------------------------------------
    void bucleMenu();
    void accionMenu(Tecla t);               // navegar/confirmar en el menu
    void confirmarOpcionMenu(int opcion);   // ejecutar la opcion elegida
    void bucleAyuda();
    void bucleAjustes();
    void accionAjustes(Tecla t);            // navegar/confirmar en los ajustes
    void confirmarAjuste();                 // accion del item seleccionado
    void elegirPaleta(int p);               // aplica y persiste la paleta
    void buclePartida();
    void bucleFin();

    // --- Reglas del juego ------------------------------------------------------
    void nuevaPartida();
    bool generarPieza();                    // false = no cabe => fin de partida
    bool intentarMover(int dx, int dy);
    bool intentarRotar(int sentido);        // +1 horario, -1 antihorario
    void accionSuave();                     // soft drop con puntos
    void procesarTecla(Tecla t);
    void logicaTemporal();                  // gravedad + lock delay
    void bloquearPieza();                   // fija, puntua, elimina lineas
    void destelloLineas(const std::vector<int>& filas);
    void terminarPartida();
    void onAccionExitosa();                 // reset del lock delay con limite

    // --- Utilidades -------------------------------------------------------------
    static Instante ahora() { return Reloj::now(); }
    static long transcurridoMs(Instante desde);
    long velocidadMs() const;               // caida del nivel actual
    static std::string rutaGuardado();      // JSON: record + historial + paleta
    static std::string rutaRecordLegado();  // texto plano antiguo (migra una vez)

    Entrada       entrada_;
    Renderizador  render_;
    Tablero       tablero_;
    Bolsa         bolsa_;
    Puntuacion    puntos_;
    std::optional<Tetramino> pieza_;
    Tipo          siguiente_ = Tipo::T;

    Estado estado_        = Estado::MENU;
    int    seleccionMenu_ = 0;
    bool   enPausa_       = false;
    bool   nuevoRecord_   = false;

    int  seleccionAjustes_ = 0;   // item elegido en la pantalla de ajustes
    bool confirmarReset_   = false; // segundo ENTER para borrar el record

    // Temporizadores de partida.
    Instante tCaida_ {};
    Instante tBloqueo_ {};
    bool     tocandoSuelo_ = false;
    int      resetsBloqueo_ = 0;

    static constexpr int RETRASO_BLOQUEO_MS = 500;  // lock delay clasico
    static constexpr int RESETS_BLOQUEO_MAX = 15;   // anti-stalling infinito
    static constexpr int TICK_MS = 10;              // resolucion del bucle
};

} // namespace juego

#endif // JUEGO_HPP
