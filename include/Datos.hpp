#ifndef DATOS_HPP
#define DATOS_HPP

#include <string>
#include <vector>

// ============================================================================
//  Datos
//  Persistencia en JSON del juego, SIN dependencias externas: serializa y
//  relaja un unico objeto con las claves "record", "partidas" (ultimas 5,
//  la mas reciente primero) y "paleta". El archivo lo crea y gestiona el
//  propio programa. Logica pura: solo <fstream>, nada de terminal.
// ============================================================================

namespace juego {

struct Guardado {
    int              record   = 0;
    int              paleta   = 1;   // 1 = CLASICA, 2 = MATRIX
    std::vector<int> partidas;       // historial, maximo MAX_PARTIDAS_GUARDADAS
};

// JSON legible para humanos.
std::string serializarGuardado(const Guardado& g);

// Parseo tolerante: lo que no reconoce conserva el valor por defecto.
Guardado parsearGuardado(const std::string& json);

// Lee el JSON de `ruta`. Si no existe, migra el record del antiguo archivo
// de texto plano `rutaLegado` (un entero suelto).
Guardado cargarGuardado(const std::string& ruta, const std::string& rutaLegado);

// Escribe el JSON completo. Devuelve true si todo fue bien.
bool guardarGuardado(const std::string& ruta, const Guardado& g);

} // namespace juego

#endif // DATOS_HPP
