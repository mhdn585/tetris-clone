#include "Renderizador.hpp"

#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>

namespace juego {

namespace {

void volcar(const std::string& s) {
    ::write(STDOUT_FILENO, s.data(), s.size());
}

constexpr int ANCHO_CONTENIDO = ANCHO_TABLERO * ANCHO_BLOQUE; // 20 caracteres

// Nuevo marco del pozo:
//   techo:  (!\/\/\/\/...!)   lados: (!  contenido  !)   suelo: *####...*
std::string hacerBordeSup() {
    std::string rayado;
    for (int i = 0; i < ANCHO_TABLERO; ++i) rayado += "\\/";
    return "(!" + rayado + "!)";
}
const std::string BORDE_SUP = hacerBordeSup();
const std::string BORDE_INF = "*" + std::string(ANCHO_CONTENIDO + 2, '#') + "*";
const std::string MARCO_IZQ = "(!";
const std::string MARCO_DER = "!)";

const std::string BORDE_INI = "\x1b[1m";  // negrita
const std::string BORDE_FIN = ANSI_RESET;

std::string centrado(const std::string& texto, int ancho) {
    if (static_cast<int>(texto.size()) >= ancho) return texto.substr(0, ancho);
    const int izq = (ancho - static_cast<int>(texto.size())) / 2;
    return std::string(izq, ' ') + texto + std::string(ancho - izq - static_cast<int>(texto.size()), ' ');
}

// Titulo TETRIS en bloques: cada letra ocupa una celda de 3 bloques (6 chars)
// y recibe el color de un tetramino distinto, siguiendo el ciclo I O T S Z J.
struct LetraTitulo {
    int         color;
    const char* filas[5];
};

const LetraTitulo TITULO[] = {
    { color::I, { "[][][]", "  []  ", "  []  ", "  []  ", "  []  " } }, // T
    { color::O, { "[][][]", "[]    ", "[][]  ", "[]    ", "[][][]" } }, // E
    { color::T, { "[][][]", "  []  ", "  []  ", "  []  ", "  []  " } }, // T
    { color::S, { "[][][]", "[]  []", "[][]  ", "[]  []", "[]  []" } }, // R
    { color::Z, { "[][][]", "  []  ", "  []  ", "  []  ", "[][][]" } }, // I
    { color::J, { "[][][]", "[]    ", "[][][]", "    []", "[][][]" } }, // S
};
constexpr int ANCHO_TITULO = 41; // 6 celdas de 6 chars + 5 separadores

// Sangria para centrar una fila YA coloreada usando su ancho plano real.
std::string sangrar(int anchoPlano, int anchoPantalla) {
    return std::string(std::max(0, (anchoPantalla - anchoPlano) / 2), ' ');
}

// Ancho real (en columnas) de una linea, ignorando sus secuencias ANSI.
// Permite centrar con exactitud filas que llevan codigos de color embebidos.
int anchoPlano(const std::string& linea) {
    int ancho = 0;
    for (size_t i = 0; i < linea.size(); ++i) {
        if (static_cast<unsigned char>(linea[i]) == 0x1b &&
            i + 1 < linea.size() && linea[i + 1] == '[') {
            size_t j = i + 2;
            while (j < linea.size() && linea[j] >= 0x30 && linea[j] <= 0x3f) ++j; // parametros
            if (j < linea.size()) ++j;                                            // byte final
            i = j - 1;
            continue;
        }
        ++ancho;
    }
    return ancho;
}

} // namespace

// ============================================================================
//  Tintado segun la paleta activa
// ============================================================================

// Unico punto de color del juego: en MATRIX hasta el ultimo caracter sale en
// verde fosforo; en CLASICA cada elemento conserva su color de siempre.
std::string Renderizador::pinta(int color, const std::string& texto) const {
    if (paleta_ == Paleta::MATRIX) return VERDE_MATRIZ + texto + ANSI_RESET;
    return ansi(color) + texto + ANSI_RESET;
}

// Centra TEXTO PLANO y luego lo colorea: asi las secuencias ANSI no rompen
// el calculo de anchos.
std::string Renderizador::centradoPintado(const std::string& texto, int ancho,
                                          int color, bool negrita) const {
    return (negrita ? BORDE_INI : "") + pinta(color, centrado(texto, ancho)) +
           (negrita ? BORDE_FIN : "");
}

// ============================================================================
//  Ciclo de vida del lienzo
// ============================================================================

void Renderizador::iniciar() {
    winsize tam{};
    if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &tam) == 0 && tam.ws_col > 0) {
        anchoPantalla_ = tam.ws_col;
        if (tam.ws_row > 0) altoPantalla_ = tam.ws_row;
    }
    volcar("\x1b[2J\x1b[H\x1b[?25l"); // limpiar, home, ocultar cursor
}

