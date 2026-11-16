class VirtualKeyboard:
    def __init__(self, pantalla_ancho=320, pantalla_alto=240):
        self.ancho = pantalla_ancho
        self.alto = pantalla_alto
        self.texto_actual = ""
        
        self.filas_teclas = [
            ["Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"],
            ["A", "S", "D", "F", "G", "H", "J", "K", "L", "<-"],
            ["Z", "X", "C", "V", "B", "N", "M", " ", "OK"]
        ]

    def resetear_texto(self):
        self.texto_actual = ""

    def obtener_texto(self):
        return self.texto_actual

    def procesar_toque(self, x, y):
        print(f"Toque registrado en coordenadas X:{x}, Y:{y}")
        return self.texto_actual

    def simular_escritura(self, texto):
        self.texto_actual += texto
        return self.texto_actual