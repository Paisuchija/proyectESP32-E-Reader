import network
import urequests
import time

class NetworkClient:
    def __init__(self, ssid, password):
        self.ssid = ssid
        self.password = password
        self.wlan = network.WLAN(network.STA_IF)

    def conectar(self):
        """Conecta el ESP32 a la red Wi-Fi de forma segura."""
        try:
            self.wlan.active(False)
            time.sleep(0.5)
            self.wlan.active(True)
            time.sleep(0.5)
            
            if not self.wlan.isconnected():
                print(f"Conectando a la red Wi-Fi '{self.ssid}'...")
                self.wlan.connect(self.ssid, self.password)
                
                intentos = 0
                while not self.wlan.isconnected() and intentos < 30:
                    time.sleep(0.5)
                    intentos += 1
                    
            if self.wlan.isconnected():
                print("¡Conectado con éxito! Configuración IP:", self.wlan.ifconfig()[0])
                return True
            else:
                print("Error: Tiempo de espera agotado. Verifica tu contraseña.")
                return False
        except Exception as e:
            print("Excepción al conectar al Wi-Fi:", e)
            return False

    def desconectar(self):
        if self.wlan.isconnected():
            self.wlan.disconnect()
            self.wlan.active(False)
            print("Wi-Fi desconectado y desactivado.")

    def descargar_archivo_por_bloques(self, url, ruta_destino, tamano_bloque=512):
        """Descarga archivos grandes por bloques para evitar desbordar la RAM."""
        try:
            print(f"Iniciando descarga en streaming...")
            respuesta = urequests.get(url, stream=True)
            if respuesta.status_code == 200:
                with open(ruta_destino, "w", encoding="utf-8") as f:
                    while True:
                        chunk = respuesta.raw.read(tamano_bloque)
                        if not chunk:
                            break
                        f.write(chunk.decode('utf-8', 'ignore'))
                respuesta.close()
                print(f"¡Descarga completada y guardada como '{ruta_destino}'!")
                return True
            else:
                print(f"Error HTTP al descargar: {respuesta.status_code}")
                respuesta.close()
                return False
        except Exception as e:
            print("Excepción durante la descarga por bloques:", e)
            return False

    def leer_fragmento_streaming(self, url, inicio_byte, fin_byte):
        """Solicita un rango específico de bytes en línea sin descargar todo el libro."""
        headers = {"Range": f"bytes={inicio_byte}-{fin_byte}"}
        try:
            respuesta = urequests.get(url, headers=headers)
            if respuesta.status_code in [200, 206]:
                contenido = respuesta.text
                respuesta.close()
                return contenido
            else:
                print(f"Error HTTP en streaming: {respuesta.status_code}")
                respuesta.close()
                return None
        except Exception as e:
            print("Excepción durante el streaming en línea:", e)
            return None