void Renderizador::terminar() {
    volcar("\x1b[0m\x1b[2J\x1b[H\x1b[?25h"); // reset, limpiar, mostrar cursor
}

// Borra toda la pantalla de raiz: se usa en las transiciones entre pantallas
// para que no quede ningun resto de la anterior (p. ej. el menu al jugar).
void Renderizador::limpiarPantalla() {
    volcar("\x1b[0m\x1b[2J\x1b[H");
}

// Relee el tamano real de la terminal en cada fotograma (el ioctl cuesta
// microsegundos): asi un redimensionado re-centra el juego al instante.
void Renderizador::abrirFrame() {
    winsize tam{};
    if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &tam) == 0 && tam.ws_col > 0) {
        anchoPantalla_ = tam.ws_col;
        if (tam.ws_row > 0) altoPantalla_ = tam.ws_row;
    }
    lienzo_.clear();
    lienzo_ += "\x1b[H";
}

// El EL por linea (\x1b[K tras reset) borra todo lo que sobro del fotograma
// anterior en esa fila: sin esto, al cambiar la preview o al pasar del menu
// centrado al tablero quedaban bloques "fantasma" a la derecha.
void Renderizador::escribirLinea(const std::string& linea) {
    lienzo_ += linea;
    lienzo_ += "\x1b[0m\x1b[K\r\n";
}

void Renderizador::cerrarFrame() {
    lienzo_ += "\x1b[J"; // borra lo que sobro del frame anterior
    volcar(lienzo_);
}

// Escribe un bloque de lineas ya compuestas centrado horizontal y
// verticalmente. Las filas sobrantes arriba se emiten vacias (su \x1b[K
// limpia cualquier residuo de la pantalla anterior).
void Renderizador::pintarBloqueCentrado(const std::vector<std::string>& lineas) {
    int anchoMax = 0;
    for (const auto& l : lineas) anchoMax = std::max(anchoMax, anchoPlano(l));

    const int margen  = std::max(0, (anchoPantalla_ - anchoMax) / 2);
    const int arriba  = std::max(0, (altoPantalla_ - static_cast<int>(lineas.size())) / 2);
    const std::string sangria(margen, ' ');

    for (int f = 0; f < arriba; ++f) escribirLinea("");
    for (const auto& l : lineas) escribirLinea(sangria + l);
}

// ============================================================================
//  Piezas del tablero
// ============================================================================

std::string Renderizador::bloquePieza(const std::string& texto) const {
    return texto; // hook para cambiar la glifa de bloque en el futuro
}

std::vector<std::string> Renderizador::filasTablero(
    const Tablero& tablero, const Tetramino* pieza,
    const std::vector<int>& filasDestello) const {

    // Celdas como fragmentos de texto independientes (colores embebidos).
    std::vector<std::vector<std::string>> rejilla(
        ALTO_TABLERO, std::vector<std::string>(ANCHO_TABLERO, "  "));

    // 1) Bloques ya fijados.
    for (int y = 0; y < ALTO_TABLERO; ++y)
        for (int x = 0; x < ANCHO_TABLERO; ++x) {
            const int c = tablero.celdaEn(x, y);
            if (c != Tablero::VACIA) rejilla[y][x] = pinta(c, bloquePieza(BLOQUE));
        }

    // 2) Pieza activa por encima de lo fijado.
    if (pieza) {
        const int color = Tetramino::colorDe(pieza->tipo());
        for (const auto& c : pieza->celdas())
            if (c.y >= 0 && c.y < ALTO_TABLERO && c.x >= 0 && c.x < ANCHO_TABLERO)
                rejilla[c.y][c.x] = pinta(color, bloquePieza(BLOQUE));
    }

    // 3) Destello de lineas a punto de eliminarse.
    for (int y : filasDestello)
        for (int x = 0; x < ANCHO_TABLERO; ++x)
            rejilla[y][x] = BORDE_INI + pinta(color::TEXTO, bloquePieza(BLOQUE));

    // 4) Composicion de filas con marcos: (!\/\/\/...!) / (! cel !!) / *####*
    std::vector<std::string> filas;
    filas.reserve(ALTO_TABLERO + 2);
    filas.push_back(pinta(color::BORDE, BORDE_SUP));
    for (int y = 0; y < ALTO_TABLERO; ++y) {
        std::string fila = pinta(color::BORDE, MARCO_IZQ);
        for (int x = 0; x < ANCHO_TABLERO; ++x) fila += rejilla[y][x];
        fila += pinta(color::BORDE, MARCO_DER);
        filas.push_back(fila);
    }
    filas.push_back(pinta(color::BORDE, BORDE_INF));
    return filas;
}

