# Programación II

# PROYECTO 1: SISTEMA DE GESTIÓN DE TALLER MECÁNICO

**República Bolivariana de Venezuela**  
**Universidad Rafael Urdaneta — Facultad de Ingeniería**  
**Escuela de Ingeniería de Computación**  
**Asignatura:** Programación 2 (Trimestre 2026-C)  
**Profesor:** Ing. Victor Kneider  

---

## 1. Descripción del Sistema e Integrantes

### Integrantes de la Pareja:
* **Integrante 1:** Luis Gómez — C.I.: V-32.801.258
* **Integrante 2:** Nelson Mendoza — C.I.: V-32.444.541

### Descripción General:
El **Sistema de Gestión de Taller Mecánico ("Autoservicio El Wikiti")** es una aplicación de consola desarrollada en C++ (estándar C++11) que permite administrar vehículos, mecánicos, inventario de repuestos y órdenes de servicio. El sistema implementa gestión manual y segura de memoria dinámica mediante punteros (`new` y `delete[]`), redimensionamiento automático de arreglos dinámicos al alcanzar su capacidad límite, memoria dinámica anidada dentro de estructuras (`OrdenServicio` -> `DetalleOrden*`), operaciones CRUD completas con integridad referencial e histórica de precios, y una separación estricta entre la Capa de Lógica y la Capa de Presentación.

---

## 2. Instrucciones de Compilación y Ejecución

### Requisitos:
* Compilador de C++ compatible con C++11 o superior (`g++`).

### Compilación:
Abra una terminal en el directorio del proyecto y ejecute el siguiente comando:

    g++ -std=c++11 proyecto_test.cpp -o taller

### Ejecución Interactiva:
* **En Windows:**

      taller.exe

* **En Linux / macOS:**

      ./taller

### Ejecución con Redirección de Archivos (Batería de Pruebas):
Para ejecutar los 8 casos de uso obligatorios de forma automatizada leyendo desde `pruebas/entrada.txt` y volcando la salida en `pruebas/salida.txt`:

    ./taller < pruebas/entrada.txt > pruebas/salida.txt

---

## 3. Diagrama de las Estructuras y sus Relaciones

El sistema emplea la estructura principal `SistemaTaller` como contexto general. Esta estructura contiene por valor los datos del `Taller` y administra cuatro arreglos dinámicos principales en el *heap* (`vehiculos`, `mecanicos`, `repuestos` y `ordenes`), cada uno inicializado con capacidad para 4 elementos. A su vez, cada `OrdenServicio` gestiona su propio arreglo dinámico interno `detalles` (`DetalleOrden*`), inicializado con capacidad para 2 elementos:

    +-------------------------------------------------------------------------+
    |                              SistemaTaller                              |
    +-------------------------------------------------------------------------+
    | + taller : Taller (nombre, rif, direccion, telefono)                    |
    | + Contadores: numVehiculos, numMecanicos, numRepuestos, numOrdenes      |
    | + Capacidades: capacidadVehiculos (4), capacidadMecanicos (4), ...      |
    | + Autoincrementales: siguienteIdVehiculo (1), siguienteIdMecanico (1)...|
    |                                                                         |
    | + vehiculos : Vehiculo* --------> [ Vehiculo 0 | Vehiculo 1 | ... ]     |
    |                                         ^                               |
    |                                         | (Relación por idVehiculo)     |
    | + mecanicos : Mecanico* --------> [ Mecanico 0 | Mecanico 1 | ... ]     |
    |                                         ^                               |
    |                                         | (Relación por idMecanico)     |
    | + ordenes   : OrdenServicio* ---> [ Orden 0    | Orden 1    | ... ]     |
    |                                         |                               |
    |                                         | (Puntero dinámico detalles*)  |
    |                                         v                               |
    |                                   [ Detalle 0  | Detalle 1  | ... ]     |
    |                                     (idRepuesto, cantidad, precioUnit.) |
    |                                         |                               |
    |                                         | (Relación por idRepuesto)     |
    |                                         v                               |
    | + repuestos : Repuesto* --------> [ Repuesto 0 | Repuesto 1 | ... ]     |
    +-------------------------------------------------------------------------+

