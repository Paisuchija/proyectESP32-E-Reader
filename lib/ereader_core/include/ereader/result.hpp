// result.hpp — Códigos de error del núcleo.
//
// El núcleo NO usa excepciones (regla §6). Todas las operaciones que pueden
// fallar devuelven un Result. Es un enum class pequeño para que quepa en un
// registro y no obligue a asignar memoria.
#pragma once

#include <cstdint>

namespace ereader {

enum class Result : uint8_t {
    Ok = 0,          // Éxito.
    End,             // Fin de datos (no es un error; p. ej. fin de archivo o de libro).
    ErrIO,           // Error de lectura/escritura del medio.
    ErrNotFound,     // El recurso solicitado no existe.
    ErrCorrupt,      // Datos corruptos (CRC inválido, magic desconocido, ...).
    ErrNoSpace,      // No hay espacio libre suficiente.
    ErrInvalidArg,   // Argumento fuera de rango o incoherente.
    ErrCancelled,    // Operación cancelada mediante un token de cancelación.
    ErrUnsupported,  // Operación no soportada por esta implementación.
    ErrTimeout,      // Se agotó el tiempo de espera.
    ErrAgain,        // Aún no hay resultado; reintentar más tarde (no bloqueante).
};

// Devuelve true si el resultado representa éxito operativo.
inline bool isOk(Result r) { return r == Result::Ok; }

// Nombre legible del resultado, para diagnóstico por consola.
const char* resultName(Result r);

}  // namespace ereader