std::vector<std::string> Renderizador::filasPanel(const Puntuacion& puntos,
                                                  Tipo siguiente) const {
    // Panel de 24 filas alineadas con el tablero (22) mas una extra.
    std::vector<std::string> panel(24, "");

    auto titulo = [&](int fila, const std::string& t) {
        panel[fila] = BORDE_INI + pinta(color::TEXTO, t) + BORDE_FIN;
    };
    auto dato = [&](int fila, const std::string& etq, const std::string& val) {
        panel[fila] = pinta(color::BORDE, etq) + " " +
                      BORDE_INI + pinta(color::TEXTO, val) + BORDE_FIN;
    };

    titulo(0, "TETRIS");
    dato(2, "PUNTOS ", std::to_string(puntos.puntos()));
    dato(4, "NIVEL  ", std::to_string(puntos.nivel()));
    dato(5, "LINEAS ", std::to_string(puntos.lineas()));
    dato(7, "RECORD ", std::to_string(std::max(puntos.record(), puntos.puntos())));
    titulo(9, "SIGUIENTE");
    pintarPiezaSiguiente(panel, siguiente);

    panel[15] = pinta(color::BORDE, " A/D  mover");
    panel[16] = pinta(color::BORDE, " W    rotar");
    panel[17] = pinta(color::BORDE, " S    bajar");
    panel[18] = pinta(color::BORDE, " P    pausa");
    panel[19] = pinta(color::BORDE, " R    reiniciar");
    panel[20] = pinta(color::BORDE, " Q    menu");
    return panel;
}

// Dibuja la proxima pieza anunciada en el panel. Primero limpia por completo
// el area de la preview (panel[FILA_PREVIEW .. +ALTO_PREVIEW]) para que la pieza
// anunciada anterior deje de verse cuando llega una mas pequena o angosta;
// luego la pinta recortada a su caja util.
void Renderizador::pintarPiezaSiguiente(std::vector<std::string>& panel,
                                        Tipo siguiente) const {
    for (int i = 0; i < ALTO_PREVIEW; ++i) panel[FILA_PREVIEW + i] = "";

    const auto& base = Tetramino::celdasBase(siguiente, 0);
    int minx = 4, maxx = 0, miny = 4, maxy = 0;
    for (const auto& c : base) {
        minx = std::min(minx, c.x); maxx = std::max(maxx, c.x);
        miny = std::min(miny, c.y); maxy = std::max(maxy, c.y);
    }
    const int color = Tetramino::colorDe(siguiente);
    for (int y = miny; y <= maxy && y - miny < ALTO_PREVIEW; ++y) {
        std::string fila;
        for (int x = minx; x <= maxx; ++x) {
            const bool dentro = std::find(base.begin(), base.end(), Celda{ x, y }) != base.end();
            fila += dentro ? pinta(color, bloquePieza(BLOQUE)) : "  ";
        }
        panel[FILA_PREVIEW + (y - miny)] = "  " + fila;
    }
}

// ============================================================================
//  Pantallas
// ============================================================================