### Resumen de Relaciones:
1. **`SistemaTaller` -> Entidades (1 a N):** Administra los arreglos dinámicos de `Vehiculo`, `Mecanico`, `Repuesto` y `OrdenServicio`. Cuando `num == capacidad`, el arreglo correspondiente duplica su capacidad (`capacidad * 2`).
2. **`OrdenServicio` -> `DetalleOrden` (Composición dinámica 1 a N):** Cada orden posee un puntero `DetalleOrden* detalles` reservado en el *heap* con capacidad inicial de 2 líneas, duplicándose automáticamente cuando se agregan más tipos de repuestos a la misma orden.
3. **Integridad entre Entidades:** Un `Vehiculo` no puede eliminarse si su `id` aparece en alguna `OrdenServicio`. Un `Repuesto` no puede eliminarse si su `id` figura dentro del arreglo `detalles` de cualquier orden.

---

## 4. Explicación de la Separación de Capas

El programa respeta estrictamente la arquitectura de dos capas exigida:

1. **Capa de Lógica (Negocio y Gestión de Memoria):**
   * Opera exclusivamente sobre el puntero `SistemaTaller* s` y los parámetros recibidos.
   * Contiene todas las validaciones de formato y reglas de negocio (homogeneidad de placas, cédulas y códigos, límite de 3 órdenes abiertas por mecánico, validación de kilometraje, descuento y devolución de stock, y cálculo de totales).
   * Gestiona todas las reservas (`new`), redimensionamientos y liberaciones (`delete[]`) de las estructuras de datos.
   * **Regla cumplida:** Ninguna función de lógica contiene instrucciones `cin` ni `cout`. Toda respuesta se entrega mediante el valor de retorno (`Entidad*`, `Entidad**`, `bool`, `int` o `float`).

2. **Capa de Presentación (Interfaz de Usuario / UI):**
   * Compuesta por funciones `void` (`menu...`, `ui...`, `mostrar...`) que interactúan con el usuario mediante `cin` y `cout`.
   * Captura los datos de entrada, solicita confirmaciones `(S/N)` en operaciones críticas, invoca a las funciones de la Capa de Lógica y traduce sus resultados en mensajes claros de éxito o de error específico.
   * **Regla cumplida:** Nunca modifica directamente los arreglos internos del sistema y siempre libera con `delete[]` los arreglos temporales de punteros (`Entidad**`) retornados por las funciones de búsqueda y listado.

---

## 5. Firmas Diseñadas (Mecánicos, Repuestos y Órdenes) y Justificación

Siguiendo el modelo del módulo de Vehículos, se diseñaron las siguientes firmas para la Capa de Lógica:

### 5.1 Módulo de Mecánicos
* `Mecanico* buscarMecanicoPorCedula(SistemaTaller* s, const char* cedula);`
  * **Justificación:** Retorna un puntero `Mecanico*` al mecánico encontrado para consultar sus datos, o `nullptr` si no existe, lo cual permite verificar fácilmente que la cédula sea única antes de registrar.
* `Mecanico* buscarMecanicoPorID(SistemaTaller* s, int id);`
  * **Justificación:** Retorna `Mecanico*` apuntando a la estructura dentro de `s->mecanicos` para acceso directo en búsquedas y órdenes de servicio, o `nullptr` si el ID no está registrado.
* `Mecanico* registrarMecanico(SistemaTaller* s, Mecanico datos);`
  * **Justificación:** Retorna `Mecanico*` apuntando al mecánico recién insertado (con su ID autoincremental ya asignado), o `nullptr` si falla alguna regla de negocio (cédula duplicada, formato inválido o especialidad no permitida).
* `Mecanico** listarMecanicos(SistemaTaller* s, int* cantidad);`
  * **Justificación:** Retorna un arreglo dinámico de punteros (`Mecanico**`) y escribe el total en `*cantidad`, permitiendo a la capa visual recorrer los mecánicos sin duplicar estructuras completas ni comprometer el arreglo original.
* `int contarOrdenesAbiertasMecanico(SistemaTaller* s, int idMecanico);`
  * **Justificación:** Retorna un entero (`int`) con el número exacto de órdenes en estado `"ABIERTA"` asignadas a ese mecánico, dato indispensable tanto para el listado de mecánicos como para validar el tope máximo de 3 órdenes abiertas.

### 5.2 Módulo de Repuestos (Inventario)
* `Repuesto* registrarRepuesto(SistemaTaller* s, Repuesto datos);`
  * **Justificación:** Retorna `Repuesto*` apuntando al nuevo repuesto creado en memoria dinámica, o `nullptr` si el código ya existe, el precio es `<= 0` o los stocks son negativos.
