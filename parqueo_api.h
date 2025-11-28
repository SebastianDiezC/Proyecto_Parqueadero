#ifndef PARQUEO_API_H
#define PARQUEO_API_H

#ifdef PARQUEO_API_EXPORTS
#define PARQUEO_API __declspec(dllexport)
#else
#define PARQUEO_API __declspec(dllimport)
#endif

// Asegura que la función sea accesible desde Python/Java (manejo de nombres)
extern "C" {
    /**
     * Obtiene el estado actual del parqueadero leyendo el archivo de estado.
     * * @param buffer Puntero al buffer de caracteres donde se escribirá el estado (CELDA,PLACA\n...).
     * @param buffer_size Tamaño máximo del buffer.
     * @return int Número de bytes escritos en el buffer. 0 si hay error.
     */
    PARQUEO_API int obtener_estado(char* buffer, int buffer_size);
}

#endif // PARQUEO_API_H