void Renderizador::pintarPartida(const Tablero& tablero, const Tetramino* pieza,
                                 Tipo siguiente, const Puntuacion& puntos,
                                 bool enPausa,
                                 const std::vector<int>& filasDestello) {
    abrirFrame();

    auto filas = filasTablero(tablero, enPausa ? nullptr : pieza, filasDestello);
    auto panel = filasPanel(puntos, siguiente);

    if (enPausa) {
        filas[10] = pinta(color::BORDE, MARCO_IZQ) +
                    centradoPintado(" P A U S A ", ANCHO_CONTENIDO, color::O, true) +
                    pinta(color::BORDE, MARCO_DER);
        filas[12] = pinta(color::BORDE, MARCO_IZQ) +
                    centradoPintado("P para seguir", ANCHO_CONTENIDO, color::TEXTO, false) +
                    pinta(color::BORDE, MARCO_DER);
    }

    std::vector<std::string> lineas;
    lineas.reserve(filas.size() + 2);
    lineas.push_back("  " + panel[0]);
    for (size_t i = 0; i < filas.size(); ++i)
        lineas.push_back(filas[i] + "  " + panel[i + 1]);
    lineas.push_back(pinta(color::BORDE,
                           " A D mover   W rotar   S bajar   P pausa   Q menu"));
    pintarBloqueCentrado(lineas); // tablero + panel, al centro de la terminal
    cerrarFrame();
}

void Renderizador::pintarFin(const Tablero& tablero, const Puntuacion& puntos,
                             bool esNuevoRecord) {
    abrirFrame();
    auto filas = filasTablero(tablero, nullptr, {});
    auto panel = filasPanel(puntos, Tipo::O);

    filas[9]  = pinta(color::BORDE, MARCO_IZQ) +
                centradoPintado(" F I N   D E   L A ", ANCHO_CONTENIDO, color::Z, true) +
                pinta(color::BORDE, MARCO_DER);
    filas[10] = pinta(color::BORDE, MARCO_IZQ) +
                centradoPintado(" P A R T I D A ", ANCHO_CONTENIDO, color::Z, true) +
                pinta(color::BORDE, MARCO_DER);
    filas[12] = pinta(color::BORDE, MARCO_IZQ) +
                centradoPintado(esNuevoRecord ? " ! NUEVO RECORD !" : "  ENTER: repetir",
                                ANCHO_CONTENIDO,
                                esNuevoRecord ? color::S : color::TEXTO, true) +
                pinta(color::BORDE, MARCO_DER);
    filas[13] = pinta(color::BORDE, MARCO_IZQ) +
                centradoPintado(esNuevoRecord ? "  ENTER: repetir" : "     Q: menu",
                                ANCHO_CONTENIDO,
                                esNuevoRecord ? color::TEXTO : color::BORDE, false) +
                pinta(color::BORDE, MARCO_DER);

    std::vector<std::string> lineas;
    lineas.reserve(filas.size() + 2);
    lineas.push_back("  " + panel[0]);
    for (size_t i = 0; i < filas.size(); ++i)
        lineas.push_back(filas[i] + "  " + panel[i + 1]);
    lineas.push_back(pinta(color::BORDE, " ENTER repetir   Q menu"));
    pintarBloqueCentrado(lineas); // mismo centrado que la partida
    cerrarFrame();
}

