#include <iostream>
#include <windows.h>
using namespace std;


//sturcts
struct Taller{
    char nombre[100];
    char rif[20];
    char direccion[200];
    char telefono[20];
};

struct Vehiculo{
    int  id;
    char placa[10];
    char marca[50];
    char modelo[50];
    int  anio;
    int  kilometraje;
    char nombrePropietario[100];
    char cedulaPropietario[20];
    char telefonoPropietario[20];
    char fechaRegistro[11]; //yyyy-mm-dd
};

struct Mecanico{
    int id;
    char nombre[100];
    char cedula[20];
    char especialidad[20];
    char fechaIngreso[11]; //yyyy-mm-dd
};

struct Repuesto {
    int id;
    char codigo[20];
    char nombre[100];
    char marca[50];
    float precio;
    int stock;
    int stockMinimo;
    char fechaRegistro [11]; //yyyy-mm-dd
};

struct DetalleOrden {
    int idRepuesto;
    int cantidad;
    float precioUnitario;
};

struct OrdenServicio {
    int id;
    int idVehiculo;
    int idMecanico;
    char fechaApertura[11]; //yyyy-mm-dd
    char fechaCierre[11]; //vacia mientras la orden esté abierta
    char estado[12]; // Abierta, Cerrada, anulada
    char descripcionFalla[200];
    int kilometrajeIngreso;
    float manoObra;

    
    DetalleOrden* detalles;
    int           numDetalles;
    int           capacidadDetalles;
};

struct SistemaTaller{
    Taller taller;                  //Datos del taller
    
    Vehiculo*      vehiculos;    int numVehiculos; int capacidadVehiculos;
    Mecanico*      mecanicos;    int numMecanicos; int capacidadMecanicos;
    Repuesto*      repuestos;    int numRepuestos; int capacidadRepuestos;
    OrdenServicio* ordenes;      int numOrdenes;   int capacidadOrdenes;
    
    //Contadores para IDs autoincrementales
    int siguienteIdVehiculo;
    int siguienteIdMecanico;
    int siguienteIdRepuesto;
    int siguienteIdOrden;
};

//=========================
//      CAPA LOGICA
//=========================

