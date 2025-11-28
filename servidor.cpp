#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <fstream>      // NUEVO: Para guardar el estado en un archivo
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib") // Enlaza la librería Winsock
#define PORT 5050


// Estructura para almacenar los datos de cada evento de placa
struct Evento {
    std::string placa;
    std::string hora;
    int celda;
    std::string tipo; // "ENTRADA" o "SALIDA"
};

// ===================== FUNCIONES DE ESTADO =====================

/**
 * Guarda el estado actual del mapa de parqueadero en el archivo
 * estado_parqueadero.txt para que la DLL lo pueda leer.
 */
void guardar_estado(const std::map<int, std::string>& parqueadero) {
    std::ofstream outfile("estado_parqueadero.txt");
    if (outfile.is_open()) {
        for (const auto& pair : parqueadero) {
            // Formato: CELDA,PLACA (necesario para el visualizador)
            outfile << pair.first << "," << pair.second << "\n";
        }
        outfile.close();
    }
}


// ===================== FUNCIONES DE PARSEO =====================

/**
 * Parsea el mensaje de texto recibido del cliente (Generador)
 * en un struct Evento. Formato esperado: PLACA;HORA;CELDA;TIPO
 */
Evento parsear(const std::string& msg){
    Evento e;
    std::stringstream ss(msg);
    std::string parte;

    // Leer los campos separados por ';'
    getline(ss, e.placa, ';');
    getline(ss, e.hora,  ';');
    getline(ss, parte,   ';');
    // Convertir la celda a entero
    try {
        e.celda = std::stoi(parte);
    } catch (...) {
        e.celda = -1; // Marcar como inválida si falla la conversión
    }
    getline(ss, e.tipo,  ';');

    return e;
}


// ===================== MAIN SERVER =====================

int main(){

    WSADATA wsaData;
    SOCKET server_fd, socket_cli;
    sockaddr_in direccion{};
    int addrlen = sizeof(direccion);
    char buffer[1024];

    // Inicializar Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        std::cerr << "Fallo al inicializar Winsock." << std::endl;
        return 1;
    }

    // Crear socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "Fallo en crear socket." << std::endl;
        WSACleanup();
        return 1;
    }

    // Configurar dirección
    direccion.sin_family = AF_INET;
    direccion.sin_port   = htons(PORT);
    direccion.sin_addr.s_addr = INADDR_ANY;

    // Bind (Asociar)
    if (bind(server_fd, (sockaddr*)&direccion, sizeof(direccion)) == SOCKET_ERROR) {
        std::cerr << "Fallo en bind. Verifique si el puerto está libre." << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // Listen (Escuchar)
    if (listen(server_fd, 3) == SOCKET_ERROR) {
        std::cerr << "Fallo en listen." << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    std::cout<< "SERVIDOR esperando conexión en puerto " << PORT << "...\n";

    // Accept (Aceptar conexión)
    socket_cli = accept(server_fd, (sockaddr*)&direccion, &addrlen);
    if (socket_cli == INVALID_SOCKET) {
        std::cerr << "Fallo en accept." << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    std::cout<< "Cliente conectado!\n";

    // Mapa para almacenar el estado actual del parqueadero: <Celda, Placa>
    std::map<int,std::string> parqueadero; 

    // Bucle principal de recepción de eventos
    while(true){
        int len = recv(socket_cli, buffer, 1024, 0);

        if(len <= 0){
            if (len == 0) {
                std::cout << "Cliente desconectado. Servidor cerrando.\n";
            } else {
                std::cerr << "Error de recepción.\n";
            }
            break; 
        }

        // Asegurarse de que el buffer es una cadena nula-terminada
        buffer[len] = '\0';
        std::string mensaje_placa(buffer);

        // 1. Parsear el evento
        Evento e = parsear(mensaje_placa);
        
        std::cout << "\n-------------------------------------\n";
        std::cout << "RECIBIDO: " << e.placa << " (" << e.hora << ") - Celda: " << e.celda << " - Tipo: " << e.tipo << "\n";

        // 2. Lógica de ENTRADA/SALIDA y gestión del estado
        if(e.tipo == "ENTRADA"){
            // El generador.cpp asegura que la placa es nueva y la celda está libre.
            if (parqueadero.count(e.celda)) {
                std::cout << "  -> ADVERTENCIA: Celda " << e.celda << " ya ocupada. Evento de ENTRADA ignorado.\n";
            } else {
                // ENTRADA normal: Ocupa la celda
                parqueadero[e.celda] = e.placa;
                std::cout << "  -> ENTRADA REGISTRADA. Celda " << e.celda << " asignada a " << e.placa << ".\n";
            }

        } else if (e.tipo == "SALIDA") {
            // El generador.cpp envía SALIDA de la celda que tenía esa placa.
            if (parqueadero.count(e.celda) && parqueadero[e.celda] == e.placa) {
                parqueadero.erase(e.celda);
                std::cout << "  -> SALIDA REGISTRADA. Celda " << e.celda << " liberada.\n";
            } else {
                std::cout << "  -> ADVERTENCIA: Placa " << e.placa << " intentó SALIR de celda " << e.celda << " que no ocupaba o ya estaba libre.\n";
            }
        } else {
             std::cout << "  -> ADVERTENCIA: Tipo de evento desconocido: " << e.tipo << "\n";
        }


        // 3. GUARDAR EL ESTADO (Este es el cambio clave para el Visualizador)
        guardar_estado(parqueadero);
        
        // 4. Imprimir estado actual en consola del servidor (opcional, para debug)
        std::cout << "\n--- ESTADO PARQUEADERO (Celda: Placa) ---\n";
        if (parqueadero.empty()) {
            std::cout << "Parqueadero vacío.\n";
        } else {
            for(const auto& par : parqueadero){
                std::cout << "Celda " << par.first << ": " << par.second << "\n";
            }
        }
        std::cout << "-------------------------------------\n";
    }

    // 5. Limpieza y cierre
    closesocket(socket_cli);
    closesocket(server_fd);
    WSACleanup();

    return 0;
}