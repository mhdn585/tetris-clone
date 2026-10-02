#include "Tetramino.hpp"

#include <stdexcept>

#include "Tablero.hpp" // solo para rotarConKick

// ============================================================================
//  Implementacion de Tetramino: tablas de rotacion SRS y wall kicks.
// ============================================================================

namespace juego {

namespace {

// --- Celdas de nacimiento (estado 0), dentro de la caja de cada tipo ---------
// convencion: x crece a la derecha, y crece hacia abajo.
const std::array<Celda, 4> SPAWN_J = { { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 } } };
const std::array<Celda, 4> SPAWN_L = { { { 2, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 } } };
const std::array<Celda, 4> SPAWN_S = { { { 1, 0 }, { 2, 0 }, { 0, 1 }, { 1, 1 } } };
const std::array<Celda, 4> SPAWN_T = { { { 1, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 } } };
const std::array<Celda, 4> SPAWN_Z = { { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 2, 1 } } };
const std::array<Celda, 4> SPAWN_I = { { { 0, 1 }, { 1, 1 }, { 2, 1 }, { 3, 1 } } };
const std::array<Celda, 4> SPAWN_O = { { { 1, 0 }, { 2, 0 }, { 1, 1 }, { 2, 1 } } };

// Rotacion horaria dentro de una caja cuadrada de lado n: (x, y) -> (n-1-y, x).
// Esta formula reproduce exactamente los cuatro estados oficiales del SRS.
Celda girarHorario(const Celda& c, int n) {
    return { n - 1 - c.y, c.x };
}

// Siguiente estado a partir del anterior. La pieza O nunca se desplaza al girar.
std::array<Celda, 4> siguienteEstado(const std::array<Celda, 4>& previos, Tipo tipo) {
    if (tipo == Tipo::O) return previos;
    const int n = Tetramino::cajaDe(tipo);
    std::array<Celda, 4> salida;
    for (int i = 0; i < 4; ++i) salida[i] = girarHorario(previos[i], n);
    return salida;
}

const std::array<Celda, 4>& spawnDe(Tipo tipo) {
    switch (tipo) {
        case Tipo::I: return SPAWN_I;
        case Tipo::O: return SPAWN_O;
        case Tipo::T: return SPAWN_T;
        case Tipo::S: return SPAWN_S;
        case Tipo::Z: return SPAWN_Z;
        case Tipo::J: return SPAWN_J;
        case Tipo::L: return SPAWN_L;
        default: throw std::logic_error("tipo de tetramino desconocido");
    }
}

// --- Wall kicks ---------------------------------------------------------------
// Tablas oficiales del SRS convertidas a coordenadas de pantalla
// (dy positivo = hacia abajo). Solo se declaran las transiciones HORARIAS;
// las antihorarias se derivan con la propiedad k(A->B) = -k(B->A).
using Patadas = std::array<Celda, Tetramino::PRUEBAS_KICK>;

const Patadas KICK_JLSTZ_01 = { { { 0, 0 }, { -1, 0 }, { -1, -1 }, { 0, 2 }, { -1, 2 } } };
const Patadas KICK_JLSTZ_12 = { { { 0, 0 }, {  1, 0 }, {  1,  1 }, { 0, -2 }, { 1, -2 } } };
const Patadas KICK_JLSTZ_23 = { { { 0, 0 }, {  1, 0 }, {  1, -1 }, { 0, 2 }, { 1, 2 } } };
const Patadas KICK_JLSTZ_30 = { { { 0, 0 }, { -1, 0 }, { -1,  1 }, { 0, -2 }, { -1, -2 } } };

const Patadas KICK_I_01 = { { { 0, 0 }, { -2, 0 }, {  1, 0 }, { -2, 1 }, { 1, -2 } } };
const Patadas KICK_I_12 = { { { 0, 0 }, { -1, 0 }, {  2, 0 }, { -1, -2 }, { 2, 1 } } };
const Patadas KICK_I_23 = { { { 0, 0 }, {  2, 0 }, { -1, 0 }, {  2, -1 }, { -1, 2 } } };
const Patadas KICK_I_30 = { { { 0, 0 }, {  1, 0 }, { -2, 0 }, {  1, 2 }, { -2, -1 } } };

const Patadas SIN_KICK = { { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } } };

// Tabla completa [tipo][desde][hacia], construida una sola vez.
struct TablaKicks {
    std::array<std::array<std::array<Patadas, Tetramino::ESTADOS>, Tetramino::ESTADOS>,
               static_cast<int>(Tipo::CANTIDAD)> datos{};

