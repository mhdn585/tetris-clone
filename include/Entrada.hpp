#ifndef ENTRADA_HPP
#define ENTRADA_HPP

// ============================================================================
//  Entrada
//  Lectura de teclado en modo "raw" (sin eco, sin espera de linea) sobre la
//  terminal POSIX. Configura termios al construirse y lo RESTAURA al
//  destruirse (RAII), de modo que la terminal nunca quede inservible tras
//  cerrar el juego, incluso por excepcion.
//
//  Devuelve codigos NEUTROS: el contexto (menu o partida) decide el signi-
//  ficado. Por ejemplo, 'w' llega como ARRIBA y el juego la interpreta como
//  rotacion horaria, mientras el menu la usa para navegar.
//
//  Las flechas llegan como la secuencia ESC + '[' + letra; se consume la
//  secuencia completa y se traduce a ARRIBA/ABAJO/IZQUIERDA/DERECHA. Un ESC
//  suelto significa SALIR.
//
//  No usa ncurses: solo cabeceras estandar de Linux/POSIX.
// ============================================================================

namespace juego {

enum class Tecla {
    NINGUNA,
    IZQUIERDA,   // a o flecha izquierda
    DERECHA,     // d o flecha derecha
    ARRIBA,      // w o flecha arriba (juego: rotar horario | menu: subir)
    ABAJO,       // s o flecha abajo  (juego: soft drop      | menu: bajar)
    ROTAR_CCW,   // x  (rotacion antihoraria)
    PAUSA,       // p
    CONFIRMAR,   // enter
    SALIR,       // q / escape aislado
    REINICIAR,   // r
    OPCION_1,    // 1
    OPCION_2,    // 2
    OPCION_3,    // 3
    OPCION_4     // 4
};

class Entrada {
public:
    Entrada();
    ~Entrada();

    Entrada(const Entrada&)            = delete;
    Entrada& operator=(const Entrada&) = delete;

    // Espera hasta `timeoutMs` una pulsacion y la traduce a Tecla.
    // Devuelve Tecla::NINGUNA si pasa el tiempo sin entradas.
    Tecla leer(int timeoutMs);

    // El constructor activa el modo raw automaticamente; desactivar() lo
    // revierte de forma idempotente (tambien lo hace el destructor).
    void desactivar();

    bool activo() const { return activo_; }

private:
    int  fd_ = -1;      // STDIN
    bool activo_ = false;
    void* guardado_ = nullptr; // copia heap del struct termios original
};

} // namespace juego

#endif // ENTRADA_HPP
