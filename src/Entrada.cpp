#include "Entrada.hpp"

#include <poll.h>
#include <termios.h>
#include <unistd.h>

#include <new>

namespace juego {

namespace {

// Copia del estado original de termios, alojada en el heap para no exponer
// <termios.h> en el header.
struct TerminiosGuardados {
    termios original{};
};

// Lee un byte si llega dentro de `timeoutMs`; 0 si no llega nada.
unsigned char leerRapido(int fd, int timeoutMs) {
    pollfd pfd{ fd, POLLIN, 0 };
    if (::poll(&pfd, 1, timeoutMs) <= 0) return 0;
    unsigned char c = 0;
    return (::read(fd, &c, 1) == 1) ? c : 0;
}

} // namespace

Entrada::Entrada() : fd_(STDIN_FILENO) {
    auto* g = new (std::nothrow) TerminiosGuardados;
    if (!g) return;

    termios actual{};
    if (::tcgetattr(fd_, &actual) != 0) {
        delete g;
        return;
    }
    g->original = actual;

    // Modo raw minimo: sin eco, sin canonicidad, lectura byte a byte.
    termios crudo = actual;
    crudo.c_lflag &= ~static_cast<tcflag_t>(ECHO | ICANON | ISIG | IEXTEN);
    crudo.c_iflag &= ~static_cast<tcflag_t>(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    crudo.c_oflag &= ~static_cast<tcflag_t>(OPOST);
    crudo.c_cc[VMIN]  = 0;
    crudo.c_cc[VTIME] = 0;

    if (::tcsetattr(fd_, TCSANOW, &crudo) != 0) {
        delete g;
        return;
    }

    guardado_ = g;
    activo_   = true;
}

Entrada::~Entrada() {
    desactivar();
    delete static_cast<TerminiosGuardados*>(guardado_);
    guardado_ = nullptr;
}

void Entrada::desactivar() {
    if (!activo_) return;
    auto* g = static_cast<TerminiosGuardados*>(guardado_);
    if (g) ::tcsetattr(fd_, TCSANOW, &g->original);
    activo_ = false;
}

Tecla Entrada::leer(int timeoutMs) {
    pollfd pfd{ fd_, POLLIN, 0 };
    int    listo = ::poll(&pfd, 1, timeoutMs);
    if (listo <= 0) return Tecla::NINGUNA;

    unsigned char c = 0;
    ssize_t leidos = ::read(fd_, &c, 1);
    if (leidos == 0) return Tecla::SALIR;  // EOF (stdin cerrado): salir a buen recaudo
    if (leidos < 0) return Tecla::NINGUNA; // interrupcion u otra condicion transitoria

    switch (c) {
        case 'a': case 'A': return Tecla::IZQUIERDA;
        case 'd': case 'D': return Tecla::DERECHA;
        case 'w': case 'W': return Tecla::ARRIBA;
        case 's': case 'S': return Tecla::ABAJO;
        case 'x': case 'X': return Tecla::ROTAR_CCW;
        case 'p': case 'P': return Tecla::PAUSA;
        case 'r': case 'R': return Tecla::REINICIAR;
        case 'q': case 'Q': return Tecla::SALIR;
        case '1':           return Tecla::OPCION_1;
        case '2':           return Tecla::OPCION_2;
        case '3':           return Tecla::OPCION_3;
        case '4':           return Tecla::OPCION_4;
        case 0x1B: {
            // Las flechas llegan como la secuencia ESC + '[' + letra.
            // Un ESC aislado (sin continuation) sigue significando salir.
            const unsigned char s = leerRapido(fd_, 30);
            if (s == 0) return Tecla::SALIR;
            if (s != '[' && s != 'O') return Tecla::NINGUNA;
            switch (leerRapido(fd_, 30)) {
                case 'A': return Tecla::ARRIBA;
                case 'B': return Tecla::ABAJO;
                case 'C': return Tecla::DERECHA;
                case 'D': return Tecla::IZQUIERDA;
                default:  return Tecla::NINGUNA;
            }
        }
        case '\r':
        case '\n':          return Tecla::CONFIRMAR;
        default:            return Tecla::NINGUNA;
    }
}

} // namespace juego