void Renderizador::pintarMenu(int seleccion, int record) {
    constexpr int ANCHO_CAJA   = 36; // incluye los dos bordes verticales
    constexpr int ANCHO_NOMBRE = 24; // campo de texto de cada opcion

    abrirFrame();

    // El bloque del menu se compone primero y se emite despues, para poder
    // centrarlo tambien en vertical (antes solo estaba centrado en columnas).
    std::vector<std::string> lineas;

    lineas.push_back("");

    // --- Titulo TETRIS en bloques, una letra por color de pieza. ---
    for (int f = 0; f < 5; ++f) {
        std::string fila = sangrar(ANCHO_TITULO, anchoPantalla_);
        for (int l = 0; l < 6; ++l) {
            if (l > 0) fila += ' ';
            fila += BORDE_INI + pinta(TITULO[l].color, TITULO[l].filas[f]) +
                    ANSI_RESET;
        }
        lineas.push_back(fila);
    }

    lineas.push_back("");
    lineas.push_back(centradoPintado("Edicion de terminal ASCII   10 x 20",
                                     anchoPantalla_, color::BORDE, false));
    lineas.push_back("");

    // --- Caja de opciones centrada. ---
    const std::string marco =
        std::string("+") + std::string(ANCHO_CAJA - 2, '-') + "+";
    lineas.push_back(sangrar(ANCHO_CAJA, anchoPantalla_) +
                     pinta(color::BORDE, BORDE_INI + marco + ANSI_RESET));

    const char* opciones[] = { "JUGAR", "COMO SE JUEGA", "AJUSTES", "SALIR" };
    const int   colores[]  = { color::S, color::J, color::O, color::Z };
    for (int i = 0; i < 4; ++i) {
        const bool activa = (i == seleccion);

        std::string nombre(ANCHO_NOMBRE, ' ');
        nombre.replace(0, std::strlen(opciones[i]), opciones[i]);

        std::string interna = std::string(4, ' ');
        interna += activa ? pinta(colores[i], ">") : " ";
        interna += " ";
        if (activa)
            interna += "\x1b[7m" + BORDE_INI + pinta(colores[i], nombre) +
                       ANSI_RESET;
        else
            interna += pinta(color::BORDE, nombre);
        interna += std::string(ANCHO_CAJA - 2 - 6 - ANCHO_NOMBRE, ' ');

        lineas.push_back(sangrar(ANCHO_CAJA, anchoPantalla_) +
                         pinta(color::BORDE, "|") + interna +
                         pinta(color::BORDE, "|"));
    }

    lineas.push_back(sangrar(ANCHO_CAJA, anchoPantalla_) +
                     pinta(color::BORDE, BORDE_INI + marco + ANSI_RESET));

    lineas.push_back("");
    lineas.push_back(centradoPintado("RECORD: " + std::to_string(record),
                                     anchoPantalla_, color::O, true));
    lineas.push_back("");
    lineas.push_back(centradoPintado("Flechas / W S navegar   ENTER confirmar",
                                     anchoPantalla_, color::BORDE, false));
    lineas.push_back(centradoPintado("Q salir     1 / 2 / 3 / 4 elegir opcion",
                                     anchoPantalla_, color::BORDE, false));

    // Filas en blanco arriba para el centrado vertical (su \x1b[K limpia la
    // pantalla anterior); el \x1b[J de cerrarFrame borra lo que sobre abajo.
    const int arriba = std::max(0,
        (altoPantalla_ - static_cast<int>(lineas.size())) / 2);
    for (int f = 0; f < arriba; ++f) escribirLinea("");
    for (const auto& l : lineas) escribirLinea(l);
    cerrarFrame();
}

void Renderizador::pintarAyuda() {
    abrirFrame();
    escribirLinea("");
    escribirLinea("  " + BORDE_INI + pinta(color::T, "COMO SE JUEGA") + BORDE_FIN);
    escribirLinea("");
    escribirLinea("  Encaja los tetraminos que caen y completa");
    escribirLinea("  filas horizontales enteras para eliminarlas.");
    escribirLinea("  Cada 10 filas eliminas subes de nivel y la");
    escribirLinea("  caida se acelera. La partida termina cuando");
    escribirLinea("  el pozo alcanza la parte superior.");
    escribirLinea("");
    escribirLinea("  " + pinta(color::BORDE, "CONTROLES"));
    escribirLinea("    A / D    mover izquierda / derecha");
    escribirLinea("    W        rotar (horario)");
    escribirLinea("    X        rotar (antihorario)");
    escribirLinea("    S        soft drop: bajar +rapido (+1 pto)");
    escribirLinea("    P        pausa        R    reiniciar");
    escribirLinea("    Q/ESC    volver al menu");
    escribirLinea("");
    escribirLinea("  " + pinta(color::BORDE, "PUNTUACION (x nivel actual)"));
    escribirLinea("    1 linea: 100    2 lineas: 300");
    escribirLinea("    3 lineas: 500   TETRIS (4): 800");
    escribirLinea("");
    escribirLinea("  " + pinta(color::S, "ENTER / Q: volver al menu"));
    cerrarFrame();
}

// ============================================================================
//  Pantalla de ajustes: paleta de color y reset de puntuacion
// ============================================================================

