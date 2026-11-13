import os
from storage import StorageManager
from network_client import NetworkClient

storage = StorageManager("/")
net = NetworkClient("", "") 

CATALOGO_ONLINE = [
    {
        "id": 1, 
        "titulo": "Don Quijote de la Mancha", 
        "autor": "Miguel de Cervantes", 
        "sinopsis": "Las divertidas aventuras de un hidalgo enloquecido por los libros de caballería que cree ser caballero andante.",
        "url": "https://www.gutenberg.org/cache/epub/2000/pg2000.txt"
    },
    {
        "id": 2, 
        "titulo": "La Odisea", 
        "autor": "Homero", 
        "sinopsis": "El épico viaje de regreso a casa de Odiseo (Ulises) tras la guerra de Troya, enfrentando monstruos y dioses.",
        "url": "https://www.gutenberg.org/cache/epub/1727/pg1727.txt"
    },
    {
        "id": 3, 
        "titulo": "Frankenstein", 
        "autor": "Mary Shelley", 
        "sinopsis": "Un joven científico da vida a una criatura monstruosa en un experimento que desencadenará trágicas consecuencias.",
        "url": "https://www.gutenberg.org/cache/epub/84/pg84.txt"
    },
    {
        "id": 4, 
        "titulo": "La Metamorfosis", 
        "autor": "Franz Kafka", 
        "sinopsis": "Gregor Samsa despierta una mañana convertido en un gigantesco insecto, alterando drásticamente la vida de su familia.",
        "url": "https://www.gutenberg.org/cache/epub/5200/pg5200.txt"
    },
    {
        "id": 5, 
        "titulo": "Drácula", 
        "autor": "Bram Stoker", 
        "sinopsis": "La clásica novela epistolar sobre el conde vampiro que viaja desde Transilvania a Inglaterra en busca de nuevas víctimas.",
        "url": "https://www.gutenberg.org/cache/epub/345/pg345.txt"
    },
    {
        "id": 6, 
        "titulo": "Crimen y Castigo", 
        "autor": "Fiódor Dostoyevski", 
        "sinopsis": "Un exestudiante pobre en San Petersburgo comete un asesinato mientras debate sus complejas teorías morales y psicológicas.",
        "url": "https://www.gutenberg.org/cache/epub/2554/pg2554.txt"
    },
    {
        "id": 7, 
        "titulo": "Orgullo y Prejuicio", 
        "autor": "Jane Austen", 
        "sinopsis": "La turbulenta relación entre Elizabeth Bennet y el apuesto y orgulloso señor Darcy en la Inglaterra del siglo XIX.",
        "url": "https://www.gutenberg.org/cache/epub/1342/pg1342.txt"
    },
    {
        "id": 8, 
        "titulo": "La Isla del Tesoro", 
        "autor": "Robert Louis Stevenson", 
        "sinopsis": "Aventuras de piratas, mapas ocultos y búsqueda de botines lideradas por el joven Jim Hawkins y el capitán Long John Silver.",
        "url": "https://www.gutenberg.org/cache/epub/120/pg120.txt"
    },
    {
        "id": 9, 
        "titulo": "El Retrato de Dorian Gray", 
        "autor": "Oscar Wilde", 
        "sinopsis": "Un joven vende su alma para que su retrato envejezca en su lugar mientras él mantiene eterna juventud y belleza.",
        "url": "https://www.gutenberg.org/cache/epub/4276/pg4276.txt"
    },
    {
        "id": 10, 
        "titulo": "La Divina Comedia", 
        "autor": "Dante Alighieri", 
        "sinopsis": "El viaje alegórico del poeta a través del Infierno, el Purgatorio y el Paraíso guiado por Virgilio y Beatriz.",
        "url": "https://www.gutenberg.org/cache/epub/1004/pg1004.txt"
    }
]

def buscar_archivo_flexible(nombre_parcial, lista_archivos):
    nombre_parcial = nombre_parcial.lower().strip()
    for archivo in lista_archivos:
        if nombre_parcial in archivo.lower() or archivo.lower().startswith(nombre_parcial):
            return archivo
    if not nombre_parcial.endswith(".txt"):
        posible = nombre_parcial + ".txt"
        if posible in [a.lower() for a in lista_archivos]:
            for archivo in lista_archivos:
                if archivo.lower() == posible:
                    return archivo
    return None

