#include <sys/ioctl.h>
#include <unistd.h>

#include <cstdio>

#include "Juego.hpp"

// ============================================================================
//  Tetris de terminal (ASCII, 10 x 20)
//  Punto de entrada: valida el entorno y cede el control a Juego.
// ============================================================================

namespace {

constexpr int MIN_FILAS     = 24;
constexpr int MIN_COLUMNAS  = 52; // bloque completo tablero+panel+ayuda de teclas

bool terminalSuficiente() {
    winsize tam{};
    if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &tam) != 0) return true; // sin datos: probar
    if (tam.ws_row == 0 || tam.ws_col == 0) return true;            // pty sin tamano: probar
    return tam.ws_row >= MIN_FILAS && tam.ws_col >= MIN_COLUMNAS;
}

} // namespace

int main() {
    if (!::isatty(STDIN_FILENO)) {
        std::fprintf(stderr,
                     "Tetris necesita una terminal interactiva "
                     "(ejecutalo directamente, sin pipe ni redireccion).\n");
        return 1;
    }
    if (!terminalSuficiente()) {
        std::fprintf(stderr,
                     "Terminal demasiado pequena: se requiere al menos "
                     "%dx%d (columnas x filas).\n",
                     MIN_COLUMNAS, MIN_FILAS);
        return 1;
    }

    juego::Juego juego;
    return juego.ejecutar();
}
