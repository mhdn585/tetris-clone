#ifndef RENDERIZADOR_HPP
#define RENDERIZADOR_HPP

#include <string>
#include <vector>

#include "Puntuacion.hpp"
#include "Tablero.hpp"
#include "Tetramino.hpp"
#include "constantes.hpp"

// ============================================================================
//  Renderizador
//  Dibuja cada fotograma COMPLETO en un unico string y lo escribe de una sola
//  pasada sobre stdout: eso elimina el parpadeo (efecto de doble bufer).
//  Usa codigos de escape ANSI (POSIX) en lugar de ncurses.
//
//  Cada bloque del tablero se representa con el texto "[]" tintado con el
//  color unico de su tetramino, tal pide el diseno ASCII del proyecto.
//
//  La paleta activa decide como se tintan TODOS los caracteres: CLASICA usa
//  el color propio de cada elemento; MATRIX fuerza el verde fosforo unico.
// ============================================================================

namespace juego {

class Renderizador {
public:
    // Oculta el cursor y limpia la pantalla. Llamar una vez al iniciar.
    void iniciar();

    // Muestra el cursor y deja la terminal limpia al terminar.
    void terminar();

    // Borra TODA la pantalla y lleva el cursor al inicio. Usar en las
    // transiciones MENU -> JUGANDO y JUGANDO/FIN -> MENU para que no queden
    // restos del menu junto al tablero.
    void limpiarPantalla();

    // Cambia la paleta usada por todo el pintado.
    void fijarPaleta(Paleta p) { paleta_ = p; }

    // --- Pantallas -----------------------------------------------------------
    void pintarMenu(int seleccion, int record);
    void pintarAyuda();
    void pintarAjustes(int seleccion, int record, Paleta paleta,
                       const std::vector<int>& partidas, bool esperandoConfirmar);
    void pintarPartida(const Tablero& tablero, const Tetramino* pieza,
                       Tipo siguiente, const Puntuacion& puntos,
                       bool enPausa, const std::vector<int>& filasDestello);
    void pintarFin(const Tablero& tablero, const Puntuacion& puntos,
                   bool esNuevoRecord);

private:
    std::string lienzo_;
    Paleta paleta_ = Paleta::CLASICA; // paleta de color activa
    int anchoPantalla_ = 44; // columnas reales de la terminal (para centrar)
    int altoPantalla_  = 24; // filas reales de la terminal (para centrar)

    // Tinten centralizados: en MATRIX todo sale en verde fosforo.
    std::string pinta(int color, const std::string& texto) const;
    std::string centradoPintado(const std::string& texto, int ancho,
                                int color, bool negrita) const;

    void abrirFrame();  // relee el tamano real y pone el cursor al inicio
    void escribirLinea(const std::string& linea);
    void cerrarFrame(); // borra el resto y vuelca en stdout

    // Escribe un bloque de lineas ya compuestas, centrado horizontal y
    // verticalmente segun el tamano actual de la terminal.
    void pintarBloqueCentrado(const std::vector<std::string>& lineas);

    // Reconstruye el tablero visible como filas de texto ya coloreado.
    std::vector<std::string> filasTablero(const Tablero& tablero,
                                          const Tetramino* pieza,
                                          const std::vector<int>& filasDestello) const;

    std::vector<std::string> filasPanel(const Puntuacion& puntos,
                                        Tipo siguiente) const;

    // Proxima pieza del panel: limpia el area de la preview (hace desaparecer
    // la pieza anunciada anterior) y dibuja la nueva recortada a su caja util.
    void pintarPiezaSiguiente(std::vector<std::string>& panel,
                              Tipo siguiente) const;

    // Previsualizacion de la proxima pieza (caja recortada a su bounding box).
    std::string bloquePieza(const std::string& texto) const;
};

} // namespace juego

#endif // RENDERIZADOR_HPP
