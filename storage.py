import os

class StorageManager:
    def __init__(self, base_path="/"):
        self.base_path = base_path
        if self.base_path != "/" and self.base_path not in os.listdir("/"):
            try:
                os.mkdir(self.base_path)
            except OSError:
                pass

    def listar_libros(self):
        """Devuelve una lista con todos los archivos .txt disponibles."""
        try:
            archivos = os.listdir(self.base_path)
            libros = [f for f in archivos if f.endswith(".txt")]
            return libros
        except Exception as e:
            print("Error al listar archivos:", e)
            return []

    def leer_libro_por_paginas(self, nombre_archivo, lineas_por_pagina=5):
        """Lee un archivo local y lo divide en una lista de páginas (listas de líneas)."""
        ruta_completa = self.base_path + nombre_archivo if self.base_path.endswith("/") else self.base_path + "/" + nombre_archivo
        
        try:
            with open(ruta_completa, "r", encoding="utf-8") as f:
                lineas = [linea.rstrip() for linea in f.readlines()]
            
            paginas = []
            for i in range(0, len(lineas), lineas_por_pagina):
                paginas.append(lineas[i:i + lineas_por_pagina])
                
            return paginas
        except OSError:
            print(f"No se pudo abrir el archivo: {nombre_archivo}")
            return [["Error: No se pudo cargar el archivo."]]

    def guardar_archivo_local(self, nombre_archivo, contenido_texto):
        """Guarda texto recibido en el almacenamiento."""
        ruta_completa = self.base_path + nombre_archivo if self.base_path.endswith("/") else self.base_path + "/" + nombre_archivo
        try:
            with open(ruta_completa, "w", encoding="utf-8") as f:
                f.write(contenido_texto)
            print(f"Archivo '{nombre_archivo}' guardado con éxito.")
            return True
        except Exception as e:
            print("Error al guardar el archivo:", e)
            return False