    static TablaKicks construir() {
        TablaKicks t{};
        const Patadas horarioJ[4] = { KICK_JLSTZ_01, KICK_JLSTZ_12, KICK_JLSTZ_23, KICK_JLSTZ_30 };
        const Patadas horarioI[4] = { KICK_I_01, KICK_I_12, KICK_I_23, KICK_I_30 };

        auto invertir = [](const Patadas& p) {
            Patadas q;
            for (size_t i = 0; i < p.size(); ++i) q[i] = { -p[i].x, -p[i].y };
            return q;
        };
        auto instalar = [&](Tipo tipo, const Patadas horario[4]) {
            const int it = static_cast<int>(tipo);
            for (int d = 0; d < Tetramino::ESTADOS; ++d) {
                const int h = (d + 1) % Tetramino::ESTADOS;
                t.datos[it][d][h] = horario[d];            // horario: tal cual
                t.datos[it][h][d] = invertir(horario[d]);   // antihorario: signo invertido
            }
        };
        instalar(Tipo::I, horarioI);
        for (Tipo tt : { Tipo::J, Tipo::L, Tipo::S, Tipo::T, Tipo::Z })
            instalar(tt, horarioJ);
        // La pieza O no usa kicks: queda toda su fila en SIN_KICK por defecto.
        return t;
    }
};

const TablaKicks& kicksGlobales() {
    static const TablaKicks TABLA = TablaKicks::construir();
    return TABLA;
}

} // namespace

// --- Constructores y estado ---------------------------------------------------

Tetramino::Tetramino(Tipo tipo, int xCaja, int yCaja)
    : tipo_(tipo), x_(xCaja), y_(yCaja), estado_(0) {}

int Tetramino::cajaDe(Tipo tipo) {
    return (tipo == Tipo::I || tipo == Tipo::O) ? CAJA_I_O : CAJA_JLSTZ;
}

const std::array<Celda, 4>& Tetramino::celdasBase(Tipo tipo, int estado) {
    // Tabla precalculada: 7 tipos x 4 estados, derivada por rotacion horaria.
    static const std::array<std::array<std::array<Celda, 4>, ESTADOS>,
                            static_cast<int>(Tipo::CANTIDAD)> TABLA = [] {
        std::array<std::array<std::array<Celda, 4>, ESTADOS>,
                   static_cast<int>(Tipo::CANTIDAD)> t{};
        for (int i = 0; i < static_cast<int>(Tipo::CANTIDAD); ++i) {
            const Tipo tt = static_cast<Tipo>(i);
            t[i][0] = spawnDe(tt);
            for (int e = 1; e < ESTADOS; ++e) t[i][e] = siguienteEstado(t[i][e - 1], tt);
        }
        return t;
    }();
    return TABLA[static_cast<int>(tipo)][estado % ESTADOS];
}

std::array<Celda, 4> Tetramino::celdasCon(int estado, int x, int y) const {
    std::array<Celda, 4> salida;
    const auto& base = celdasBase(tipo_, estado);
    for (int i = 0; i < 4; ++i) salida[i] = { x + base[i].x, y + base[i].y };
    return salida;
}

std::array<Celda, 4> Tetramino::celdas() const {
    return celdasCon(estado_, x_, y_);
}

void Tetramino::rotarLocal(int sentido) {
    estado_ = (estado_ + (sentido > 0 ? 1 : ESTADOS - 1)) % ESTADOS;
}

const std::array<Celda, Tetramino::PRUEBAS_KICK>& Tetramino::patadasDe(Tipo tipo,
                                                                       int desde,
                                                                       int hacia) {
    if (desde < 0 || desde >= ESTADOS || hacia < 0 || hacia >= ESTADOS)
        return SIN_KICK;
    return kicksGlobales().datos[static_cast<int>(tipo)][desde][hacia];
}

const char* Tetramino::nombreCorto(Tipo tipo) {
    switch (tipo) {
        case Tipo::I: return "I";
        case Tipo::O: return "O";
        case Tipo::T: return "T";
        case Tipo::S: return "S";
        case Tipo::Z: return "Z";
        case Tipo::J: return "J";
        case Tipo::L: return "L";
        default: return "?";
    }
}

int Tetramino::colorDe(Tipo tipo) {
    switch (tipo) {
        case Tipo::I: return color::I;
        case Tipo::O: return color::O;
        case Tipo::T: return color::T;
        case Tipo::S: return color::S;
        case Tipo::Z: return color::Z;
        case Tipo::J: return color::J;
        case Tipo::L: return color::L;
        default: return color::TEXTO;
    }
}

bool Tetramino::rotarConKick(Tetramino& pieza, const Tablero& tablero, int sentido) {
    const int desde = pieza.estado();
    const int hacia = (desde + (sentido > 0 ? 1 : ESTADOS - 1)) % ESTADOS;
    const auto& kicks = patadasDe(pieza.tipo(), desde, hacia);
    const int xPrev = pieza.x();
    const int yPrev = pieza.y();

    for (int i = 0; i < PRUEBAS_KICK; ++i) {
        const int nx = xPrev + kicks[i].x;
        const int ny = yPrev + kicks[i].y; // dy ya esta en coordenadas de pantalla
        if (tablero.cabe(pieza.celdasCon(hacia, nx, ny))) {
            pieza.fijar(hacia, nx, ny);
            return true;
        }
    }
    return false; // ninguna patada valida: la rotacion se ignora
}

} // namespace juego
