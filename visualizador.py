import ctypes
import time
import os

# --- Visualizador de Parqueadero (Python) ---

DLL_NAME = "parqueo_api.dll"

def mostrar_estado_parqueadero():
    # Carga la librería dinámica (DLL)
    try:
        # Busca la DLL en el directorio actual
        parqueo_dll = ctypes.CDLL(os.path.join(os.path.dirname(os.path.abspath(__file__)), DLL_NAME))

        # Define el prototipo de la función C++: int obtener_estado(char* buffer, int buffer_size);
        parqueo_dll.obtener_estado.argtypes = [ctypes.c_char_p, ctypes.c_int]
        parqueo_dll.obtener_estado.restype = ctypes.c_int

    except FileNotFoundError:
        print(f"Error: No se encontró '{DLL_NAME}'. Asegúrate de compilar la librería C++ primero.")
        return

    # Asignar un buffer grande para recibir la información
    BUFFER_SIZE = 4096
    buffer = ctypes.create_string_buffer(BUFFER_SIZE)

    # Llamar a la función de la DLL
    bytes_leidos = parqueo_dll.obtener_estado(buffer, BUFFER_SIZE)

    if bytes_leidos > 0:
        # Decodificar el contenido del buffer
        estado_raw = buffer.value.decode('utf-8')
        
        # Procesar el estado (formato: CELDA,PLACA\nCELDA,PLACA...)
        lineas = estado_raw.strip().split('\n')
        
        estado = {}
        for linea in lineas:
            if ',' in linea:
                try:
                    celda, placa = linea.split(',')
                    estado[int(celda.strip())] = placa.strip()
                except ValueError:
                    continue # Ignorar línea mal formada

        # --- Visualización ---
        print("\n" + "="*40)
        print("  ESTADO ACTUAL DEL PARQUEADERO")
        print("="*40)
        
        max_celda = 20 # Basado en la lógica del generador.cpp
        
        if not estado:
            print("Parqueadero Vacío.")
            print("-" * 40)
        else:
            celdas_ocupadas = len(estado)
            print(f"Total Ocupadas: {celdas_ocupadas:02d} / {max_celda}")
            print("-" * 40)
            
            # Formato de visualización de todas las celdas
            for i in range(1, max_celda + 1):
                placa = estado.get(i)
                if placa:
                    status = f"OCUPADA -> Placa: {placa}"
                    print(f"\033[92mCelda {i:02d}: {status}\033[0m") # Verde para ocupado
                else:
                    print(f"Celda {i:02d}: LIBRE")

    elif bytes_leidos == 0 and not os.path.exists("estado_parqueadero.txt"):
        print("Esperando a que el servidor cree el archivo de estado...")
        print("Asegúrate de ejecutar el servidor (servidor.exe) y el generador (generador.exe).")
    else:
        print("No se pudo obtener el estado del parqueadero. Estado vacío.")


if __name__ == "__main__":
    print("Iniciando Visualizador. Presiona Ctrl+C para detener.")
    while True:
        try:
            mostrar_estado_parqueadero()
            time.sleep(1) # Actualizar cada 1 segundo
        except KeyboardInterrupt:
            print("\nVisualizador detenido.")
            break
        except Exception as e:
            print(f"Error en el visualizador: {e}")
            time.sleep(1)