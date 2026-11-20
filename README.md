# PROYECTO 2: TALLER MECÁNICO CON PERSISTENCIA Y ACCESO ALEATORIO

**República Bolivariana de Venezuela**  
**Universidad Rafael Urdaneta — Facultad de Ingeniería**  
**Escuela de Ingeniería de Computación**  
**Asignatura:** Programación 2 (Trimestre 2026-C)  
**Profesor:** Ing. Victor Kneider  

---

## 1. Integrantes y Descripción del Sistema

### Integrantes de la Pareja:
* **Integrante 1:** Luis Gómez — C.I.: V-32801258
* **Integrante 2:** Nelson Mendoza — C.I.: V-32444541

### Descripción General:
Evolución del Sistema de Gestión de Taller Mecánico del Proyecto 1 hacia una arquitectura persistente basada en **archivos binarios (`<fstream>`) y acceso aleatorio (`seekg` / `seekp`)**. El sistema elimina los arreglos dinámicos en memoria RAM (`SistemaTaller`) y almacena toda la información en la carpeta `datos/` (`taller.bin`, `vehiculos.bin`, `mecanicos.bin`, `repuestos.bin` y `ordenes.bin`), cargando únicamente un registro a la vez en memoria, aplicando borrado lógico con preservación de historial y manteniendo una separación estricta en **3 capas**: Persistencia, Lógica y Presentación.

---

## 2. Instrucciones de Compilación y Ejecución

### Requisitos:
* Compilador compatible con C++11 o superior (`g++` / MinGW-w64).

### Compilación:
Desde la terminal ubicada en la carpeta raíz del proyecto:

    g++ -std=c++11 proyecto_test.cpp -o taller

### Ejecución Interactiva:
* **En Windows:**

      taller.exe

* **En Linux / macOS:**

      ./taller

> **Nota sobre la inicialización:** Al ejecutarse por primera vez, el sistema crea automáticamente la carpeta `datos/`, inicializa todos los archivos `.bin` con su `ArchivoHeader` y ejecuta `prepoblarDatosIniciales()` si `vehiculos.bin` se encuentra vacío.

### Ejecución con Redirección de Archivos (Casos de Prueba):

    taller.exe < pruebas/entrada_1a.txt > pruebas/salida_1a.txt

---

## 3. Tamaños de Estructuras (`sizeof`) y Fórmula de Acceso Aleatorio

Todas las estructuras utilizan arreglos de caracteres de tamaño fijo (`char[]`) y tipos primitivos sin punteros ni `std::string`, garantizando un `sizeof` constante conocido en tiempo de compilación.

### 3.1 Tabla de `sizeof` (Compilador C++ 64 bits con `time_t` de 8 bytes y alineación de memoria)

| Estructura | Archivo Asociado | Tamaño (`sizeof`) | Notas |
| :--- | :--- | :---: | :--- |
| `ArchivoHeader` | Inicio de cada `.bin` | **16 bytes** | 4 campos `int` (4 bytes c/u). |
| `Taller` | `datos/taller.bin` | **360 bytes** | Registro único sin `ArchivoHeader`. |
| `Vehiculo` | `datos/vehiculos.bin` | **288 bytes** | Incluye `bool eliminado` y dos `time_t`. |
| `Mecanico` | `datos/mecanicos.bin` | **176 bytes** | Incluye `bool eliminado` (baja lógica) y dos `time_t`. |
| `Repuesto` | `datos/repuestos.bin` | **208 bytes** | Incluye `bool eliminado` y dos `time_t`. |
| `DetalleOrden` | Embebido en `OrdenServicio` | **12 bytes** | `idRepuesto` (4) + `cantidad` (4) + `precioUnitario` (4). |
| `OrdenServicio` | `datos/ordenes.bin` | **464 bytes** | Incluye arreglo fijo `DetalleOrden detalles[15]` (180 bytes). |

### 3.2 Fórmula de Posición (Offset en Bytes)
Para ubicar cualquier registro en disco sin recorrer el archivo, la función `calcularOffset(int id, size_t tamanioEstructura)` aplica la siguiente fórmula:

    posicion = sizeof(ArchivoHeader) + (id - 1) * sizeof(Estructura)

