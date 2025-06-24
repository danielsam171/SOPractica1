# Sistema de Búsqueda de Préstamos de la Biblioteca Pública de Seattle

Este documento describe el sistema de búsqueda de préstamos de la Biblioteca Pública de Seattle, incluyendo la descripción del dataset utilizado, los campos clave, los criterios de búsqueda implementados, la justificación de estos criterios y ejemplos de uso del programa.

## 1. Descripción de la Base de Datos

Este conjunto de datos contiene un registro de todos los préstamos físicos de ítems realizados en la Biblioteca Pública de Seattle, desde abril de 2005 hasta septiembre de 2017. No incluye renovaciones. Su descarga gratuita está disponible al público en: https://www.kaggle.com/datasets/seattle-public-library/seattle-library-checkout-records?select=Checkouts_By_Title_Data_Lens_2012.csv.

## 2. Campos del Dataset Seleccionado

A continuación, se presenta una descripción detallada de los campos del dataset proporcionado, incluyendo los rangos de entrada válidos para cada uno.

* **BibNumber** 
    * Descripción: Identificador único del registro bibliográfico (título o ítem en el catálogo).
    * Tipo: Numérico.
    * Ejemplo: 2700635.

* **ItemBarcode** 
    * Descripción: Código de barras único asignado a cada ejemplar físico.
    * Tipo: Cadena numérica.
    * Ejemplo: 0010072332389.

* **ItemType** 
    * Descripción: Tipo de ítem prestado (libro para adultos, DVD, audiolibro, etc.).
    * Tipo: cadena de 3 a 5 letras.
    * Ejemplo: acbk.

* **Collection** 
    * Descripción: Sección o colección de la biblioteca a la que pertenece el ítem (CA9-Geneal. Bibliographies, CS 8 - NW Arts, etc.).
    * Tipo: Código de texto, generalmente de 4 a 6 caracteres.
    * Ejemplo: namys.

* **CallNumber** 
    * Descripción: Número de clasificación del ítem, usado para localizarlo físicamente en la biblioteca.
    * Tipo: Cadena alfanumérica.
    * Ejemplo: 133.32424 M7813T 2010.

* **CheckoutDateTime** 
    * Descripción: Fecha y hora exactas en que el ítem fue prestado.
    * Tipo: Fecha y hora en formato MM/DD/YYYY hh:mm:ss AM/PM.
    * Ejemplo: 04/04/2011 10:17:00 AM.

## 3. Criterios de Búsqueda Implementados

Los criterios de búsqueda implementados son los siguientes:

* Tipo de búsqueda (descrita en el menú): “Buscar por ID y año” o “Buscar ID en todos los años”.
* Año de búsqueda.
* ID del recurso físico: ItemBarcode.

### Justificación de los Criterios 

El **tipo de búsqueda** es un campo que permite flexibilidad en el uso de la interfaz, ya que habilita al sistema para atender tanto a necesidades de consulta precisas como a exploraciones más amplias. Los dos tipos de consultas son:

* **Consultas Específicas**: permite obtener los registros de un año en particular, permitiendo al sistema reducir el espacio de búsqueda, mejorando significativamente el rendimiento y la velocidad de las consultas para ítems específicos dentro de ese periodo.
* **Búsquedas Globales (a través del tiempo)**: los usuarios pueden optar por no especificar un año para realizar búsquedas de un ítem a lo largo de toda la línea de tiempo del dataset. Útil para encontrar un ítem sin importar su año de registro.

El **año** es uno de los campos de búsqueda primarios, debido a su importancia para filtrar y segmentar la información. Esta característica se vuelve fundamental dado que el dataset abarca más de una década de datos.

El ID de **Código de Barras** se designa como otro campo de búsqueda primario. Su función es actuar como un identificador único para cada ejemplar dentro del dataset. El uso de este campo garantiza:

* Precisión absoluta, dado que al buscar por ID de código de barras, se elimina cualquier ambigüedad, asegurando que el resultado sea exactamente el ejemplar deseado.

### Rangos de Valores Válidos para las Entradas 

* **Tipo de búsqueda**: 1, 2 o 3, para seleccionar entre las opciones respectivas “Buscar por ID y año”, “Buscar ID en todos los años” o “Salir”.
* **Año de búsqueda**: desde 2005 hasta 2017.
* **ID del recurso físico**: número entero largo.

