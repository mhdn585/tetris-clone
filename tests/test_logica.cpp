#include <cstdio>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "Bolsa.hpp"
#include "Datos.hpp"
#include "Puntuacion.hpp"
#include "Tablero.hpp"
#include "Tetramino.hpp"

// ============================================================================
//  Pruebas de logica pura (sin terminal): tablero, reglas, SRS, bolsa y
//  puntuacion. Devuelve codigo != 0 si alguna verificacion falla.
// ============================================================================

using namespace juego;

namespace {

int fallos = 0;

void check(bool condicion, const std::string& que) {
    std::cout << (condicion ? "  [OK]    " : "  [FALLA] ") << que << "\n";
    if (!condicion) ++fallos;
}

void empezar(const std::string& nombre) {
    std::cout << "\n== " << nombre << " ==\n";
}

// Llena la fila `y` por completo con 2 piezas I (cols 0..7) + 1 O (cols 8..9).
void llenarFila(Tablero& tab, int y) {
    const int fila = y - 1; // I estado 0 ocupa la fila yCaja+1
    tab.fijar(Tetramino(Tipo::I, 0, fila));
    tab.fijar(Tetramino(Tipo::I, 4, fila));
    tab.fijar(Tetramino(Tipo::O, 7, y - 1)); // O cubre filas y-1..y cols 8..9
}

} // namespace