* `Repuesto* buscarRepuestoPorID(SistemaTaller* s, int id);`
  * **Justificación:** Retorna `Repuesto*` para localizar el repuesto por su identificador único y operar sobre su stock o precio, o `nullptr` si no existe.
* `Repuesto* buscarRepuestoPorCodigo(SistemaTaller* s, const char* codigo);`
  * **Justificación:** Retorna `Repuesto*` cuando halla coincidencia exacta con el código alfanumérico del repuesto, o `nullptr` si no está registrado.
* `Repuesto** buscarRepuestosPorNombreParcial(SistemaTaller* s, const char* subcadena, int* cantidad);`
  * **Justificación:** Retorna `Repuesto**` (arreglo dinámico de punteros) porque una subcadena de nombre puede coincidir con varios repuestos; almacena la cantidad encontrada en `*cantidad`.
* `Repuesto** listarRepuestos(SistemaTaller* s, int* cantidad);`
  * **Justificación:** Retorna `Repuesto**` con las direcciones de memoria de todos los repuestos del inventario para que la capa visual los imprima y evalúe la alerta de `BAJO STOCK`.
* `bool registrarEntradaMercancia(SistemaTaller* s, int idRepuesto, int cantidad);`
  * **Justificación:** Retorna `bool` (`true` si encontró el repuesto y sumó la cantidad `> 0` a su stock; `false` en caso contrario).
* `bool cambiarPrecioRepuesto(SistemaTaller* s, int idRepuesto, float nuevoPrecio);`
  * **Justificación:** Retorna `bool` confirmando si se actualizó el precio vigente (`true`) o si falló por ID inexistente o precio `<= 0` (`false`).
* `bool eliminarRepuesto(SistemaTaller* s, int idRepuesto);`
  * **Justificación:** Retorna `bool` (`true` si eliminó físicamente el repuesto desplazando el arreglo; `false` si no existe o si el repuesto fue utilizado en el detalle de alguna orden).

### 5.3 Módulo de Órdenes de Servicio
* `OrdenServicio* abrirOrden(SistemaTaller* s, int idVehiculo, int idMecanico, const char* fechaApertura, const char* descripcionFalla, int kilometrajeIngreso);`
  * **Justificación:** Retorna `OrdenServicio*` apuntando a la nueva orden abierta (con su arreglo `detalles` reservado para 2 elementos y el kilometraje del vehículo actualizado), o `nullptr` si incumple alguna validación.
* `OrdenServicio* buscarOrdenPorID(SistemaTaller* s, int id);`
  * **Justificación:** Retorna `OrdenServicio*` para acceder a la orden y a su arreglo interno de detalles, o `nullptr` si no existe.
* `OrdenServicio* buscarOrdenAbiertaVehiculo(SistemaTaller* s, int idVehiculo);`
  * **Justificación:** Retorna `OrdenServicio*` si el vehículo tiene actualmente una orden en estado `"ABIERTA"` (permitiendo a la capa visual mostrar exactamente el ID de la orden conflictiva), o `nullptr` si no tiene órdenes abiertas.
* `bool agregarRepuestoAOrden(SistemaTaller* s, int idOrden, int idRepuesto, int cantidad);`
  * **Justificación:** Retorna `bool` (`true` si la orden estaba abierta, había stock suficiente, se descontó del inventario y se agregó o acumuló en `orden->detalles`; `false` si falló alguna condición).
* `bool quitarRepuestoDeOrden(SistemaTaller* s, int idOrden, int idRepuesto);`
  * **Justificación:** Retorna `bool` (`true` si eliminó la línea completa de `orden->detalles` y devolvió las unidades al inventario; `false` si la orden no está abierta o el repuesto no figuraba en ella).
* `bool cerrarOrden(SistemaTaller* s, int idOrden, float manoObra, const char* fechaCierre);`
  * **Justificación:** Retorna `bool` indicando si se pudo registrar la mano de obra, asignar la fecha de cierre y cambiar el estado a `"CERRADA"`.
* `bool anularOrden(SistemaTaller* s, int idOrden);`
  * **Justificación:** Retorna `bool` (`true` si la orden estaba abierta o cerrada, revirtió al inventario el stock de todas sus líneas de detalle y cambió su estado a `"ANULADA"`).
* `float calcularTotalOrden(OrdenServicio* orden);`
  * **Justificación:** Retorna `float` con el cálculo exacto derivado de `manoObra + Σ(cantidad * precioUnitario)` sin almacenarlo de forma redundante en la estructura.
