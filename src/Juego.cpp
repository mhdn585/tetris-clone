#include "Juego.hpp"

#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <thread>

namespace juego {

Juego::Juego() = default;

// ============================================================================
//  Bucle principal y maquina de pantallas
// ============================================================================

int Juego::ejecutar() {
    if (!entrada_.activo())
        return 1; // la terminal no admite modo raw (p. ej. stdin redirigido)

    puntos_.cargar(rutaGuardado(), rutaRecordLegado());
    render_.fijarPaleta(static_cast<Paleta>(puntos_.paleta()));
    render_.iniciar();

    while (estado_ != Estado::SALIR) {
        switch (estado_) {
            case Estado::MENU:    bucleMenu();   break;
            case Estado::AYUDA:   bucleAyuda();  break;
            case Estado::AJUSTES: bucleAjustes(); break;
            case Estado::JUGANDO: buclePartida(); break;
            case Estado::FIN:     bucleFin();    break;
            case Estado::SALIR:   break;
        }
    }

    render_.terminar(); // el destructor de Entrada restaura los termios (RAII)
    return 0;
}

void Juego::bucleMenu() {
    render_.limpiarPantalla(); // borra cualquier resto de partida/FIN al volver
    while (estado_ == Estado::MENU) {
        render_.pintarMenu(seleccionMenu_, puntos_.record());
        accionMenu(entrada_.leer(50));
    }
}

// Desplazamiento y seleccion de las opciones del menu principal. Las flechas
// y W/S navegan con envoltura; ENTER confirma y 1/2/3/4 eligen en un salto.
void Juego::accionMenu(Tecla t) {
    switch (t) {
        case Tecla::ARRIBA:    seleccionMenu_ = (seleccionMenu_ + 3) % 4; break;
        case Tecla::ABAJO:     seleccionMenu_ = (seleccionMenu_ + 1) % 4; break;
        case Tecla::OPCION_1:  seleccionMenu_ = 0; confirmarOpcionMenu(0); break;
        case Tecla::OPCION_2:  seleccionMenu_ = 1; confirmarOpcionMenu(1); break;
        case Tecla::OPCION_3:  seleccionMenu_ = 2; confirmarOpcionMenu(2); break;
        case Tecla::OPCION_4:  seleccionMenu_ = 3; confirmarOpcionMenu(3); break;
        case Tecla::CONFIRMAR: confirmarOpcionMenu(seleccionMenu_); break;
        case Tecla::SALIR:     estado_ = Estado::SALIR; break;
        default: break;
    }
}

void Juego::confirmarOpcionMenu(int opcion) {
    switch (opcion) {
        case 0:  nuevaPartida(); break; // JUGAR: nuevaPartida ya pone JUGANDO
        case 1:  estado_ = Estado::AYUDA;   break;
        case 2:  estado_ = Estado::AJUSTES; break;
        default: estado_ = Estado::SALIR;   break; // SALIR
    }
}

void Juego::bucleAyuda() {
    render_.pintarAyuda();
    while (estado_ == Estado::AYUDA) {
        switch (entrada_.leer(50)) {
            case Tecla::CONFIRMAR:
            case Tecla::SALIR:    estado_ = Estado::MENU; break;
            default: break;
        }
    }
}

// Ajustes de interfaz: eleccion de paleta y reset de la puntuacion guardada.
void Juego::bucleAjustes() {
    render_.limpiarPantalla();
    while (estado_ == Estado::AJUSTES) {
        render_.pintarAjustes(seleccionAjustes_, puntos_.record(),
                              static_cast<Paleta>(puntos_.paleta()),
                              puntos_.partidas(), confirmarReset_);
        accionAjustes(entrada_.leer(50));
    }
}

void Juego::accionAjustes(Tecla t) {
    switch (t) {
        case Tecla::ARRIBA:    confirmarReset_ = false;
                               seleccionAjustes_ = (seleccionAjustes_ + 2) % 3; break;
        case Tecla::ABAJO:     confirmarReset_ = false;
                               seleccionAjustes_ = (seleccionAjustes_ + 1) % 3; break;
        case Tecla::OPCION_1:  elegirPaleta(static_cast<int>(Paleta::CLASICA)); break;
        case Tecla::OPCION_2:  elegirPaleta(static_cast<int>(Paleta::MATRIX));  break;
        case Tecla::OPCION_3:  confirmarReset_ = false;
                               seleccionAjustes_ = 2; confirmarAjuste(); break;
        case Tecla::CONFIRMAR: confirmarAjuste(); break;
        case Tecla::SALIR:
            if (confirmarReset_) confirmarReset_ = false; // la primera Q cancela
            else                 estado_ = Estado::MENU;
            break;
        default: break;
    }
}

void Juego::confirmarAjuste() {
    if (seleccionAjustes_ == 0)      elegirPaleta(static_cast<int>(Paleta::CLASICA));
    else if (seleccionAjustes_ == 1) elegirPaleta(static_cast<int>(Paleta::MATRIX));
    else {
        // RESET PUNTUACION: pide una segunda confirmacion antes de borrar.
        if (!confirmarReset_) { confirmarReset_ = true; return; }
        confirmarReset_ = false;
        puntos_.reiniciarGuardado(rutaGuardado());
    }
}

void Juego::elegirPaleta(int p) {
    if (p < static_cast<int>(Paleta::CLASICA) || p > static_cast<int>(Paleta::MATRIX))
        return;
    confirmarReset_ = false;
    puntos_.fijarPaleta(p);
    puntos_.guardar(rutaGuardado()); // se persiste para las proximas sesiones
    render_.fijarPaleta(static_cast<Paleta>(p));
}

void Juego::bucleFin() {
    while (estado_ == Estado::FIN) {
        render_.pintarFin(tablero_, puntos_, nuevoRecord_);
        switch (entrada_.leer(50)) {
            case Tecla::CONFIRMAR: nuevaPartida(); break;
            case Tecla::SALIR:     estado_ = Estado::MENU; break;
            default: break;
        }
    }
}

// ============================================================================
//  Partida
// ============================================================================

void Juego::nuevaPartida() {
    tablero_.reiniciar();
    puntos_.reiniciar();
    siguiente_   = bolsa_.extraer();
    enPausa_     = false;
    nuevoRecord_ = false;
    generarPieza();
    render_.limpiarPantalla(); // quita el menu de la pantalla antes del juego
    estado_ = Estado::JUGANDO;
}

bool Juego::generarPieza() {
    const int caja = Tetramino::cajaDe(siguiente_);
    Tetramino nueva(siguiente_, (ANCHO_TABLERO - caja) / 2, 0);
    siguiente_ = bolsa_.extraer();

    if (!tablero_.cabe(nueva)) {
        pieza_.reset();
        return false; // no hay sitio: fin de partida
    }
    pieza_         = nueva;
    tCaida_        = ahora();
    tBloqueo_      = ahora();
    tocandoSuelo_  = false;
    resetsBloqueo_ = 0;
    return true;
}

void Juego::buclePartida() {
    while (estado_ == Estado::JUGANDO) {
        const Tecla t = entrada_.leer(TICK_MS);
        if (t != Tecla::NINGUNA) procesarTecla(t);

        if (!enPausa_ && pieza_) logicaTemporal();

        render_.pintarPartida(tablero_, pieza_ ? &*pieza_ : nullptr,
                              siguiente_, puntos_, enPausa_, {});
    }
}

void Juego::procesarTecla(Tecla t) {
    if (enPausa_) {
        if (t == Tecla::PAUSA) {
            enPausa_   = false;
            tCaida_    = ahora();
            tBloqueo_  = ahora();
        } else if (t == Tecla::REINICIAR) {
            nuevaPartida();
        } else if (t == Tecla::SALIR) {
            estado_ = Estado::MENU;
        }
        return;
    }

    switch (t) {
        case Tecla::IZQUIERDA: if (intentarMover(-1, 0)) onAccionExitosa(); break;
        case Tecla::DERECHA:   if (intentarMover( 1, 0)) onAccionExitosa(); break;
        case Tecla::ARRIBA:    if (intentarRotar( 1))    onAccionExitosa(); break;
        case Tecla::ROTAR_CCW: if (intentarRotar(-1))    onAccionExitosa(); break;
        case Tecla::ABAJO:     accionSuave(); break;
        case Tecla::PAUSA:     enPausa_ = true; break;
        case Tecla::REINICIAR: nuevaPartida(); break;
        case Tecla::SALIR:     estado_ = Estado::MENU; break;
        default: break;
    }
}

bool Juego::intentarMover(int dx, int dy) {
    if (!pieza_) return false;
    if (tablero_.cabe(pieza_->celdasCon(pieza_->estado(), pieza_->x() + dx,
                                        pieza_->y() + dy))) {
        pieza_->desplazarse(dx, dy);
        return true;
    }
    return false;
}

bool Juego::intentarRotar(int sentido) {
    if (!pieza_) return false;
    return Tetramino::rotarConKick(*pieza_, tablero_, sentido);
}

void Juego::accionSuave() {
    if (intentarMover(0, 1)) {
        puntos_.registrarSoftDrop();
        tCaida_ = ahora();
        onAccionExitosa();
    }
    // Si no puede bajar, el lock delay se encargara de bloquearla.
}

void Juego::onAccionExitosa() {
    // Reinicia la cuenta atras del bloqueo, con limite para no procrastinar
    // la caida eternamente sobre el borde (teoria del "infinite spin").
    if (tocandoSuelo_ && resetsBloqueo_ < RESETS_BLOQUEO_MAX) {
        tBloqueo_ = ahora();
        ++resetsBloqueo_;
    }
}

long Juego::transcurridoMs(Instante desde) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(ahora() - desde).count();
}