int main() {
    empezar("Tablero: limites y colisiones");
    {
        Tablero tab;
        check(tab.libre(0, 0) && tab.libre(9, 19), "celdas interiores libres");
        check(!tab.libre(-1, 5) && !tab.libre(10, 5), "paredes laterales bloquean");
        check(!tab.libre(0, 20), "suelo bloquea");
        check(tab.libre(0, -1), "zona sobre el techo es libre (spawn/kicks)");

        Tetramino i(Tipo::I, 3, 0);
        check(tab.cabe(i), "pieza I cabe en su posicion de nacimiento");
        check(!tab.cabe(Tetramino(Tipo::I, 7, 0)), "I no cabe desplazada fuera de la pared");
    }

    empezar("Tablero: fijar piezas");
    {
        Tablero tab;
        check(tab.fijar(Tetramino(Tipo::O, 0, 18)), "fijar dentro del tablero");
        check(tab.celdaEn(1, 19) != Tablero::VACIA, "las celdas quedan ocupadas");
        check(tab.celdaEn(0, 19) == Tablero::VACIA, "celdas vecinas intactas");
        check(!tab.fijar(Tetramino(Tipo::I, 0, -2)),
              "fijar con bloques sobre el techo reporta tope (game over)");
    }

    empezar("Tablero: deteccion y eliminacion de lineas");
    {
        Tablero tab;
        llenarFila(tab, 19);
        check(tab.celdaEn(8, 19) != Tablero::VACIA, "la fila queda llena");
        auto llenas = tab.lineasCompletas();
        check(llenas.size() == 1 && llenas[0] == 19, "detecta solo la fila 19");

        // Una pieza suelta encima debe bajar al eliminar.
        tab.fijar(Tetramino(Tipo::O, 0, 16)); // filas 16-17, cols 1-2
        tab.eliminarFilas(llenas);
        check(tab.lineasCompletas().empty(), "tras eliminar ya no hay filas llenas");
        check(tab.celdaEn(1, 17) != Tablero::VACIA && tab.celdaEn(1, 18) != Tablero::VACIA,
              "el bloque suelto bajo exactamente una fila");
        check(tab.celdaEn(1, 16) == Tablero::VACIA, "la fila que dejo atras queda vacia");

        // Cuatro lineas a la vez (TETRIS).
        Tablero tab2;
        for (int y = 19; y >= 16; --y) llenarFila(tab2, y);
        check(tab2.lineasCompletas().size() == 4, "detecta un tetris de 4 filas");
    }

    empezar("Tetramino: rotacion SRS");
    {
        Tablero tab;
        Tetramino i(Tipo::I, 3, 0);
        check(Tetramino::rotarConKick(i, tab, +1) && i.estado() == 1,
              "I gira del estado 0 a R");
        Tetramino t(Tipo::T, 3, 5);
        for (int k = 0; k < 4; ++k) Tetramino::rotarConKick(t, tab, +1);
        check(t.estado() == 0 && t.x() == 3 && t.y() == 5,
              "cuatro giros vuelven al estado y posicion originales");
        Tetramino o(Tipo::O, 3, 0);
        const auto antes = o.celdas();
        Tetramino::rotarConKick(o, tab, +1);
        check(o.celdas() == antes, "la pieza O no se desplaza al rotar");
    }

    empezar("Tetramino: wall kicks");
    {
        Tablero tab;
        // I vertical pegada a la pared izquierda (estado L, col x+1=0):
        // girar a horizontal exige la patada (+1,0) del SRS.
        Tetramino i(Tipo::I, 0, 0);
        Tetramino::rotarConKick(i, tab, -1);      // un giro antihorario = estado L
        i.desplazarse(-1, 0);                      // pegar a la pared (col 0)
        check(i.estado() == 3 && i.x() == -1, "preparado: I vertical pegada a la pared");
        check(Tetramino::rotarConKick(i, tab, +1), "rotacion contra pared via kick");
        check(i.estado() == 0 && i.x() == 0, "la patada (+1,0) empujo la pieza al interior");
    }

    empezar("Bolsa: reparto 7-bag");
    {
        Bolsa bolsa;
        bolsa.fijarSemilla(12345u);
        for (int tanda = 0; tanda < 4; ++tanda) {
            std::multiset<int> vistas;
            for (int i = 0; i < 7; ++i) vistas.insert(static_cast<int>(bolsa.extraer()));
            bool completa = true;
            for (int i = 0; i < static_cast<int>(Tipo::CANTIDAD); ++i)
                if (vistas.count(i) != 1) completa = false;
            check(completa, "cada tanda entrega los 7 tipos una sola vez");
        }
    }

    empezar("Puntuacion: valores, nivel y record");
    {
        Puntuacion p;
        p.registrarLineas(1);
        check(p.puntos() == 100, "simple = 100 x nivel 1");
        p.registrarLineas(4);
        check(p.puntos() == 900, "tetris = 800 x nivel 1");
        p.registrarLineas(4);                                    // +800 -> 1700, filas 9
        p.registrarLineas(1);                                    // +100 -> 1800, filas 10
        check(p.lineas() == 10 && p.nivel() == 2, "a las 10 filas el nivel pasa a 2");
        p.registrarLineas(1);                                    // +100 x nivel 2 = +200
        check(p.puntos() == 2000, "el nivel vigente multiplica");

        Puntuacion r;
        r.forzarRecord(1000);
        r.registrarLineas(4);            // 800
        check(!r.nuevoRecord(), "no bate el record quedandose corto");
        r.registrarLineas(2);            // +300 = 1100
        check(r.nuevoRecord(), "bate el record superandolo");
    }

    empezar("Datos: serializacion JSON tolerante");
    {
        Guardado g;
        g.record = 4500;
        g.paleta = 2;
        g.partidas = { 900, 800, 100 };
        const std::string json = serializarGuardado(g);
        check(json.find("\"record\": 4500") != std::string::npos, "claves legibles en el JSON");

        const Guardado h = parsearGuardado(json);
        check(h.record == 4500 && h.paleta == 2 && h.partidas.size() == 3 &&
                  h.partidas[0] == 900,
              "ida y vuelta: los datos sobreviven al parseo");

        const Guardado basura = parsearGuardado("esto no es json {{{ ][");
        check(basura.record == 0 && basura.paleta == 1 && basura.partidas.empty(),
              "parseo tolerante: entrada basura da valores por defecto");

        const Guardado raro = parsearGuardado("{ \"record\": 77, \"paleta\": 9 }");
        check(raro.record == 77 && raro.paleta == 1, "la paleta fuera de rango se descarta");
    }

    empezar("Puntuacion: persistencia JSON, historial y reset");
    {
        const std::string ruta   = "/tmp/tetris_prueba_guardado.json";
        const std::string legado = "/tmp/tetris_prueba_record.txt";
        std::remove(ruta.c_str());
        std::remove(legado.c_str());

        { std::ofstream v(legado); v << 1000; }
        Puntuacion m;
        m.cargar(ruta, legado);
        check(m.record() == 1000, "migra el record del antiguo archivo de texto");

        m.registrarLineas(4);
        m.registrarLineas(4);                       // 1600 puntos
        check(m.cerrarPartida(ruta), "cerrarPartida avisa que batio el record");
        m.reiniciar();
        m.registrarLineas(1);                       // 100 puntos
        check(!m.cerrarPartida(ruta), "una partida corta no bate el record");

        Puntuacion leido;
        leido.cargar(ruta, "/tmp/tetris_prueba_inexistente.txt");
        check(leido.record() == 1600, "el JSON conserva el record batido");
        check(leido.partidas().size() == 2 && leido.partidas()[0] == 100 &&
                  leido.partidas()[1] == 1600,
              "el historial guarda las ultimas partidas, la mas reciente primero");

        for (int i = 0; i < 6; ++i) { leido.reiniciar(); leido.cerrarPartida(ruta); }
        check(leido.partidas().size() == 5, "el historial esta capped a 5 partidas");

        leido.fijarPaleta(2);
        leido.guardar(ruta);
        Puntuacion recargado;
        recargado.cargar(ruta, legado);
        check(recargado.paleta() == 2, "la paleta elegida sobrevive entre sesiones");

        recargado.reiniciarGuardado(ruta);
        check(recargado.record() == 0 && recargado.partidas().empty(),
              "el reset borra record e historial");
        Puntuacion trasReset;
        trasReset.cargar(ruta, legado);
        check(trasReset.record() == 0 && trasReset.paleta() == 2,
              "el reset persistido respeta la paleta configurada");

        std::remove(ruta.c_str());
        std::remove(legado.c_str());
    }

    std::cout << "\n"
              << (fallos == 0 ? "TODAS LAS PRUEBAS PASAN" : "HAY PRUEBAS FALLANDO")
              << " (" << fallos << " fallos)\n";
    return fallos == 0 ? 0 : 1;
}
