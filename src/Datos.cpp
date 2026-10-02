#include "Datos.hpp"

#include <cctype>
#include <fstream>

#include "constantes.hpp"

namespace juego {

namespace {

// Lee un entero optativo-signo a partir de `desde`, sin pasar de `limite`.
// Devuelve false si no hay digitos; `fin` queda tras el numero leido.
bool extraerEntero(const std::string& s, size_t desde, size_t limite,
                   size_t& fin, long& valor) {
    size_t i = desde;
    while (i < limite && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    const bool negativo = (i < limite && s[i] == '-');
    if (negativo) ++i;

    const size_t inicio = i;
    long v = 0;
    while (i < limite && std::isdigit(static_cast<unsigned char>(s[i]))) {
        v = v * 10 + (s[i] - '0');
        ++i;
    }
    if (i == inicio) return false; // sin digitos
    fin   = i;
    valor = negativo ? -v : v;
    return true;
}

// Busca `"clave"` y lee el entero que sigue a sus dos puntos.
bool buscarEntero(const std::string& s, const std::string& clave, long& valor) {
    size_t p = s.find(clave);
    if (p == std::string::npos) return false;
    p = s.find(':', p + clave.size());
    if (p == std::string::npos) return false;
    size_t fin;
    return extraerEntero(s, p + 1, s.size(), fin, valor);
}

// Busca `"clave": [ ... ]` y devuelve los enteros del array.
std::vector<int> buscarArray(const std::string& s, const std::string& clave) {
    std::vector<int> salida;
    size_t p = s.find(clave);
    if (p == std::string::npos) return salida;
    const size_t apertura = s.find('[', p);
    const size_t cierre   = s.find(']', apertura == std::string::npos ? 0 : apertura);
    if (apertura == std::string::npos || cierre == std::string::npos) return salida;

    size_t i = apertura + 1;
    while (i < cierre) {
        long n;
        size_t fin;
        if (extraerEntero(s, i, cierre, fin, n)) {
            salida.push_back(static_cast<int>(n));
            i = fin;
        } else {
            ++i; // comas, espacios o basura: seguir buscando
        }
    }
    return salida;
}

std::string unirs(const std::vector<int>& v) {
    std::string s;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) s += ", ";
        s += std::to_string(v[i]);
    }
    return s;
}

} // namespace

std::string serializarGuardado(const Guardado& g) {
    return std::string("{\n") +
           "  \"record\": "   + std::to_string(g.record)   + ",\n" +
           "  \"partidas\": [" + unirs(g.partidas)         + "],\n" +
           "  \"paleta\": "   + std::to_string(g.paleta)   + "\n" +
           "}\n";
}

Guardado parsearGuardado(const std::string& json) {
    Guardado g;
    long v = 0;

    if (buscarEntero(json, "\"record\"", v) && v > 0) g.record = static_cast<int>(v);

    if (buscarEntero(json, "\"paleta\"", v) &&
        v >= static_cast<long>(Paleta::CLASICA) && v <= static_cast<long>(Paleta::MATRIX))
        g.paleta = static_cast<int>(v);

    for (int n : buscarArray(json, "\"partidas\"")) {
        if (n < 0) continue;
        g.partidas.push_back(n);
        if (static_cast<int>(g.partidas.size()) >= MAX_PARTIDAS_GUARDADAS) break;
    }
    return g;
}

Guardado cargarGuardado(const std::string& ruta, const std::string& rutaLegado) {
    Guardado g;

    std::ifstream in(ruta);
    if (in) {
        const std::string json((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
        return parsearGuardado(json);
    }

    // Sin JSON aun: migrar el record del antiguo archivo de texto plano.
    std::ifstream viejo(rutaLegado);
    if (viejo) {
        long v = 0;
        if ((viejo >> v) && v > 0) g.record = static_cast<int>(v);
    }
    return g;
}

bool guardarGuardado(const std::string& ruta, const Guardado& original) {
    Guardado g = original; // normalizamos antes de escribir
    if (g.record < 0) g.record = 0;
    if (static_cast<int>(g.partidas.size()) > MAX_PARTIDAS_GUARDADAS)
        g.partidas.resize(MAX_PARTIDAS_GUARDADAS);

    std::ofstream out(ruta, std::ios::trunc);
    if (!out) return false;
    out << serializarGuardado(g);
    return static_cast<bool>(out);
}

} // namespace juego
