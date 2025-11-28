#include <iostream>
#include <string>
#include <random>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <map>
#include <algorithm>
#include <ctime>        
#include <winsock2.h>   // Provee SOCKET, WSADATA, y la función Sleep()
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 5050 // Debe coincidir con el puerto del servidor

// ===================== FUNCIONES AUXILIARES =====================

/**
 * Obtiene la hora actual en formato YYYY-MM-DD HH:MM:SS
 */
std::string obtenerHoraActual() {
    auto ahora = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(ahora);
    
    tm *localTime = std::localtime(&t); 

    if (localTime == nullptr) {
        return "HORA_ERROR";
    }

    std::ostringstream oss;
    oss << std::put_time(localTime, "%Y-%m-%d %H:%M:%S"); 
    return oss.str();
}

/**
 * Genera una placa de 6 caracteres (ej: ABC123)
 */
std::string generarPlaca(std::mt19937& gen) {
    std::uniform_int_distribution<int> L(0, 25), D(0,9);
    std::string p;
    p.reserve(6);

    for(int i=0;i<3;i++) p.push_back('A'+L(gen));
    for(int i=0;i<3;i++) p.push_back('0'+D(gen));
    return p;
}


// ===================== MAIN CLIENTE (GENERADOR) =====================

int main(){

    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    
    // Inicializar Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        std::cerr << "Fallo al inicializar Winsock." << std::endl;
        return 1;
    }

    // Crear socket
    sock = socket(AF_INET, SOCK_STREAM, 0); 
    if(sock == INVALID_SOCKET){
        std::cerr << "Fallo en crear socket." << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Conectar
    if(connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR){
        std::cerr << "No se pudo conectar al servidor en el puerto " << PORT << ". Asegúrese que servidor.exe esté ejecutándose.\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "GENERADOR conectado al servidor.\n";

    // --- CONFIGURACIÓN GENERADOR ---
    std::random_device rd;
    std::mt19937 gen(rd());
    // Espera entre 2 y 5 segundos
    std::uniform_int_distribution<int> wait(2,5); 
    std::uniform_int_distribution<int> celdaDist(1,20); 

    // Mapa local: <Placa, Celda>
    std::map<std::string,int> ocupado; 

    while(true){
        
        // Usamos Sleep() de Windows (milisegundos)
        Sleep(wait(gen) * 1000); 

        std::string placa = generarPlaca(gen);
        std::string hora  = obtenerHoraActual();
        int celda;
        std::string evento;
        
        // Si la placa ya existe -> SALIDA. Si no -> ENTRADA.
        bool esSalida = ocupado.count(placa); 

        if(!esSalida){
            // --- Lógica de ENTRADA ---
            
            // 1. Verificar si el parqueadero está lleno
            if (ocupado.size() >= 20) {
                // Si está lleno, forzamos una SALIDA
                if (!ocupado.empty()) {
                    auto it = ocupado.begin();
                    placa = it->first; 
                    esSalida = true; 
                    // Continuar al bloque 'if(esSalida)'
                } else {
                    std::cout << "ADVERTENCIA: Parqueadero lleno. Esperando...\n";
                    continue; 
                }
            } else { // <-- Este bloque ELSE estaba abierto
                // 2. Buscar una celda que NO esté ocupada
                bool celda_en_uso;
                do {
                    celda = celdaDist(gen); 
                    celda_en_uso = false;

                    // Itera sobre los VALORES (celdas) del mapa 'ocupado'
                    for (const auto& par : ocupado) {
                        if (par.second == celda) {
                            celda_en_uso = true;
                            break;
                        }
                    }
                } while (celda_en_uso); // Repetir si la celda generada ya está en uso.

                // 3. Registrar la nueva ocupación localmente y enviar
                ocupado[placa] = celda;
                evento = placa + ";" + hora + ";" + std::to_string(celda) + ";ENTRADA";
                std::cout << "ENTRADA: Placa " << placa << " asignada a Celda " << celda << std::endl;
            } // <-- ¡Brace de cierre insertado aquí!
        } 
        
        if(esSalida) {
            // --- Lógica de SALIDA ---
            
            if (ocupado.count(placa)) { 
                celda = ocupado[placa];
                ocupado.erase(placa); 
                evento = placa + ";" + hora + ";" + std::to_string(celda) + ";SALIDA";
                std::cout << "SALIDA: Placa " << placa << " liberando Celda " << celda << std::endl;
            } else {
                std::cout << "ADVERTENCIA: Placa " << placa << " no encontrada para SALIDA. Re-generando...\n";
                continue;
            }
        }

        // 4. Enviar el evento al servidor
        if (!evento.empty()) {
            send(sock, evento.c_str(), evento.length(), 0);
        }
    }

    // Limpieza y cierre
    closesocket(sock);
    WSACleanup();

    return 0;
}