def mostrar_menu_principal():
    print("\n" + "="*45)
    print("      E-READER ESP32 - MENÚ INTELIGENTE      ")
    print("="*45)
    print("1. Biblioteca Local")
    print("2. Catálogo Online (10 Libros, Sinopsis y Streaming)")
    print("3. Conectar a red Wi-Fi")
    print("4. Salir")
    print("="*45)

def bucle_principal():
    while True:
        mostrar_menu_principal()
        opcion = input("Selecciona una opción (1-4): ").strip()
        
        if opcion == "1":
            libros = storage.listar_libros()
            print(f"\nLibros locales disponibles: {libros}")
            if libros:
                busqueda = input("Escribe el nombre o parte del libro que buscas: ").strip()
                libro_encontrado = buscar_archivo_flexible(busqueda, libros)
                
                if libro_encontrado:
                    print(f"Abriendo: {libro_encontrado}...")
                    paginas = storage.leer_libro_por_paginas(libro_encontrado, lineas_por_pagina=3)
                    idx = 0
                    while idx < len(paginas):
                        print(f"\n[Página {idx+1}/{len(paginas)}]")
                        for linea in paginas[idx]:
                            print(linea)
                        
                        accion = input("\n[N] Siguiente | [P] Anterior | [S] Volver al menú: ").strip().lower()
                        if accion == 'n' and idx < len(paginas) - 1:
                            idx += 1
                        elif accion == 'p' and idx > 0:
                            idx -= 1
                        elif accion == 's':
                            break
                else:
                    print("No se encontró ningún libro con ese nombre en local.")
            else:
                print("No hay libros locales.")
                
        elif opcion == "2":
            print("\n" + "-"*50)
            print("        CATÁLOGO ONLINE (10 RECOMENDADOS)       ")
            print("-"*50)
            for item in CATALOGO_ONLINE:
                print(f"[{item['id']}] {item['titulo']} - {item['autor']}")
            print("-" * 50)
            
            seleccion = input("Introduce el número del libro (1-10) o 0 para volver: ").strip()
            if seleccion.isdigit():
                sel_id = int(seleccion)
                if sel_id == 0:
                    continue
                
                libro_seleccionado = next((l for l in CATALOGO_ONLINE if l['id'] == sel_id), None)
                
                if libro_seleccionado:
                    print("\n" + "="*50)
                    print(f"📖 {libro_seleccionado['titulo'].upper()}")
                    print(f"Autor: {libro_seleccionado['autor']}")
                    print(f"Sinopsis: {libro_seleccionado['sinopsis']}")
                    print("="*50)
                    
                    modo = input("¿Deseas [L]eer en línea (streaming) o [D]escargar a local? [L/d]: ").strip().lower()
                    
                    if modo == '' or modo == 'l':
                        print(f"\nConectando para lectura en línea de '{libro_seleccionado['titulo']}'...")
                        tamano_fragmento = 1200
                        offset_actual = 0
                        
                        while True:
                            print(f"\n--- LEYENDO EN LÍNEA (Bytes {offset_actual} - {offset_actual + tamano_fragmento}) ---")
                            fragmento = net.leer_fragmento_streaming(libro_seleccionado['url'], offset_actual, offset_actual + tamano_fragmento)
                            
                            if fragmento:
                                print(fragmento)
                            else:
                                print("No se pudo obtener más contenido o fin del archivo.")
                                break
                            
                            accion = input("\n[N] Siguiente fragmento | [P] Fragmento anterior | [S] Volver al menú: ").strip().lower()
                            if accion == 'n':
                                offset_actual += tamano_fragmento
                            elif accion == 'p' and offset_actual >= tamano_fragmento:
                                offset_actual -= tamano_fragmento
                            elif accion == 's':
                                break
                    else:
                        nombre_archivo = f"{libro_seleccionado['titulo'].lower().replace(' ', '_')}.txt"
                        confirmar = input(f"¿Guardar como '{nombre_archivo}'? [S/n]: ").strip().lower()
                        if confirmar == '' or confirmar == 's':
                            print("\nDescargando archivo por bloques...")
                            net.descargar_archivo_por_bloques(libro_seleccionado['url'], nombre_archivo)
                        else:
                            print("Descarga cancelada.")
                else:
                    print("Número de libro no válido.")
                    
        elif opcion == "3":
            ssid = input("Introduce tu red Wi-Fi (SSID): ").strip()
            password = input("Introduce la contraseña: ").strip()
            net.ssid = ssid
            net.password = password
            net.conectar()
            
        elif opcion == "4":
            print("Apagando e-reader... ¡Hasta pronto!")
            break
        else:
            print("Opción no válida.")

if __name__ == "__main__":
    bucle_principal()