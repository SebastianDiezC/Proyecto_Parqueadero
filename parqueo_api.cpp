#define PARQUEO_API_EXPORTS // Define esto para exportar funciones
#include "parqueo_api.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <iostream>

#define FILENAME "estado_parqueadero.txt"

// Implementación de la función exportada
PARQUEO_API int obtener_estado(char* buffer, int buffer_size) {
    std::ifstream file(FILENAME);
    if (!file.is_open()) {
        if (buffer_size > 0) buffer[0] = '\0';
        return 0; 
    }

    std::stringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();
    file.close();

    // Copiar el contenido al buffer
    int len = content.length();
    
    // Asegurarse de no desbordar el buffer
    if (len >= buffer_size) {
        len = buffer_size - 1;
    }
    
    if (buffer_size > 0) {
        strncpy(buffer, content.c_str(), len);
        buffer[len] = '\0'; // Asegurar terminación nula
    }

    return len;
}