//mouske herramienta misteriosa que me ayudara mas tarde
void limpiar(){
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void pausarylimpiar(){
    cout << "\nPresiona Enter para continuar...";
    cin.get();
    #ifdef _WIN32
        system("cls");//si estamos en windows -, limpia con 'cls'
    #else
        system("clear");//sino (linux/mac), limpia con 'clear'
    #endif
}

char aMinuscula(char c){
    if(c >= 'A' && c <= 'Z')return c + 32;
    return c;
}

bool compararSubcadenaIgnoreCase(const char* nombre, const char* sub){
    for(int i = 0; nombre[i] != '\0'; i++){
        int k = 0;
        while(nombre[i + k] != '\0' && sub[k] != '\0' && aMinuscula(nombre[i + k]) == aMinuscula(sub[k])){
            k++;
        }
        if (sub[k] == '\0') return true;
    }
    return false;
}

Vehiculo* buscarVehiculoPorPlaca(SistemaTaller* s, const char* placa);
//==========================================
//       CAPA DE LOGICA: VEHICULOS
//==========================================

//Inializar el sistema (reserva capacidad 4 para entidades y 2 para detalles)
void inicializarSistema(SistemaTaller* s, const char* nombre, const char* rif, const char* direccion, const char* telefono){
    
    //1. Copiar datos del taller caracter a caracter
    for(int i = 0; i < 99 && nombre[i] != '\0'; i++) s->taller.nombre[i] = nombre[i];
    s->taller.nombre[99] = '\0';

    for(int i = 0; i < 19 && rif[i] != '\0'; i++) s->taller.rif[i] = rif[i];
    s->taller.rif[19] = '\0';

    for(int i = 0; i < 199 && direccion[i] != '\0'; i++) s->taller.direccion[i] = direccion[i];
    s->taller.direccion[199] = '\0';

    for(int i = 0; i < 19 && telefono[i] != '\0'; i++) s->taller.telefono[i] = telefono[i];
    s->taller.telefono[19] = '\0';

    //2. Poner contadores en cero
    s->numVehiculos = 0;
    s->numMecanicos = 0;
    s->numRepuestos = 0;
    s->numOrdenes = 0;

    //3. Capacidades iniciales en en 4
    s->capacidadVehiculos = 4;
    s->capacidadMecanicos = 4;
    s->capacidadRepuestos = 4;
    s->capacidadOrdenes = 4;

    //4. Reservar memoria dinamica con new
    s->vehiculos = new Vehiculo[s->capacidadVehiculos];
    s->mecanicos = new Mecanico[s->capacidadMecanicos];
    s->repuestos = new Repuesto[s->capacidadRepuestos];
    s->ordenes = new OrdenServicio[s->capacidadOrdenes];

    //5. IDs en 1
    s->siguienteIdVehiculo = 1;
    s->siguienteIdMecanico = 1;
    s->siguienteIdRepuesto = 1;
    s->siguienteIdOrden = 1;
}

Vehiculo* agregarVehiculo(SistemaTaller* s, Vehiculo datos){
    // Validamos que los textos obligatorios no esten vacios
    if (datos.placa[0] == '\0' || datos.marca[0] == '\0' || datos.modelo[0] == '\0' ||
        datos.nombrePropietario[0] == '\0' || datos.cedulaPropietario[0] == '\0') {
        return nullptr;
    }
    
    //Validamos anio (1950 - 2027) y kilometraje (>=0)
    if(datos.anio < 1950 || datos.anio > 2027 || datos.kilometraje < 0){
        return nullptr;
    }

    //Validar que la placa no este duplicada
    if(buscarVehiculoPorPlaca(s, datos.placa) != nullptr){
        return nullptr;
    }
    
    //1. Verificar si el arreglo esta lleno 
    if (s->numVehiculos >= s->capacidadVehiculos){
        //1.1 Duplicar la capacidad
        int nuevaCapacidad = s->capacidadVehiculos * 2;

        //1.2 Crear un nuevo arreglo con la nueva capacidad
        Vehiculo* nuevosVehiculos = new Vehiculo[nuevaCapacidad];

        //1.3 Copiar los datos del arreglo viejo al nuevo 
        for (int i = 0; i < s->numVehiculos; i++){
            nuevosVehiculos[i] = s->vehiculos[i];
        }

        //1.4 Liberar la memoria del arreglo viejo
        delete[] s->vehiculos;

        //1.5 Actualizar el puntero y la capacidad
        s->vehiculos = nuevosVehiculos;
        s->capacidadVehiculos = nuevaCapacidad;
    }

    //2. Asignar un ID autoincremental al nuevo vehículo
    datos.id = s->siguienteIdVehiculo;
    s->siguienteIdVehiculo++;

    //3. Guardar el nuevo vehículo en el arreglo
    s->vehiculos[s->numVehiculos] = datos;

    //4. Incrementar el contador de vehículos
    s->numVehiculos++;

    //5. Retornar un puntero al nuevo vehículo
    return &(s->vehiculos[s->numVehiculos -1]);
}

Vehiculo* buscarVehiculoPorID(SistemaTaller* s, int id){
    for(int i = 0; i < s->numVehiculos; i++){
        if (s->vehiculos[i].id == id){
            //retornar la direccion de memoria del vehiculo encontrado
            return &(s->vehiculos[i]);
        }
    }
    //Si recorrimos todo y no esta
    return nullptr;
}

Vehiculo* buscarVehiculoPorPlaca(SistemaTaller* s, const char* placa){
    //Recorrer el arreglo de vehiculos
    for(int i = 0; i < s->numVehiculos; i++){

        //comparamos la placa del vehiculo actual caracter por caracter
        const char* p1 = s->vehiculos[i].placa;
        const char* p2 = placa;
        bool iguales = true;

        int j = 0;
        while (p1[j] != '\0' && p2[j] != '\0'){
            if (p1[j] != p2[j]){
                iguales = false;
                break;
            }
            j++;
        }
        
        //Validamos que ambas hayan terminado al mismo tiempo
        if (iguales && p1[j] == '\0' && p2[j] == '\0'){
            //encontramos el vehiculo, retornamos su direccion de memoria
            return &(s->vehiculos[i]);
        }
    }
    //Si recorrimos todo el taller y ninguna placa coincide
    return nullptr;
}

Vehiculo** buscarVehiculosPorPropietario(SistemaTaller* s, const char* subcadena, int* cantidad){
    //1. Primero contamos cuantos vehiculos coinciden con la subcadena del nombre del propietario
    int encontrados = 0;
    for(int i = 0; i < s->numVehiculos; i++){
        //funcion aux interna o logica para buscar subcadena
        //Verificamos si la subcadena esta dentro de nombrePropietario
        const char* nombre = s->vehiculos[i].nombrePropietario;
        const char* sub = subcadena;

        bool coincide = false;
        for(int j = 0; nombre[j] != '\0'; j++){
            int k = 0;
            while (nombre[j + k] != '\0' && sub[k] != '\0' && nombre[j + k] == sub[k]){
                k++;
            }
            if (sub[k] == '\0'){ //encontramos la coincidencia
                coincide = true;
                break;
            }
        }
        if (coincide){
            encontrados++;
        }
    }

    //Si no se encontro ninguno, devolvemos cantidad en 0 y nullptr
    *cantidad = 0;
    if (encontrados == 0){
        return nullptr;
    }

    //2.Reservamos memoria dinamica para un arreglo de punteros del tamanio exacto
    Vehiculo** resultados = new Vehiculo*[encontrados];

    //3.Volvemos a recorrer para llenar el arreglo con las direcciones de memoria
    int index = 0;
    for(int i = 0; i < s->numVehiculos; i++){
        const char* nombre = s->vehiculos[i].nombrePropietario;
        const char* sub = subcadena;

        bool coincide = false;
        for(int j = 0; nombre[j] != '\0'; j++){
            int k = 0;
            while (nombre[j + k] != '\0' && sub[k] != '\0' && nombre[j + k] == sub[k]){
                k++;
            }
            if (sub[k] == '\0'){ //encontramos la coincidencia
                coincide = true;
                break;
            }
        }
        if (coincide){
            resultados[index] = &(s->vehiculos[i]);
            index++;
        }
    }
    //Actualizamos el parametro de salida con la cantidad total hallada
    *cantidad = encontrados;
    return resultados;
}

Vehiculo** listarVehiculos(SistemaTaller* s, int* cantidad){
    //1.Verificamos si el taller esta vacio
    if (s->numVehiculos == 0){
        *cantidad = 0;
        return nullptr;
    }

    //2.Definimos la cantidad encontrada como el total de vehiculos actuales
    *cantidad = s->numVehiculos;

    //3, Reservamos memoria dinamica para un arreglo de punteros del tamanio exacto
    Vehiculo** lista = new Vehiculo*[s->numVehiculos];

    //4. Llenamos el arreglo con las direcciones de memoria de cada vehiculo existente
    for(int i = 0; i < s->numVehiculos; i++){
        lista[i] = &(s->vehiculos[i]);
    }

    //Retornamos el arreglo de punteros
    return lista;
}

bool actualizarVehiculo(SistemaTaller* s, int id, Vehiculo nuevosDatos){
    //1.Buscamos el vehiculo en el arreglo
    Vehiculo* v = buscarVehiculoPorID(s, id);

    //Si no existe, retornamos falso
    if (v == nullptr){
        return false;
    }

    //Actualizamos los campos numericos directamente
    v->anio = nuevosDatos.anio;

    //Copiamos la placa caracter por caracter
    int i = 0;
    while(nuevosDatos.placa[i] != '\0' && i < 9){
        v->placa[i] = nuevosDatos.placa[i];
        i++;
    }
    v->placa[i] = '\0';

    //Copiamos la marca caracter por caracter
    i = 0;
    while(nuevosDatos.marca[i] != '\0' && i < 49){
        v->marca[i] = nuevosDatos.marca[i];
        i++;
    }
    v->marca[i] = '\0';

    //Copiamos el modelo caracter por caracter
    i = 0;
    while(nuevosDatos.modelo[i] != '\0' && i < 49){
        v->modelo[i] = nuevosDatos.modelo[i];
        i++;
    }
    v->modelo[i] = '\0';

    //Copiamos el nombre del propietario caracter a caracter
    i = 0;
    while(nuevosDatos.nombrePropietario[i] != '\0' && i < 99){
        v->nombrePropietario[i] = nuevosDatos.nombrePropietario[i];
        i++;
    }
    v->nombrePropietario[i] = '\0';

    return true; //actualizacion exitante
}

bool eliminarVehiculo(SistemaTaller* s, int id){
    //Verificar si el vehiculo tiene ordenes de servicio registrada
    for(int i = 0; i < s->numOrdenes; i ++){
        if(s->ordenes[i].idVehiculo == id){
            return false;//rechazado:tiene ordenes asociadas
        }
    }
    
    int indiceEncontrado = -1;

    //Buscamos el indice exacto del vehiculo en el arreglo
    for(int i = 0; i < s->numVehiculos; i++){
        if (s->vehiculos[i].id == id){
            indiceEncontrado = i;
            break;
        }
    }

    //Si no se encuentra, retornamos falso
    if(indiceEncontrado == -1){
        return false;
    }

    //desplazamos los elemntos hacia la izquierda para "rellenar" el espacio
    for(int i = indiceEncontrado; i < s->numVehiculos - 1; i++){
        s->vehiculos[i] = s->vehiculos[i + 1];
    }

    //Reducimos el contador total de vehiculos
    s->numVehiculos--;

    return true; //Eliminacion exitante
}

//==========================================
//       CAPA DE LOGICA: MECANICOS
//==========================================

//Funcion auxiliar para verificar que no existen duplicados
Mecanico* buscarMecanicoPorCedula(SistemaTaller* s, const char* cedula) {
    // Recorrer el arreglo de mecanicos
    for (int i = 0; i < s->numMecanicos; i++) {
        // Comparamos la cedula actual 
        const char* p1 = s->mecanicos[i].cedula;
        const char* p2 = cedula;
        bool iguales = true;

        int j = 0;
        while (p1[j] != '\0' && p2[j] != '\0') {
            if (p1[j] != p2[j]) {
                iguales = false;
                break;
            }
            j++;
        }
        // Validamos que ambas hayan terminado al mismo tiempo en '\0'
        if (iguales && p1[j] == '\0' && p2[j] == '\0') {
            // Retornamos la direccion de memoria del mecánico encontrado
            return &(s->mecanicos[i]);
        }
    }
    // Si recorrimos todo y ninguna cedula coincide
    return nullptr;
}

Mecanico* buscarMecanicoPorID(SistemaTaller* s, int id) {
    for (int i = 0; i < s->numMecanicos; i++) {
        if (s->mecanicos[i].id == id) {
            // Retornar la direccion de memoria del mecanico encontrado
            return &(s->mecanicos[i]);
        }
    }
    // Si recorrimos todo y no esta
    return nullptr;
}

Mecanico* registrarMecanico(SistemaTaller* s, Mecanico datos) {
    // Validamos que los campos no estén vacíos y la especialidad sea válida
    if (datos.nombre[0] == '\0' || datos.cedula[0] == '\0') {
        return nullptr;
    }
    // Validar que la especialidad sea una de las 5 permitidas caracter por caracter
    const char* permitidas[5] = {"MOTOR", "FRENOS", "ELECTRICIDAD", "SUSPENSION", "GENERAL"};
    bool especialidadValida = false;

    for (int k = 0; k < 5; k++) {
        const char* p1 = datos.especialidad;
        const char* p2 = permitidas[k];
        bool iguales = true;

        int j = 0;
        while (p1[j] != '\0' && p2[j] != '\0') {
            if (p1[j] != p2[j]) {
                iguales = false;
                break;
            }
            j++;
        }

        if (iguales && p1[j] == '\0' && p2[j] == '\0') {
            especialidadValida = true;
            break;
        }
    }

    if (!especialidadValida) {
        return nullptr; // Especialidad no permitida
    }

    // Validar que la cédula no este duplicada
    if (buscarMecanicoPorCedula(s, datos.cedula) != nullptr) {
        return nullptr;
    }

    // 1. Verificar si el arreglo de mecanicos esta lleno
    if (s->numMecanicos >= s->capacidadMecanicos) {
        // 1.1 Duplicar la capacidad
        int nuevaCapacidad = s->capacidadMecanicos * 2;

        // 1.2 Crear un nuevo arreglo dinamico con la nueva capacidad
        Mecanico* nuevosMecanicos = new Mecanico[nuevaCapacidad];

        // 1.3 Copiar los datos del arreglo viejo al nuevo
        for (int i = 0; i < s->numMecanicos; i++) {
            nuevosMecanicos[i] = s->mecanicos[i];
        }

        // 1.4 Liberar la memoria del arreglo viejo
        delete[] s->mecanicos;

        // 1.5 Actualizar el puntero y la capacidad
        s->mecanicos = nuevosMecanicos;
        s->capacidadMecanicos = nuevaCapacidad;
    }

    // 2. Asignar un ID autoincremental al nuevo mecanico
    datos.id = s->siguienteIdMecanico;
    s->siguienteIdMecanico++;

    // 3. Guardar el nuevo mecanico en el arreglo
    s->mecanicos[s->numMecanicos] = datos;

    // 4. Incrementar el contador de mecanicos
    s->numMecanicos++;

    // 5. Retornar un puntero al nuevo mecanico
    return &(s->mecanicos[s->numMecanicos - 1]);

}

Mecanico** listarMecanicos(SistemaTaller* s, int* cantidad) {
    // 1. Verificamos si no hay mecanicos registrados
    if (s->numMecanicos == 0) {
        *cantidad = 0;
        return nullptr;
    }

    // 2. Definimos la cantidad encontrada como el total de mecanicos actuales
    *cantidad = s->numMecanicos;

    // 3. Reservamos memoria dinamica para un arreglo de punteros del tamanio exacto
    Mecanico** lista = new Mecanico*[s->numMecanicos];

    // 4. Llenamos el arreglo con las direcciones de memoria de cada mecanico existente
    for (int i = 0; i < s->numMecanicos; i++) {
        lista[i] = &(s->mecanicos[i]);
    }

    // Retornamos el arreglo de punteros (la capa visual lo liberara con delete[])
    return lista;
}

int contarOrdenesAbiertasMecanico(SistemaTaller* s, int idMecanico) {
    int contador = 0;
    const char* estadoBuscado = "ABIERTA";

    for (int i = 0; i < s->numOrdenes; i++) {
        if (s->ordenes[i].idMecanico == idMecanico) {
            // Comparamos caracter por caracter si el estado de la orden es "ABIERTA"
            const char* p1 = s->ordenes[i].estado;
            const char* p2 = estadoBuscado;
            bool iguales = true;

            int j = 0;
            while (p1[j] != '\0' && p2[j] != '\0') {
                if (p1[j] != p2[j]) {
                    iguales = false;
                    break;
                }
                j++;
            }

            if (iguales && p1[j] == '\0' && p2[j] == '\0') {
                contador++;
            }
        }
    }
    return contador;
}

//==========================================
//       CAPA DE LOGICA: REPUESTOS
//==========================================

Repuesto* buscarRepuestoPorID(SistemaTaller* s, int id) {
    for (int i = 0; i < s->numRepuestos; i++) {
        if (s->repuestos[i].id == id) {
            return &(s->repuestos[i]);
        }
    }
    return nullptr;
}

//==========================================
//       CAPA DE LOGICA: ORDENES
//==========================================

// Funcion auxiliar que verifica si un vehiculo ya tiene una orden abierta
OrdenServicio* buscarOrdenAbiertaVehiculo(SistemaTaller* s, int idVehiculo) {
    const char* estadoAbierta = "ABIERTA";

    for (int i = 0; i < s->numOrdenes; i++) {
        if (s->ordenes[i].idVehiculo == idVehiculo) {
            // Comparamos caracter por caracter si el estado es "ABIERTA"
            const char* p1 = s->ordenes[i].estado;
            const char* p2 = estadoAbierta;
            bool iguales = true;

            int j = 0;
            while (p1[j] != '\0' && p2[j] != '\0') {
                if (p1[j] != p2[j]) {
                    iguales = false;
                    break;
                }
                j++;
            }

            if (iguales && p1[j] == '\0' && p2[j] == '\0') {
                return &(s->ordenes[i]); // Retornamos la orden abierta encontrada
            }
        }
    }
    return nullptr; // No tiene ordenes abiertas
}

OrdenServicio* abrirOrden (SistemaTaller* s, int idVehiculo, int idMecanico, const char* fechaApertura, const char* descripcionFalla, int kilometrajeIngreso) {
    // Validamos que la fecha y la descripcion de la falla no esten vacias
    if (fechaApertura[0] == '\0' || descripcionFalla[0] == '\0') {
        return nullptr;
    }
    // Verificamos la existencia de vehículos y mecánicos
    Vehiculo* v = buscarVehiculoPorID(s, idVehiculo);
    Mecanico* m = buscarMecanicoPorID(s, idMecanico);
    if (v == nullptr || m == nullptr) {
        return nullptr;
    }

    if (buscarOrdenAbiertaVehiculo(s, idVehiculo) != nullptr) {
        return nullptr;
    }
    
    if (contarOrdenesAbiertasMecanico(s, idMecanico) >= 3) {
        return nullptr;
    }

    if (kilometrajeIngreso < v->kilometraje) {
        return nullptr;
    }

    // Redimensionar el arreglo de ordenes del sistema si esta lleno
    if (s->numOrdenes >= s->capacidadOrdenes) {
        int nuevaCapacidad = s->capacidadOrdenes * 2;
        OrdenServicio* nuevasOrdenes = new OrdenServicio[nuevaCapacidad];

        for (int i = 0; i < s->numOrdenes; i++) {
            nuevasOrdenes[i] = s->ordenes[i];
        }

        delete[] s->ordenes;
        s->ordenes = nuevasOrdenes;
        s->capacidadOrdenes = nuevaCapacidad;
    }
    // Actualizamos el kilometraje del vehiculo al kilometraje de ingreso
    v->kilometraje = kilometrajeIngreso;

    // Preparamos la nueva estructura OrdenServicio
    OrdenServicio nueva;
    nueva.id = s->siguienteIdOrden;
    s->siguienteIdOrden++;

    nueva.idVehiculo = idVehiculo;
    nueva.idMecanico = idMecanico;
    nueva.kilometrajeIngreso = kilometrajeIngreso;
    nueva.manoObra = 0.0f; // inicialmente, mano de obra 0

    // Copiamos fechaApertura caracter por caracter
    int i = 0;
    while (fechaApertura[i] != '\0' && i < 10) {
        nueva.fechaApertura[i] = fechaApertura[i];
        i++;
    }
    nueva.fechaApertura[i] = '\0';

    // fechaCierre queda vacia mientras la orden este ABIERTA
    nueva.fechaCierre[0] = '\0';

    // Copiamos el estado inicial "ABIERTA"
    const char* estInicial = "ABIERTA";
    i = 0;
    while (estInicial[i] != '\0' && i < 11) {
        nueva.estado[i] = estInicial[i];
        i++;
    }
    nueva.estado[i] = '\0';

    // Copiamos la descripcion de la falla
    i = 0;
    while (descripcionFalla[i] != '\0' && i < 199) {
        nueva.descripcionFalla[i] = descripcionFalla[i];
        i++;
    }
    nueva.descripcionFalla[i] = '\0';

    // Cada orden nueva reserva su arreglo de detalles con capacidad inicial 2
    nueva.numDetalles = 0;
    nueva.capacidadDetalles = 2;
    nueva.detalles = new DetalleOrden[nueva.capacidadDetalles];

    // Guardamos la orden en el arreglo del taller
    s->ordenes[s->numOrdenes] = nueva;
    s->numOrdenes++;

    return &(s->ordenes[s->numOrdenes - 1]);
}

OrdenServicio* buscarOrdenPorID(SistemaTaller* s, int id) {
    for (int i = 0; i < s->numOrdenes; i++) {
        if (s->ordenes[i].id == id) {
            return &(s->ordenes[i]);
        }
    }
    return nullptr;
}

bool agregarRepuestoAOrden(SistemaTaller* s, int idOrden, int idRepuesto, int cantidad) {
    // La cantidad debe ser mayor a 0
    if (cantidad <= 0) {
        return false;
    }

    OrdenServicio* orden = buscarOrdenPorID(s, idOrden);
    if (orden == nullptr) {
        return false;
    }

    // Verificamos caracter por caracter que la orden este abierta
    const char* p1 = orden->estado;
    const char* p2 = "ABIERTA";
    bool estaAbierta = true;
    int j = 0;
    while (p1[j] != '\0' && p2[j] != '\0') {
        if (p1[j] != p2[j]) {
            estaAbierta = false;
            break;
        }
        j++;
    }
    if (!estaAbierta || p1[j] != '\0' || p2[j] != '\0') {
        return false; // No se puede modificar si no esta abierta
    }

    // Verificamos que el repuesto exista y tenga stock suficiente
    Repuesto* rep = buscarRepuestoPorID(s, idRepuesto);
    if (rep == nullptr || rep->stock < cantidad) {
        return false;
    }

    // Descontamos las unidades del stock inmediatamente 
    rep->stock -= cantidad;

    // Buscamos si el repuesto ya existe en alguna linea de detalle de esta orden
    for (int i = 0; i < orden->numDetalles; i++) {
        if (orden->detalles[i].idRepuesto == idRepuesto) {
            // Si ya esta en la orden, se acumula en la linea existente
            orden->detalles[i].cantidad += cantidad;
            return true;
        }
    }

    // Si es un repuesto nuevo en la orden, verificamos si hay que redimensionar detalles
    if (orden->numDetalles >= orden->capacidadDetalles) {
        int nuevaCapacidad = orden->capacidadDetalles * 2;
        DetalleOrden* nuevosDetalles = new DetalleOrden[nuevaCapacidad];

        for (int i = 0; i < orden->numDetalles; i++) {
            nuevosDetalles[i] = orden->detalles[i];
        }

        delete[] orden->detalles;
        orden->detalles = nuevosDetalles;
        orden->capacidadDetalles = nuevaCapacidad;
    }

    // Creamos la nueva linea guardando el precio vigente en este momento (Precio Historico)
    orden->detalles[orden->numDetalles].idRepuesto = idRepuesto;
    orden->detalles[orden->numDetalles].cantidad = cantidad;
    orden->detalles[orden->numDetalles].precioUnitario = rep->precio;
    orden->numDetalles++;

    return true;
}

bool quitarRepuestoDeOrden(SistemaTaller* s, int idOrden, int idRepuesto) {
    OrdenServicio* orden = buscarOrdenPorID(s, idOrden);
    if (orden == nullptr) {
        return false;
    }

    // Validar que la orden este abierta
    const char* p1 = orden->estado;
    const char* p2 = "ABIERTA";
    bool estaAbierta = true;
    int j = 0;
    while (p1[j] != '\0' && p2[j] != '\0') {
        if (p1[j] != p2[j]) {
            estaAbierta = false;
            break;
        }
        j++;
    }
    if (!estaAbierta || p1[j] != '\0' || p2[j] != '\0') {
        return false;
    }

    // Buscar el repuesto dentro del arreglo de detalles de la orden
    int indiceEncontrado = -1;
    for (int i = 0; i < orden->numDetalles; i++) {
        if (orden->detalles[i].idRepuesto == idRepuesto) {
            indiceEncontrado = i;
            break;
        }
    }

    if (indiceEncontrado == -1) {
        return false; // El repuesto no estaba en la orden
    }

    // Devolver las unidades al inventario de repuestos
    Repuesto* rep = buscarRepuestoPorID(s, idRepuesto);
    if (rep != nullptr) {
        rep->stock += orden->detalles[indiceEncontrado].cantidad;
    }

    // Desplazar los detalles siguientes hacia la izquierda para eliminar la linea
    for (int i = indiceEncontrado; i < orden->numDetalles - 1; i++) {
        orden->detalles[i] = orden->detalles[i + 1];
    }

    orden->numDetalles--;
    return true;
}

bool cerrarOrden(SistemaTaller* s, int idOrden, float manoObra, const char* fechaCierre) {
    // Validar mano de obra >= 0 y fecha no vacia
    if (manoObra < 0.0f || fechaCierre[0] == '\0') {
        return false;
    }

    OrdenServicio* orden = buscarOrdenPorID(s, idOrden);
    if (orden == nullptr) {
        return false;
    }

    // Validar que este abierta
    const char* p1 = orden->estado;
    const char* p2 = "ABIERTA";
    bool estaAbierta = true;
    int j = 0;
    while (p1[j] != '\0' && p2[j] != '\0') {
        if (p1[j] != p2[j]) {
            estaAbierta = false;
            break;
        }
        j++;
    }
    if (!estaAbierta || p1[j] != '\0' || p2[j] != '\0') {
        return false;
    }

    // Debe quedar con al menos una linea de detalle o mano de obra > 0
    if (orden->numDetalles == 0 && manoObra <= 0.0f) {
        return false;
    }

    // Actualizamos mano de obra, fecha de cierre y cambiamos estado a cerrada
    orden->manoObra = manoObra;

    int i = 0;
    while (fechaCierre[i] != '\0' && i < 10) {
        orden->fechaCierre[i] = fechaCierre[i];
        i++;
    }
    orden->fechaCierre[i] = '\0';

    const char* estCerrada = "CERRADA";
    i = 0;
    while (estCerrada[i] != '\0' && i < 11) {
        orden->estado[i] = estCerrada[i];
        i++;
    }
    orden->estado[i] = '\0';

    return true;
}

bool anularOrden (SistemaTaller* s, int idOrden) {
    OrdenServicio* orden = buscarOrdenPorID(s, idOrden);
    if (orden == nullptr) {
        return false;
    }

    // Verificamos si ya esta anulada
    const char* p1 = orden->estado;
    const char* p2 = "ANULADA";
    bool yaAnulada = true;
    int j = 0;
    while (p1[j] != '\0' && p2[j] != '\0') {
        if (p1[j] != p2[j]) {
            yaAnulada = false;
            break;
        }
        j++;
    }
    if (yaAnulada && p1[j] == '\0' && p2 [j] == '\0') {
        return false; // Ya estaba anulada
    }

    // Devolvemos al inventario todas las unidades de cada linea de detalle
    for (int i = 0; i < orden->numDetalles; i++) {
        Repuesto* rep = buscarRepuestoPorID(s, orden->detalles[i].idRepuesto);
        if (rep != nullptr) {
            rep->stock += orden->detalles[i].cantidad;
        }
    }
    // Cambiamos el estado a "ANULADA"
    const char* estAnulada = "ANULADA";
    int i = 0;
    while (estAnulada[i] != '\0' && i < 11) {
        orden->estado[i] = estAnulada[i];
        i++;
    }
    orden->estado[i] = '\0';

    return true;
}

float calcularTotalOrden(OrdenServicio* orden) {
    if (orden == nullptr) {
        return 0.0f;
    }

    float sumaRepuestos = 0.0f;
    for (int i = 0; i < orden->numDetalles; i++) {
        sumaRepuestos += (orden->detalles[i].cantidad * orden->detalles[i].precioUnitario);
    }

    return orden->manoObra + sumaRepuestos;
}

//=========================
//      CAPA VISUAL
//=========================

void menuPrincipal () {
    cout << "╔═══════════════════════════════════════════╗" << endl;
    cout << "║    SISTEMA DE GESTIÓN DE TALLER           ║" << endl;
    cout << "║    Taller: Autoservicio El Wikiti         ║" << endl;
    cout << "╠═══════════════════════════════════════════╣" << endl;
    cout << "║ 1. Gestión de Vehículos                   ║" << endl;
    cout << "║ 2. Gestión de Mecánicos                   ║" << endl;
    cout << "║ 3. Inventario de Repuestos                ║" << endl;
    cout << "║ 4. Órdenes de Servicio                    ║" << endl;
    cout << "║ 0. Salir                                  ║" << endl;
    cout << "╚═══════════════════════════════════════════╝" << endl;
}

void menuVehículos () {
    cout << "╔═══════════════════════════════════════════╗" << endl;
    cout << "║            GESTIÓN DE VEHÍCULOS           ║" << endl;
    cout << "╠═══════════════════════════════════════════╣" << endl;
    cout << "║ 1. Registrar vehículo                     ║" << endl;
    cout << "║ 2. Buscar vehículo                        ║" << endl;
    cout << "║ 3. Actualizar vehículo                    ║" << endl;
    cout << "║ 4. Eliminar vehículo                      ║" << endl;
    cout << "║ 5. Listar vehículos                       ║" << endl;
    cout << "║ 6. Mostrar vehículos                      ║" << endl;
    cout << "║ 7. Mostrar lista de vehículos             ║" << endl;
    cout << "║ 0. Volver al menú principal               ║" << endl;
    cout << "╚═══════════════════════════════════════════╝" << endl;
}

void mostrarVehiculo(Vehiculo v){
    cout << "----------------------------------------" << endl;
    cout << " ID          : " << v.id << endl;
    cout << " Placa       : " << v.placa << endl;
    cout << " Marca/Modelo: " << v.marca << " " << v.modelo << " (" << v.anio << ")" << endl;
    cout << " Kilometraje : " << v.kilometraje << " km" << endl;
    cout << " Propietario : " << v.nombrePropietario << "(CI:" << v.cedulaPropietario << ")" << endl;
    cout << " Telefono    : " << v.telefonoPropietario << endl;
    cout << " Registro    : " << v.fechaRegistro << endl;
}

void mostrarListaVehiculos(Vehiculo** vehiculos, int cantidad){
    if (vehiculos == nullptr || cantidad <= 0){
        cout << "\n[!] No hay vehiculos para mostrar." << endl;
        return;
    }
    cout << "\n=== LISTADO DE VEHICULOS (" << cantidad << ") ===" << endl;
    for (int i = 0; i < cantidad; i ++){
        mostrarVehiculo(*(vehiculos[i]));
    }
    cout << "----------------------------------------" << endl;
}

void mostrarVehiculoIndividual(SistemaTaller* s){
    limpiar();
    cout << "=== MOSTRAR VEHICULO ===" << endl;
    int id;
    cout << "Ingrese el ID del vehiculo: ";
    cin >> id;

    Vehiculo* v = buscarVehiculoPorID(s, id);
    if (v != nullptr){
        cout << "\n--- DATOS DEL VEHICULO ---" << endl;
        cout << "ID          : " << v->id << endl;
        cout << "Placa       : " << v->placa << endl;
        cout << "Marca/Modelo: " << v->marca << " " << v->modelo << " (" << v->anio << ")" << endl;
        cout << "Kilometraje : " << v->kilometraje << "km" << endl;
        cout << "Propietario : " << v->nombrePropietario << " (C.I: " << v->cedulaPropietario << ")" << endl;
        cout << "Telefono    : " << v->telefonoPropietario << endl;
    } else{
        cout << "\nNo se encontro ningun vehiculo con el ID " << id << "." << endl;
    }
    pausarylimpiar();
}

void mostrarListaVehiculosCompleta(SistemaTaller* s){
    limpiar();
    cout << "=== LISTA DE VEHICULOS ===" << endl;

    int cantidad = 0;
    Vehiculo** lista = listarVehiculos(s, &cantidad);

    if (lista != nullptr && cantidad > 0){
        for (int i = 0; i < cantidad; i++){
            Vehiculo* v = lista[i];
            cout << "----------------------------------------" << endl;
            cout << "ID          : " << v->id << endl;
            cout << "Placa       : " << v->placa << endl;
            cout << "Vehículo    : " << v->marca << " " << v->modelo << " (" << v->anio << ")" << endl;
            cout << "Kilometraje : " << v->kilometraje << " km" << endl;
            cout << "Propietario : " << v->nombrePropietario << endl;
        }
        cout << "----------------------------------------" << endl;
        cout << "Total de vehiculos: " << cantidad << endl;

        delete[] lista;
    } else{
        cout << "\n No hay vahiculos registrados en el sistema." << endl;
        pausarylimpiar();
    }
}

void uiRegistrarVehiculo(SistemaTaller* s){
    limpiar();
    cout << "=== REGISTRAR NUEVO VEHICULO ===" << endl;
    Vehiculo nuevo;

    cout << "Placa: ";
    cin >> nuevo.placa;

    cout << "Marca: ";
    cin >> nuevo.marca;
    
    cout << "Modelo: ";
    cin >> nuevo.modelo;
    
    cout << "Año: ";
    cin >> nuevo.anio;
    
    cout << "Kilometraje: ";
    cin >> nuevo.kilometraje;
    
    // Limpiamos el salto de linea antes de usar getline
    cin.ignore(10000, '\n');
    
    cout << "Nombre del Propietario: ";
    cin.getline(nuevo.nombrePropietario, 100);

    cout << "Cedula del Propietario: ";
    cin >> nuevo.cedulaPropietario;

    cout << "Telefono del Propietario: ";
    cin >> nuevo.telefonoPropietario;

    cout << "Fecha de Registro (YYYY-MM-DD): ";
    cin >> nuevo.fechaRegistro;

    Vehiculo* creado = agregarVehiculo(s, nuevo);
    if (creado != nullptr) {
        cout << "\nVehiculo registrado exitosamente con ID asignado!" << endl;
    } else {
        cout << "\nERROR: No se pudo registrar el vehiculo." << endl;
    }
    pausarylimpiar();
}

void uiBuscarVehiculo(SistemaTaller* s) {
    limpiar();
    cout << "=== BUSCAR VEHICULO ===" << endl;
    int op = 0;
    cout << "Seleccione el modo de busqueda: " << endl;
    cout << "1. Buscar por ID" << endl;
    cout << "2. Buscar por Placa" << endl;
    cout << "3. Buscar por Propietario" << endl;
    cin >> op;

    switch (op) {
        case 1: {
            int idBuscado;
            cout << "Ingrese un ID registrado: ";
            cin >> idBuscado;
    
            Vehiculo* ubicado = buscarVehiculoPorID(s, idBuscado);
            if (ubicado != nullptr) {
                cout << "\nVehiculo ubicado con ID: " << ubicado->id << endl;
            } else {
                cout << "\nVehiculo no registrado (sin coincidencias)." << endl;
            }
            break; 
        }
        case 2: {
            char placaBuscada[10];
            cout << "Ingrese la placa exacta: ";
            cin >> placaBuscada;

            Vehiculo* ubicado = buscarVehiculoPorPlaca(s, placaBuscada);
            if (ubicado != nullptr){
                mostrarVehiculo(*ubicado);
            } else{
                cout << "\nBad milk. No se encontro ningun vehiculo con esa placa." << endl;
            }
            break;
        }
        case 3: {
            char subcadena[100];
            cin.ignore(10000, '\n');
            cout << "Ingrese parte del nombre del propietario: ";
            cin.getline(subcadena, 100);

            int cantidad = 0;
            Vehiculo** resultados = buscarVehiculosPorPropietario(s, subcadena, &cantidad);
            if (resultados != nullptr){
                mostrarListaVehiculos(resultados, cantidad);
                delete[] resultados;
            } else{
                cout << "\nNo se encontraron vehiculos con ese propietario." << endl;
            }
            break;
        }
        default:{
            cout << "\nOpcion validan't." << endl;
            break;
        }
    } 
    pausarylimpiar();
}

void uiActualizarVehiculos(SistemaTaller* s){
    limpiar();
    cout << "=== ACTUALIZAR VEHICULO ===" << endl;
    int id;
    cout << "Ingrese el ID del vehiculo a actualizar: ";
    cin >> id;

    Vehiculo* v = buscarVehiculoPorID(s, id);
    if (v == nullptr){
        cout << "\nMi loco no esxiste un vehiculo registrado con el ID " << id << "." << endl;
        pausarylimpiar();
        return;
    }

    cout << "\nVehiculo actual encontrado: " << endl;
    mostrarVehiculo(*v);

    Vehiculo nuevosDatos;
    cout << "\nIngrese los nuevos datos: " << endl;
    cout << "Nueva Marca: ";
    cin >> nuevosDatos.marca;
    cout << "Nuevo Modelo: ";
    cin >> nuevosDatos.modelo;
    cout << "Nuevo anio (1950 - 2027): ";
    cin >> nuevosDatos.anio;

    cin.ignore(10000, '\n');
    cout << "Nuevo Nombre del Propietario: ";
    cin.getline(nuevosDatos.nombrePropietario, 100);

    for (int i = 0; i < 10; i++) nuevosDatos.placa[i] = v->placa[i];

    if (actualizarVehiculo(s, id, nuevosDatos)){
        cout << "\nVehiculo actualizado sasticfactoriamente." << endl;
    } else{
        cout << "\nError al actualizar (verifique el anio)." << endl;
    }
    pausarylimpiar();
}

void uiEliminarVehiculo(SistemaTaller* s){
    limpiar();
    cout << "=== ELIMINAR VEHICULO ===" << endl;
    int id;
    cout << "Ingrese la ID del Vehiculo a eliminar: ";
    cin >> id;

    if (eliminarVehiculo(s, id)){
        cout << "\nVehiculo eliminado fisicamente del sistema." << endl;
    } else{
        cout << "\nNo se pudo eliminar (ID no existe o el vehiculo tiene ordenes de servicio asociadas)." << endl;
    }
    pausarylimpiar();
}

void uiListarVehiculos(SistemaTaller* s){
    limpiar();
    int cantidad = 0;
    Vehiculo** lista = listarVehiculos(s, &cantidad);

    if (lista != nullptr){
        mostrarListaVehiculos(lista, cantidad);
        delete[] lista;
    } else {
        cout << "\nNo hay vehiculos registrados en el sistema." << endl;
    }
}

void menuMecánicos () {
    cout << "╔═══════════════════════════════════════════╗" << endl;
    cout << "║            GESTIÓN DE MECÁNICOS           ║" << endl;
    cout << "╠═══════════════════════════════════════════╣" << endl;
    cout << "║ 1. Registrar mecánico                     ║" << endl;
    cout << "║ 2. Buscar por ID                          ║" << endl;
    cout << "║ 3. Listar todos los mecánicos             ║" << endl;
    cout << "║ 0. Volver al menú principal               ║" << endl;
    cout << "╚═══════════════════════════════════════════╝" << endl;
}

void uiRegistrarMecanico(SistemaTaller* s){
    limpiar();
    cout << "=== REGISTRAR NUEVO MECANICO ===" << endl;
    Mecanico nuevo;

    // Limpiamos el salto de linea antes de usar getline
    cin.ignore(10000, '\n');

    cout << "Nombre del Mecanico: ";
    cin.getline(nuevo.nombre, 100);

    cout << "Cedula: ";
    cin >> nuevo.cedula;
    
    cout << "Especialidad (MOTOR, FRENOS, ELECTRICIDAD, SUSPENSION, GENERAL): ";
    cin >> nuevo.especialidad;
    
    cout << "Fecha de Ingreso (YYYY-MM-DD): ";
    cin >> nuevo.fechaIngreso;
       
    cin.ignore(10000, '\n');

    Mecanico* creado = registrarMecanico(s, nuevo);

    if (creado != nullptr) {
    cout << "\nMecanico registrado exitosamente con ID asignado!" << endl;
    } else {
        cout << "\nERROR: No se pudo registrar el mecanico." << endl;
    }
    pausarylimpiar();
}

void uiBuscarMecanicoID(SistemaTaller* s) {
    limpiar();
    cout << "=== BUSCAR MECANICO POR ID ===" << endl;
    int idBuscado;

    cout << "Ingrese un ID registrado: ";
    cin >> idBuscado;

    // Limpiamos el búfer despues de cin para evitar errores
    cin.ignore(10000, '\n');
    
    Mecanico* ubicado = buscarMecanicoPorID(s, idBuscado);
    if (ubicado != nullptr) {
    cout << "\n--- MECANICO ENCONTRADO ---" << endl;
    cout << "ID          : " << ubicado->id << endl;
    cout << "Nombre      : " << ubicado->nombre << endl;
    cout << "Cedula      : " << ubicado->cedula << endl;
    cout << "Especialidad: " << ubicado->especialidad << endl;
    cout << "Ordenes Ab. : " << contarOrdenesAbiertasMecanico(s, ubicado->id) << endl;
    } else {
        cout << "\nMecanico no registrado (sin coincidencias)." << endl;
    }
    pausarylimpiar();
}

void uiListarMecanicos(SistemaTaller* s){
    limpiar();
    int cantidad = 0;
    Mecanico** lista = listarMecanicos(s, &cantidad);

    if (lista != nullptr){
        cout << "\n=== LISTADO DE MECANICOS (" << cantidad << ") ===" << endl;
        for (int i = 0; i < cantidad; i++){
            Mecanico* m = lista[i];
            cout << "----------------------------------------" << endl;
            cout << "ID          : " << m->id << endl;
            cout << "Nombre      : " << m->nombre << endl;
            cout << "Cedula      : " << m->cedula << endl;
            cout << "Especialidad: " << m->especialidad << endl;
            cout << "Ordenes Ab. : " << contarOrdenesAbiertasMecanico(s, m->id) << endl;
        }
        cout << "----------------------------------------" << endl;
        delete[] lista;
    } else{
        cout << "\nNo hay mecanicos registrados." << endl;
    }
    pausarylimpiar();
}

void menuRepuestos () {
    cout << "╔═══════════════════════════════════════════╗" << endl;
    cout << "║          INVENTARIO DE REPUESTOS          ║" << endl;
    cout << "╠═══════════════════════════════════════════╣" << endl;
    cout << "║ 1. Registrar repuesto                     ║" << endl;
    cout << "║ 2. Buscar por ID                          ║" << endl;
    cout << "║ 3. Registrar entrada de mercancía         ║" << endl;
    cout << "║ 4. Listar todos los repuestos             ║" << endl;
    cout << "║ 5. Cambiar precio                         ║" << endl;
    cout << "║ 6. Eliminar repuesto                      ║" << endl;
    cout << "║ 0. Volver al menú principal               ║" << endl;
    cout << "╚═══════════════════════════════════════════╝" << endl;
}

void menuÓrdenes () {
    cout << "╔═══════════════════════════════════════════╗" << endl;
    cout << "║            ÓRDENES DE SERVICIO            ║" << endl;
    cout << "╠═══════════════════════════════════════════╣" << endl;
    cout << "║ 1. Abrir orden                            ║" << endl;
    cout << "║ 2. Agregar repuesto a orden               ║" << endl;
    cout << "║ 3. Quitar repuesto de orden               ║" << endl;
    cout << "║ 4. Cerrar orden                           ║" << endl;
    cout << "║ 5. Anular orden                           ║" << endl;
    cout << "║ 6. Ver detalle de orden                   ║" << endl;
    cout << "║ 7. Buscar por ID                          ║" << endl;
    cout << "║ 8. Listar por estado/vehículo             ║" << endl;
    cout << "║ 0. Volver al menú principal               ║" << endl;
    cout << "╚═══════════════════════════════════════════╝" << endl;
}

int main () {
    SetConsoleOutputCP(65001);
    

    //Iniciamos el sistema
    SistemaTaller miTaller;
    inicializarSistema(&miTaller, "Autoservicio El Wikiti", "J-12345678-9", "Maracaibo", "0414-1234567");
    
    int opcion = 0;
    int opcion2 = 0;

    do {
        limpiar();
        menuPrincipal();
        cin >> opcion;

        switch (opcion) {
            case 1: {
                do{
                    limpiar();
                    menuVehículos();
                    cin >> opcion2;
                
                    switch (opcion2) {
                        case 1: {
                            uiRegistrarVehiculo(&miTaller);
                            break;
                        } case 2: {
                            uiBuscarVehiculo(&miTaller);
                            break;
                        } case 3: {
                            uiActualizarVehiculos(&miTaller);
                            break;
                        } case 4: {
                            uiEliminarVehiculo(&miTaller);
                            break;  
                        } case 5: {
                            uiListarVehiculos(&miTaller);
                            break;
                        } case 6: {
                            mostrarVehiculoIndividual(&miTaller);
                            break;
                        } case 7: {
                            mostrarListaVehiculosCompleta(&miTaller);
                            break;
                        } case 0: {
                            break;
                        }
                        default: {
                            cout << "Opcion invalida." << endl;
                            pausarylimpiar();
                            break;
                        }
                    }
                }while(opcion2 != 0);
                limpiar();
                break;
            }case 2: {
                do{
                    limpiar();
                    menuMecánicos();
                    cin >> opcion2;
                
                    switch (opcion2) {
                        case 1: {
                            uiRegistrarMecanico(&miTaller);
                            break;
                        } case 2: {
                            uiBuscarMecanicoID(&miTaller);
                            break;
                        } case 3: {
                            uiListarMecanicos(&miTaller);
                            break;
                        } case 0: {
                            break;  
                        }
                        default: {
                            cout << "Opcion invalida." << endl;
                            pausarylimpiar();
                            break;
                        }
                    }
                }while(opcion2 != 0);
                limpiar();
                break;
            

            } case 3: {
                limpiar();
                menuRepuestos();
                cin >> opcion;
                limpiar();
                break;

            } case 4: {
                limpiar();
                menuÓrdenes();
                cin >> opcion;
                limpiar();
                break;

            } case 0: {
                cout << "Cerrando programa. Hasta pronto..." << endl;
                break;
            } default:
                pausarylimpiar();
                cout << "Escoja una opción válida (0-4).\n" << endl;
            break;
        }

    } while (opcion != 0);

    delete[] miTaller.vehiculos;
    delete[] miTaller.mecanicos;
    delete[] miTaller.repuestos;
    delete[] miTaller.ordenes;
    
return 0;
}