* `OrdenServicio** listarOrdenesPorEstado(SistemaTaller* s, const char* estadoBuscado, int* cantidad);`
  * **Justificación:** Retorna `OrdenServicio**` con las direcciones de todas las órdenes cuyo estado coincide con el filtro solicitado.
* `OrdenServicio** listarOrdenesPorVehiculo(SistemaTaller* s, int idVehiculo, int* cantidad);`
  * **Justificación:** Retorna `OrdenServicio**` con el historial de órdenes asociadas a un vehículo específico.

---

## 6. Preguntas de Análisis

### Pregunta 1:
*`agregarVehiculo` retorna un puntero a un elemento de `s->vehiculos`. Si guardan ese puntero en una variable, registran varios vehículos más y luego lo usan, ¿qué puede ocurrir y por qué?*

**Respuesta:**  
Puede ocurrir un fallo crítico de memoria conocido como **puntero colgante (*dangling pointer*)**, provocando que al usar esa variable se lean datos corruptos/basura o que el programa sufra un cierre inesperado (*segmentation fault* / *crash*).  
**¿Por qué ocurre?** Porque el puntero retornado apunta a una casilla dentro del bloque de memoria actual de `s->vehiculos` (cuya capacidad inicial es 4). Si guardamos ese puntero en una variable externa y registramos más vehículos hasta llenar el arreglo (`numVehiculos == capacidadVehiculos`), la función `agregarVehiculo` redimensiona el arreglo: solicita un bloque nuevo más grande en otra zona de la memoria (`new Vehiculo[nuevaCapacidad]`), copia los vehículos al bloque nuevo y **destruye el bloque viejo mediante `delete[] s->vehiculos`**. A partir de ese momento, la dirección guardada en nuestra variable apunta a memoria que ya fue liberada e invalidada.

### Pregunta 2:
*¿Por qué `DetalleOrden` guarda `precioUnitario` en lugar de consultar el precio del repuesto cada vez que se calcula el total?*

**Respuesta:**  
Porque es necesario conservar el **precio histórico** al que se vendió o instaló el repuesto en el momento exacto de la operación. En un taller real, el precio de los repuestos en el inventario (`Repuesto.precio`) se actualiza periódicamente (usando la función `cambiarPrecioRepuesto`). Si `calcularTotalOrden` consultara siempre el precio vigente en la estructura `Repuesto`, cualquier aumento de precio en el inventario modificaría retroactivamente el monto total de órdenes que ya fueron cerradas y cobradas en el pasado. Al guardar `precioUnitario` dentro de `DetalleOrden` cuando se agrega el repuesto, el valor de esa orden queda blindado frente a futuros cambios de precios.

### Pregunta 3:
*Cuando se redimensiona el arreglo de órdenes, ¿qué ocurre con el arreglo de detalles de cada orden? ¿Se copia, se comparte o se pierde? Justifiquen con su implementación.*

**Respuesta:**  
El arreglo dinámico de detalles (`DetalleOrden* detalles`) de cada orden **no se pierde ni se duplica físicamente elemento por elemento; su dirección de memoria se transfiere (se copia el puntero) desde el arreglo viejo de órdenes hacia el nuevo arreglo redimensionado**.  
**Justificación según nuestra implementación:**  
En la función `abrirOrden`, cuando `s->numOrdenes >= s->capacidadOrdenes`, ejecutamos:

    int nuevaCap = s->capacidadOrdenes * 2;
    OrdenServicio* nuevasO = new OrdenServicio[nuevaCap];
    for (int i = 0; i < s->numOrdenes; i++) nuevasO[i] = s->ordenes[i];
    delete[] s->ordenes;
    s->ordenes = nuevasO;

Al hacer la asignación `nuevasO[i] = s->ordenes[i];`, C++ realiza una copia superficial (*shallow copy*) de cada estructura `OrdenServicio`. Esto significa que el puntero interno `nuevasO[i].detalles` recibe exactamente la misma dirección de memoria del *heap* donde ya estaban almacenados los detalles (`s->ordenes[i].detalles`). Inmediatamente después, al ejecutar `delete[] s->ordenes;`, solo se libera el arreglo contenedor viejo de estructuras `OrdenServicio`, pero **no** se invoca `delete[]` sobre los punteros `.detalles` de cada orden. De esta manera, los arreglos de detalles permanecen intactos en sus mismas direcciones de memoria y pasan a ser administrados por las órdenes del nuevo arreglo `s->ordenes` sin fugas ni pérdida de información.