long Juego::velocidadMs() const {
    const int nivel = std::clamp(puntos_.nivel(), 1, MAX_NIVEL);
    return VELOCIDAD_NIVEL[nivel];
}

void Juego::logicaTemporal() {
    if (!pieza_) return;

    const bool puedeBajar = tablero_.cabe(
        pieza_->celdasCon(pieza_->estado(), pieza_->x(), pieza_->y() + 1));

    if (tocandoSuelo_ && puedeBajar) tocandoSuelo_ = false;

    if (!puedeBajar) {
        if (!tocandoSuelo_) {
            tocandoSuelo_ = true;
            tBloqueo_     = ahora();
        } else if (transcurridoMs(tBloqueo_) >= RETRASO_BLOQUEO_MS) {
            bloquearPieza();
            return;
        }
    }

    if (puedeBajar && transcurridoMs(tCaida_) >= velocidadMs()) {
        pieza_->desplazarse(0, 1);
        tCaida_ = ahora();
    }
}

void Juego::bloquearPieza() {
    if (!pieza_) { terminarPartida(); return; }

    const bool todoVisible = tablero_.fijar(*pieza_);
    pieza_.reset();

    const auto filas = tablero_.lineasCompletas();
    if (!filas.empty()) {
        destelloLineas(filas);
        puntos_.registrarLineas(static_cast<int>(filas.size()));
        tablero_.eliminarFilas(filas);
    }

    if (!todoVisible || !generarPieza()) terminarPartida();
}

void Juego::destelloLineas(const std::vector<int>& filas) {
    constexpr auto DURACION = std::chrono::milliseconds(110);
    render_.pintarPartida(tablero_, nullptr, siguiente_, puntos_, false, filas);
    std::this_thread::sleep_for(DURACION);
    render_.pintarPartida(tablero_, nullptr, siguiente_, puntos_, false, {});
    std::this_thread::sleep_for(DURACION);
}

void Juego::terminarPartida() {
    nuevoRecord_ = puntos_.nuevoRecord();
    puntos_.cerrarPartida(rutaGuardado()); // record + historial en el JSON
    estado_ = Estado::FIN;
}

// ============================================================================
//  Utilidades
// ============================================================================

std::string Juego::rutaGuardado() {
    const char* home = std::getenv("HOME");
    return (home && *home) ? std::string(home) + "/" + ARCHIVO_GUARDADO
                           : std::string(ARCHIVO_GUARDADO);
}

std::string Juego::rutaRecordLegado() {
    const char* home = std::getenv("HOME");
    return (home && *home) ? std::string(home) + "/" + ARCHIVO_RECORD_LEGADO
                           : std::string(ARCHIVO_RECORD_LEGADO);
}

} // namespace juego