## 4. Ejemplos de Uso del Programa

### 4.1. Pasos de Ejecución 

Para compilar cada componente de tu programa, usa los siguientes comandos:

```bash
gcc -o indexer indexer.c
gcc -o backend backend.c
gcc -o frontend frontend.c
```

Una vez compilado, el primer paso es generar el archivo índice de los hashes de todos los archivos CSV. Para hacer esto, ejecuta:
```bash
./indexer
```
Después de generar el índice, necesitas crear las tuberías de comunicación:
```bash
mkfifo /tmp/checkout_req_pipe 
mkfifo /tmp/checkout_res_pipe
```
Ahora, en una terminal, ejecuta el backend:
```bash
./backend
```
Espera a que el backend muestre el mensaje de que el servicio de búsqueda está iniciado, el índice cargado y que está esperando conexiones.
Finalmente, en otra terminal, ejecuta el frontend:
```bash
./frontend
```
### 4.2. Ejemplos específicos de búsquedas

#### Búsqueda por ID en Todos los Años (Opción 2)

Este tipo de búsqueda permite al usuario encontrar todas las ocurrencias de un **ID de código de barras** específico a lo largo de todos los años disponibles en el dataset.

Output obtenido:
```bash
--- MENU PRINCIPAL ---
1) Buscar por ID y año
2) Buscar ID en todos los años
3) Salir
Opción: 2

ID a buscar en todos los años: 0010066538447

>> Resultado(s):
Año:BibNumber,ItemBarcode,ItemType,Collection,CallNumber,CheckoutDateTime
2009:2553230,0010066538447,acdvd,nadvd,FRENCH DVD NE LED,04/15/2009 05:30:00 PM
2010:2553230,0010066538447,acdvd,nadvd,FRENCH DVD NE LED,09/16/2010 06:19:00 PM
2011:2553230,0010066538447,acdvd,nadvd,FRENCH DVD NE LED,06/11/2011 11:14:00 AM
2012:2553230,0010066538447,acdvd,nadvd,FRENCH DVD NE LED,12/07/2012 03:39:00 PM
2013:2553230,0010066538447,acdvd,nadvd,FRENCH DVD NE LED,05/30/2013 12:16:00 PM
```
---

#### Búsqueda por ID y Año Específico (Opción 1)

Esta opción permite una búsqueda más precisa, especificando tanto el **ID de código de barras** como el **año** en el que se desea buscar.

Output obtenido:
```bash
--- MENU PRINCIPAL ---
1) Buscar por ID y año
2) Buscar ID en todos los años
3) Salir
Opción: 1

Año (2005-2017): 2009
ID a buscar: 0010066538447

>> Resultado(s):
Año:BibNumber,ItemBarcode,ItemType,Collection,CallNumber,CheckoutDateTime
2553230,0010066538447,acdvd,nadvd,FRENCH DVD NE LED,04/15/2009 05:30:00 PM
```
---

#### Búsqueda con Año No Válido

Se muestra cómo el programa maneja la entrada de un año que está fuera del rango permitido (2005-2017).
```bash
Output obtenido:
--- MENU PRINCIPAL ---
1) Buscar por ID y año
2) Buscar ID en todos los años
3) Salir
Opción: 1

Año (2005-2017): 2020
Año no válido.

--- MENU PRINCIPAL ---
1) Buscar por ID y año
2) Buscar ID en todos los años
3) Salir
Opción: 1

Año (2005-2017): 1900
Año no válido.
```
---

#### Búsqueda con ID No Existente en el Dataset

Este ejemplo ilustra el comportamiento del sistema cuando se busca un **ID de código de barras** que no se encuentra en el dataset, incluso si el año es válido.
```bash
Output obtenido:
--- MENU PRINCIPAL ---
1) Buscar por ID y año
2) Buscar ID en todos los años
3) Salir
Opción: 1

Año (2005-2017): 2005
ID a buscar: 1111

>> Resultado(s):
Año:BibNumber,ItemBarcode,ItemType,Collection,CallNumber,CheckoutDateTime
>> Registro no encontrado.
```