Con `sizeof(ArchivoHeader) = 16`, por ejemplo, el **Vehículo con ID #5** se lee directamente en:  
`posicion = 16 + (5 - 1) * 288 = 16 + 1152 = 1168 bytes`.

---

## 4. Relación entre ID e Índice Físico

### Relación Deducida:
    indiceFisico = id - 1

Esta relación directa funciona porque el sistema cumple simultáneamente tres condiciones:
1. Los IDs se asignan de forma estrictamente autoincremental comenzando en `1` (`proximoID = 1`).
2. Todo registro nuevo se inserta siempre al final del archivo.
3. Los registros nunca se mueven de lugar ni se eliminan físicamente (solo se marcan con `eliminado = true`).

### ¿Qué cambio en el sistema dejaría de hacer válida esta relación?
La fórmula `indiceFisico = id - 1` dejaría de ser válida si se implementara cualquiera de los siguientes cambios:
* **Borrado físico o compactación del archivo:** Si se eliminara físicamente un registro (por ejemplo, el ID #3) y se desplazaran los registros posteriores (ID #4, #5, #6) hacia atrás para ahorrar espacio, el ID #5 pasaría al índice `3` en lugar del `4`, rompiendo el cálculo directo.
* **Reordenamiento físico del archivo:** Si se ordenara el archivo en disco por otro criterio (por ejemplo, alfabéticamente por placa o nombre).
* **Asignación de IDs manuales o no consecutivos:** Si se permitieran IDs arbitrarios (ej. saltar del ID 10 al ID 50) o reutilizar huecos físicos con nuevos IDs.

---

## 5. Diseño del Detalle de la Orden de Servicio

### 5.1 Estructura Diseñada y su `sizeof`
Se embebió un **arreglo de tamaño fijo de 15 posiciones** (`const int MAX_DETALLES_ORDEN = 15;`) directamente dentro de `OrdenServicio`, acompañado de un contador entero `numDetalles`:

    struct DetalleOrden {
        int   idRepuesto;     // 4 bytes
        int   cantidad;       // 4 bytes
        float precioUnitario; // 4 bytes (precio histórico congelado)
    }; // sizeof(DetalleOrden) = 12 bytes

    // Dentro de OrdenServicio (sizeof = 464 bytes):
    DetalleOrden detalles[15]; // 15 * 12 = 180 bytes fijos
    int          numDetalles;  // 4 bytes

### 5.2 Espacio Ocupado en Disco (Orden con 1 línea vs. 15 líneas)
* **Orden con 1 línea de detalle:** Ocupa **464 bytes** en `ordenes.bin`. Del bloque de 180 bytes reservado para detalles, utiliza **12 bytes** y desperdicia **168 bytes** (14 casillas vacías — fragmentación interna).
* **Orden con 15 líneas de detalle:** Ocupa exactamente los mismos **464 bytes** en `ordenes.bin`, aprovechando los **180 bytes** completos del arreglo de detalles (**0 bytes desperdiciados**).

### 5.3 Alternativa Considerada y Motivo de Descarte
* **Alternativa considerada:** Crear un sexto archivo binario dedicado (`datos/detalles.bin`) con su propio `ArchivoHeader`, donde cada registro representara una línea de detalle con un campo adicional `int idOrden;` y `bool eliminado;`.
* **Por qué se descartó:** Aunque esa alternativa elimina el desperdicio de bytes en órdenes pequeñas y no impone un tope de 15 líneas, **rompe el acceso aleatorio directo al detalle de una orden**. Como las líneas de distintas órdenes se intercalan al insertarse cronológicamente, consultar una orden o calcular su total exigiría recorrer secuencialmente todo `detalles.bin`. Además, obligaría a coordinar un sexto archivo en operaciones compuestas, verificación de integridad y respaldo. Con el arreglo fijo embebido, un solo `seekg()` en `ordenes.bin` recupera instantáneamente la orden y todas sus líneas.

### 5.4 Cómo se Resuelve la Operación de Quitar una Línea
En `quitarRepuestoDeOrden(int idOrden, int idRepuesto)`:
1. Se lee la orden por ID desde `ordenes.bin` y se valida que esté en estado `"ABIERTA"`.
2. Se busca la posición `idx` del `idRepuesto` dentro de `orden.detalles[0 ... numDetalles - 1]`.
3. Se lee el repuesto en `repuestos.bin`, se suma la `cantidad` de la línea a su `stock` y se sobrescribe en `repuestos.bin`.
4. Se desplazan las líneas siguientes una posición hacia la izquierda (`orden.detalles[i] = orden.detalles[i + 1]`), se decrementa `orden.numDetalles--` y se sobrescribe la orden modificada en su misma posición de `ordenes.bin` (`seekp`).

---

## 6. Firmas Diseñadas (Capa de Persistencia y Capa de Lógica)

### Estrategia de Retorno y Comunicación de Errores
Al desaparecer los arreglos en RAM, ninguna función retorna punteros a estructuras (`Entidad*` o `Entidad**`). En su lugar:
* Se definió el **`enum CodigoResultado`** (`RES_OK`, `RES_ERROR_ARCHIVO`, `RES_NO_EXISTE`, `RES_ELIMINADO`, `RES_DUPLICADO`, `RES_DATOS_INVALIDOS`, `RES_TIENE_RELACIONES`, `RES_VEHICULO_NO_EXISTE`, `RES_VEHICULO_ELIMINADO`, `RES_MECANICO_NO_EXISTE`, `RES_MECANICO_DE_BAJA`, `RES_REPUESTO_NO_EXISTE`, `RES_REPUESTO_ELIMINADO`, `RES_ORDEN_YA_ABIERTA`, `RES_LIMITE_ORDENES_MECANICO`, `RES_KM_MENOR`, `RES_ORDEN_NO_ABIERTA`, `RES_ORDEN_YA_ANULADA`, `RES_STOCK_INSUFICIENTE`, `RES_LIMITE_DETALLES`, `RES_DETALLE_VACIO_SIN_MANO_OBRA`).
* Las lecturas individuales reciben una referencia (`Entidad &salida`) donde depositan la copia leída del disco, y retornan `CodigoResultado` para que la presentación sepa con precisión si tuvo éxito o cuál fue la causa exacta del fallo.
* Las búsquedas múltiples reciben un arreglo fijo de tamaño `MAX_RESULTADOS = 20`, una referencia `int &cantidad` y una bandera `bool &hayMas` que avisa a la presentación si quedaron registros sin mostrar en disco.
* La Capa de Lógica nunca abre archivos directamente: consulta el total histórico mediante `persistenciaObtenerTotalRegistros(ruta)` e itera por ID llamando a `persistenciaLeer...PorID(...)`.

### 6.1 Capa de Persistencia (Headers, Taller y 4 Entidades)
* **Utilidades de Archivo y Taller:**
  * `bool inicializarArchivoEntidad(const char* ruta);` — Crea el archivo con `ArchivoHeader` inicial si no existe; retorna `true` si quedó listo.
  * `bool leerHeader(const char* ruta, ArchivoHeader &header);` — Lee el header en el byte 0; retorna `false` si falla la apertura/lectura.
  * `bool actualizarHeader(const char* ruta, const ArchivoHeader &header);` — Sobrescribe únicamente el header en el byte 0.
  * `int persistenciaObtenerTotalRegistros(const char* ruta);` — Devuelve `cantidadRegistros` desde el header sin recorrer el archivo.
  * `bool existeArchivoTaller();` / `bool guardarTaller(Taller &datos);` / `bool leerTaller(Taller &datos);` — Gestión del registro único de `taller.bin`.
* **Operaciones por Entidad** *(implementadas para `Vehiculo`, `Mecanico`, `Repuesto` y `OrdenServicio`)*:
  * `CodigoResultado persistenciaGuardar<Entidad>(<Entidad> &nuevo, int &idAsignado);` — Asigna ID y `time_t`, escribe al final, actualiza el header y entrega el ID en `idAsignado`. Retorna `RES_OK` o `RES_ERROR_ARCHIVO`.
  * `CodigoResultado persistenciaLeer<Entidad>PorID(int id, <Entidad> &registro, long* posBytesLeida = nullptr);` — Acceso aleatorio por cálculo; entrega el registro esté activo o eliminado y opcionalmente el offset en bytes. Retorna `RES_OK`, `RES_NO_EXISTE` o `RES_ERROR_ARCHIVO`.
  * `CodigoResultado persistenciaSobrescribir<Entidad>(<Entidad> &registro);` — Sobrescribe en su offset original y actualiza `fechaUltimaModificacion`.
  * `CodigoResultado persistenciaEliminar<Entidad>(int id);` — Marca `eliminado = true` y decrementa `registrosActivos` en el header. Retorna `RES_OK`, `RES_NO_EXISTE`, `RES_ELIMINADO` o `RES_ERROR_ARCHIVO`.
  * `CodigoResultado persistenciaContarVehiculosActivos(int &cantidad);` / `CodigoResultado persistenciaContarOrdenesActivas(int &cantidad);` / `int persistenciaContarMecanicosActivos();` / `int persistenciaContarRepuestosActivos();` — Leen `registrosActivos` directo del header.

### 6.2 Capa de Lógica — Módulo Vehículos
* `CodigoResultado agregarVehiculo(Vehiculo datos, int &idAsignado);` — Valida formato, rango y placa única entre activos; retorna `RES_OK`, `RES_DATOS_INVALIDOS`, `RES_DUPLICADO` o `RES_ERROR_ARCHIVO`.
* `CodigoResultado buscarVehiculoPorID(int id, Vehiculo &salida, long* posBytesLeida);` — Distingue `RES_OK` (activo), `RES_ELIMINADO` (dado de baja) y `RES_NO_EXISTE`.
* `CodigoResultado buscarVehiculoPorPlaca(const char* placa, Vehiculo &salida);` — Busca placa exacta solo entre activos (`RES_OK` / `RES_NO_EXISTE`).
* `CodigoResultado buscarVehiculosPorPropietario(const char* subcadena, Vehiculo resultados[], int &cantidad, bool &hayMas);` — Coincidencia parcial ignorando mayúsculas, hasta `MAX_RESULTADOS`.
* `CodigoResultado listarVehiculos(Vehiculo resultados[], int &cantidad, bool &hayMas);` — Lista vehículos activos hasta `MAX_RESULTADOS`.
* `CodigoResultado actualizarVehiculo(int id, Vehiculo nuevosDatos);` — Modifica marca, modelo, año y propietario preservando ID, placa y kilometraje.
* `CodigoResultado eliminarVehiculo(int id);` — Borrado lógico; retorna `RES_TIENE_RELACIONES` si el vehículo tiene órdenes en cualquier estado.

### 6.3 Capa de Lógica — Módulo Mecánicos
* `CodigoResultado registrarMecanico(Mecanico datos, int &idAsignado);` — Valida cédula única entre activos, fecha y especialidad permitida.
* `CodigoResultado ObtenerMecanicoPorID(int id, Mecanico &salida, long* posBytesLeida = nullptr);` — Entrega los datos en `salida` y distingue `RES_OK` (activo) de `RES_ELIMINADO` (dado de baja).
* `CodigoResultado buscarMecanicoPorCedula(const char* cedula, Mecanico &salida);` — Verifica unicidad de cédula entre mecánicos activos.
* `CodigoResultado listarMecanicos(Mecanico resultados[], int &cantidad, bool &hayMas);` — Lista activos y dados de baja para conservar visibilidad histórica.
* `int ContarOrdenesAbiertasMecanico(int idMecanico);` — Cuenta cuántas órdenes en estado `"ABIERTA"` tiene asignadas.
* `CodigoResultado DarDeBajaMecanico(int id);` — Borrado lógico; retorna `RES_TIENE_RELACIONES` si tiene órdenes abiertas.

### 6.4 Capa de Lógica — Módulo Repuestos
* `CodigoResultado registrarRepuesto(Repuesto datos, int &idAsignado);` — Valida código único entre activos, precio > 0 y stocks >= 0.
* `CodigoResultado buscarRepuestoPorID(int id, Repuesto &salida, long* posBytesLeida = nullptr);` — Acceso directo distinguiendo `RES_OK`, `RES_ELIMINADO` y `RES_NO_EXISTE`.
* `CodigoResultado buscarRepuestoPorCodigo(const char* codigo, Repuesto &salida);` — Búsqueda por código exacto entre activos.
* `CodigoResultado buscarRepuestosPorNombreParcial(const char* subcadena, Repuesto resultados[], int &cantidad, bool &hayMas);` — Búsqueda parcial entre activos.
* `CodigoResultado registrarEntradaMercancia(int idRepuesto, int cantidad, int &nuevoStock);` — Suma unidades al stock y sobrescribe en disco.
* `CodigoResultado listarRepuestosActivos(Repuesto resultados[], int &cantidad, bool &hayMas);` — Lista activos; la UI marca `BAJO STOCK` cuando `stock <= stockMinimo`.
* `CodigoResultado cambiarPrecioRepuesto(int idRepuesto, float nuevoPrecio);` — Actualiza precio vigente sin afectar órdenes históricas.
* `CodigoResultado eliminarRepuesto(int idRepuesto);` — Borrado lógico; retorna `RES_TIENE_RELACIONES` si aparece en el detalle de alguna orden.

### 6.5 Capa de Lógica — Módulo Órdenes de Servicio
* `CodigoResultado buscarOrdenAbiertaVehiculo(int idVehiculo, OrdenServicio &salida, int* idOrdenEncontrada = nullptr);` — Verifica si un vehículo ya tiene una orden en estado `"ABIERTA"`.
* `CodigoResultado abrirOrden(int idVehiculo, int idMecanico, const char* fechaApertura, const char* descripcionFalla, int kilometrajeIngreso, int &idAsignado);` — Valida vehículo activo, mecánico activo, ausencia de otra orden abierta, tope de 3 órdenes por mecánico y kilometraje >= actual antes de actualizar `vehiculos.bin` y guardar en `ordenes.bin`.
* `CodigoResultado buscarOrdenPorID(int id, OrdenServicio &salida, long* posBytesLeida = nullptr);` — Lectura directa por ID.
* `CodigoResultado agregarRepuestoAOrden(int idOrden, int idRepuesto, int cantidad);` — Valida estado `"ABIERTA"`, stock y límite de 15 líneas; acumula si ya existe o crea línea nueva con precio histórico, descontando stock en `repuestos.bin` y actualizando `ordenes.bin`.
* `CodigoResultado quitarRepuestoDeOrden(int idOrden, int idRepuesto);` — Devuelve stock a `repuestos.bin`, remueve la línea del arreglo fijo y actualiza `ordenes.bin`.
* `CodigoResultado cerrarOrden(int idOrden, float manoObra, const char* fechaCierre);` — Valida mano de obra >= 0 y al menos una línea o mano de obra > 0; cambia estado a `"CERRADA"`.
* `CodigoResultado anularOrden(int idOrden);` — Restaura el stock de todas sus líneas en `repuestos.bin` y cambia el estado a `"ANULADA"` en `ordenes.bin`.
* `float calcularTotalOrden(const OrdenServicio &orden);` — Retorna `manoObra + Σ(cantidad * precioUnitario)`.
* `CodigoResultado listarOrdenesPorEstado(const char* estadoBuscado, OrdenServicio resultados[], int &cantidad, bool &hayMas);` — Filtra órdenes por estado.
* `CodigoResultado listarOrdenesPorVehiculo(int idVehiculo, OrdenServicio resultados[], int &cantidad, bool &hayMas);` — Devuelve el historial de órdenes de un vehículo.

---

## 7. Preguntas de Análisis

### Pregunta 1:
*Escriban con `write` una `OrdenServicio` del Proyecto 1 tal cual (con su `DetalleOrden*`), cierren el programa, vuelvan a ejecutarlo y lean ese registro. ¿Qué observaron al intentar acceder a sus líneas? ¿Por qué ocurre?*

**Respuesta:**  
Al intentar acceder a las líneas de detalle después de reiniciar el programa y leer el registro del archivo binario, el programa **muestra datos corruptos (basura de memoria) o se cierra abruptamente con un error crítico (*Segmentation Fault* / violación de acceso a memoria)**.  
**¿Por qué ocurre?** Porque `archivo.write()` copia literalmente los bytes que existen dentro de la estructura `OrdenServicio`. Cuando la estructura contiene un puntero (`DetalleOrden* detalles`), lo único que se guarda en el disco son los **8 bytes de la dirección de memoria RAM** donde estaba ubicado ese arreglo durante la primera ejecución, pero **no se guardan los datos de los repuestos** que estaban en el *heap*. Al cerrar el programa, el sistema operativo libera y reclama toda esa memoria RAM. Cuando el programa se abre de nuevo y lee el registro desde disco, recupera esa dirección de memoria vieja que ya no pertenece al programa (un *puntero colgante*), provocando un fallo inmediato al intentar leer `orden.detalles[0]`.

---

### Pregunta 2:
*Al agregar un repuesto a una orden se escriben el detalle y el stock. Si el programa se interrumpe entre esas dos escrituras, ¿qué inconsistencia queda según el orden que eligieron? ¿Qué orden de escritura deja el daño más fácil de detectar o corregir, y por qué?*

**Respuesta:**  
En nuestra función `agregarRepuestoAOrden`, el orden de escritura elegido es:
1. **Primero:** descontar el stock en `repuestos.bin`.
2. **Segundo:** guardar la nueva línea de detalle en `ordenes.bin`.

* **Inconsistencia si se interrumpe entre ambas escrituras:** El inventario en `repuestos.bin` queda con menos unidades de las que realmente están en el almacén, mientras que la orden en `ordenes.bin` no tiene registrado ese repuesto (un huérfano de stock).
* **Análisis de qué orden deja el daño más fácil de manejar:**
  * **Descontar primero el stock (`repuestos.bin` $\rightarrow$ `ordenes.bin`):** Es más seguro desde el punto de vista operativo del inventario, porque **impide la sobreventa** (nunca permitirá que dos órdenes consuman el último repuesto físico ni dejará stock negativo). Al detectar que sobran unidades físicas en el estante frente al sistema, se corrige registrando el ajuste de entrada o volviendo a cargar el repuesto a la orden.
  * **Escribir primero el detalle (`ordenes.bin` $\rightarrow$ `repuestos.bin`):** Si falla en medio, la orden queda cobrando un repuesto cuyo stock nunca se descontó del inventario (el sistema cree que tiene más unidades de las reales, arriesgando sobreventa). Optamos por **descontar primero `repuestos.bin`** para proteger la consistencia del stock disponible y evitar existencias negativas.

---

### Pregunta 3:
*En el Proyecto 1 no se podía eliminar un mecánico con órdenes. ¿Qué habría pasado con el historial si se hubiera permitido la eliminación física? ¿Por qué la baja lógica lo resuelve?*

**Respuesta:**  
Si se hubiera permitido la **eliminación física** de un mecánico que tenía órdenes cerradas o anuladas, habrían ocurrido dos problemas graves:
1. **Pérdida de integridad referencial e historial:** Todas las órdenes históricas que guardaban `idMecanico` pasarían a apuntar a un registro inexistente (ID huérfano). Al consultar una factura antigua o generar el Reporte 3 (Facturación por mecánico), no se sabría qué mecánico hizo el trabajo ni cuánto facturó.
2. **Corrupción del acceso aleatorio en disco:** En un archivo binario con acceso directo (`indice = id - 1`), borrar físicamente un registro y desplazar los siguientes hacia atrás cambiaría la posición de todos los mecánicos con ID mayor, haciendo que las órdenes de otros mecánicos apuntaran a la persona equivocada.

**¿Por qué la baja lógica lo resuelve?**  
Porque al marcar simplemente `eliminado = true` y mantener el registro intacto en su misma posición física dentro de `mecanicos.bin`, **la fórmula de acceso aleatorio nunca se rompe y el nombre del mecánico permanece siempre disponible en disco**. La Capa de Lógica bloquea nuevas asignaciones verificando `if (m.eliminado) return RES_MECANICO_DE_BAJA;`, mientras que las consultas históricas y el Reporte 3 leen su registro directamente por ID y muestran su nombre acompañado de la etiqueta `(BAJA)`.