void Renderizador::pintarAjustes(int seleccion, int record, Paleta paleta,
                                 const std::vector<int>& partidas,
                                 bool esperandoConfirmar) {
    constexpr int ANCHO_CAJA   = 40; // incluye los dos bordes verticales
    constexpr int ANCHO_NOMBRE = 24; // campo de texto de cada opcion

    abrirFrame();

    std::vector<std::string> lineas;
    lineas.push_back("");
    lineas.push_back(sangrar(ANCHO_CAJA, anchoPantalla_) +
                     centradoPintado("A J U S T E S", ANCHO_CAJA, color::TEXTO, true));
    lineas.push_back("");

    const std::string marco =
        std::string("+") + std::string(ANCHO_CAJA - 2, '-') + "+";
    lineas.push_back(sangrar(ANCHO_CAJA, anchoPantalla_) +
                     pinta(color::BORDE, BORDE_INI + marco + ANSI_RESET));

    // Muestras de cada paleta pintadas con su color literal: asi se ven
    // ambas opciones tal cual, sea cual sea la paleta activa ahora.
    const std::string muestras[] = {
        ansi(color::S) + "[][]" + ANSI_RESET,   // CLASICA: verde propio
        VERDE_MATRIZ + "[][]" + ANSI_RESET,     // MATRIX: fosforo unico
    };
    const char* nombres[]  = { "PALETA CLASICA", "PALETA MATRIX VERDE",
                               "RESET PUNTUACION" };
    const int   colores[]  = { color::S, color::TEXTO, color::Z };

    for (int i = 0; i < 3; ++i) {
        const bool activa  = (i == seleccion);
        const bool marcada = (i == 0 && paleta == Paleta::CLASICA) ||
                             (i == 1 && paleta == Paleta::MATRIX);

        std::string nombre(ANCHO_NOMBRE, ' ');
        nombre.replace(0, std::strlen(nombres[i]), nombres[i]);

        std::string interna = " ";
        interna += (i < 2) ? pinta(marcada ? color::O : color::BORDE,
                                   marcada ? "[X]" : "[ ]")
                           : std::string(3, ' ');
        interna += " ";
        interna += activa ? pinta(colores[i], ">") : " ";
        interna += " ";
        if (activa)
            interna += "\x1b[7m" + BORDE_INI + pinta(colores[i], nombre) +
                       ANSI_RESET;
        else
            interna += pinta(color::BORDE, nombre);
        interna += " ";
        interna += (i < 2) ? muestras[i] : std::string(4, ' ');
        // Relleno hasta completar el interior de la caja (ANCHO_CAJA - 2).
        constexpr int ANCHO_INTERNA = 7 + ANCHO_NOMBRE + 1 + 4;
        interna += std::string(ANCHO_CAJA - 2 - ANCHO_INTERNA, ' ');

        lineas.push_back(sangrar(ANCHO_CAJA, anchoPantalla_) +
                         pinta(color::BORDE, "|") + interna +
                         pinta(color::BORDE, "|"));
    }

    lineas.push_back(sangrar(ANCHO_CAJA, anchoPantalla_) +
                     pinta(color::BORDE, BORDE_INI + marco + ANSI_RESET));

    lineas.push_back("");
    lineas.push_back(sangrar(ANCHO_CAJA, anchoPantalla_) +
                     centradoPintado("RECORD: " + std::to_string(record),
                                     ANCHO_CAJA, color::O, true));

    std::string ultimas = "ULTIMAS: ";
    if (partidas.empty()) ultimas += "(sin partidas)";
    for (size_t i = 0; i < partidas.size(); ++i) {
        if (i) ultimas += "  ";
        ultimas += std::to_string(partidas[i]);
    }
    lineas.push_back(sangrar(ANCHO_CAJA, anchoPantalla_) +
                     centradoPintado(ultimas, ANCHO_CAJA, color::BORDE, false));
    lineas.push_back("");

    if (esperandoConfirmar)
        lineas.push_back(sangrar(ANCHO_CAJA, anchoPantalla_) +
                         centradoPintado("ENTER de nuevo: BORRAR el record",
                                         ANCHO_CAJA, color::Z, true));
    else
        lineas.push_back(sangrar(ANCHO_CAJA, anchoPantalla_) +
                         centradoPintado("1 / 2 elegir paleta   ENTER confirmar",
                                         ANCHO_CAJA, color::BORDE, false));
    lineas.push_back(sangrar(ANCHO_CAJA, anchoPantalla_) +
                     centradoPintado("Flechas / W S navegar   Q volver",
                                     ANCHO_CAJA, color::BORDE, false));

    // Centrado vertical del bloque, igual que en el menu principal.
    const int arriba = std::max(0,
        (altoPantalla_ - static_cast<int>(lineas.size())) / 2);
    for (int f = 0; f < arriba; ++f) escribirLinea("");
    for (const auto& l : lineas) escribirLinea(l);
    cerrarFrame();
}

} // namespace juego
