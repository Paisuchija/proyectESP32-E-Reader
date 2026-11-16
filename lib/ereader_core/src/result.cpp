// result.cpp — Nombres legibles de Result para diagnóstico.
#include "ereader/result.hpp"

namespace ereader {

const char* resultName(Result r) {
    switch (r) {
        case Result::Ok:            return "Ok";
        case Result::End:           return "End";
        case Result::ErrIO:         return "ErrIO";
        case Result::ErrNotFound:   return "ErrNotFound";
        case Result::ErrCorrupt:    return "ErrCorrupt";
        case Result::ErrNoSpace:    return "ErrNoSpace";
        case Result::ErrInvalidArg: return "ErrInvalidArg";
        case Result::ErrCancelled:  return "ErrCancelled";
        case Result::ErrUnsupported:return "ErrUnsupported";
        case Result::ErrTimeout:    return "ErrTimeout";
        case Result::ErrAgain:      return "ErrAgain";
    }
    return "Unknown";
}

}  // namespace ereader
