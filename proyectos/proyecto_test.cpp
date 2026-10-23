#include <iostream>
#include <windows.h>
using namespace std;


//structs
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
    int  id;
    char nombre[100];
    char cedula[20];
    char especialidad[20];
    char fechaIngreso[11]; //yyyy-mm-dd
};

struct Repuesto {
    int     id;
    char    codigo[20];
    char    nombre[100];
    char    marca[50];
    float   precio;
    int     stock;
    int     stockMinimo;
    char    fechaRegistro [11]; //yyyy-mm-dd
};

struct DetalleOrden {
    int     idRepuesto;
    int     cantidad;
    float   precioUnitario;
};

struct OrdenServicio {
    int     id;
    int     idVehiculo;
    int     idMecanico;
    char    fechaApertura[11]; //yyyy-mm-dd
    char    fechaCierre[11]; //vacia mientras la orden esté abierta
    char    estado[12]; // Abierta, Cerrada, anulada
    char    descripcionFalla[200];
    int     kilometrajeIngreso;
    float   manoObra;

    
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

bool compararCadenasExactas(const char* s1, const char* s2){
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0'){
        if (s1[i] != s2[i]) return false;
        i++;
    }
    return s1[i] == '\0' && s2[i] == '\0';
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

bool esCancelar(const char* texto){
    const char* palabra = "cancelar";
    int i = 0;
    while (texto[i] != '\0' && palabra[i] != '\0'){
        if (aMinuscula(texto[i]) != palabra[i]) return false;
        i++;
    }
    return texto[i] == '\0' && palabra[i] =='\0';
}

bool validarPlaca(const char* placa){
    int len = 0;
    while (placa[len] != '\0'){
        char c = placa[len];
        bool esLetra = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        bool esDigito = (c >= '0' && c <= '9');
        if (!esLetra && !esDigito) return false;
        len++;
    }
    return (len >= 6 && len <= 7);
}

bool validarCedula(const char* cedula){
    if (cedula[0] == '\0') return false;
    int i = 0;
    if ((cedula[0] == 'V' || cedula[0] == 'v' ||
         cedula[0] == 'E' || cedula[0] == 'e' ||
         cedula[0] == 'J' || cedula[0] == 'j') && cedula[1] == '-'){
        i=2;
    }
    if (cedula[i] =='\0') return false;
    while (cedula[i] != '\0'){
        if (cedula[i] < '0' || cedula [i] > '9') return false;
        i++;
    }
    return true;
}

bool validarFecha(const char* f){
    int len = 0;
    while (f[len] != '\0') len++;
    if (len != 10) return false;
    if (f[4] != '-' || f[7] != '-') return false;

    for (int i = 0; i < 10; i++){
        if (i == 4 || i == 7)continue;
        if (f[i] < '0' || f[i] > '9') return false;
    }

    int anio = (f[0] - '0') * 1000 + (f[1] - '0') * 100 + (f[2] - '0') * 10 + (f[3] - '0');
    int mes  = (f[5] - '0') * 10 + (f[6] - '0');
    int dia  = (f[8] - '0') * 10 + (f[9] - '0');
    
    if (anio < 1950 || anio > 2100) return false;
    if (mes < 1 || mes > 12) return false;
    if (dia < 1 || dia > 31) return false;
    return true;
}

bool pedirConfirmacion(const char* mensaje){
    char resp;
    cout << mensaje << " (S/N): ";
    cin >> resp;
    return (resp == 'S' || resp == 's');
}

//por si acaso
Vehiculo* buscarVehiculoPorPlaca(SistemaTaller* s, const char* placa);
Vehiculo* buscarVehiculoPorID(SistemaTaller* s, int id);
Mecanico* buscarMecanicoPorID(SistemaTaller* s, int id);
Repuesto* buscarRepuestoPorID(SistemaTaller* s, int id);
Repuesto* buscarRepuestoPorCodigo(SistemaTaller* s, const char* codigo);
OrdenServicio* buscarOrdenPorID(SistemaTaller* s, int id);

//==========================================
//       CAPA DE LOGICA: SISTEMA
//==========================================

//Inializar el sistema (reserva capacidad 4 para entidades y 2 para detalles)
void inicializarSistema(SistemaTaller* s, const char* nombre, const char* rif, const char* direccion, const char* telefono){
    
    //1. Copiar datos del taller caracter a caracter
    int i = 0;
    for(i = 0; i < 99 && nombre[i] != '\0'; i++) s->taller.nombre[i] = nombre[i];
    s->taller.nombre[i] = '\0';

    for(i = 0; i < 19 && rif[i] != '\0'; i++) s->taller.rif[i] = rif[i];
    s->taller.rif[i] = '\0';

    for(i = 0; i < 199 && direccion[i] != '\0'; i++) s->taller.direccion[i] = direccion[i];
    s->taller.direccion[i] = '\0';

    for(i = 0; i < 19 && telefono[i] != '\0'; i++) s->taller.telefono[i] = telefono[i];
    s->taller.telefono[i] = '\0';

    //2. Poner contadores en cero
    s->numVehiculos = 0;
    s->numMecanicos = 0;
    s->numRepuestos = 0;
    s->numOrdenes   = 0;

    //3. Capacidades iniciales en en 4
    s->capacidadVehiculos = 4;
    s->capacidadMecanicos = 4;
    s->capacidadRepuestos = 4;
    s->capacidadOrdenes   = 4;

    //4. Reservar memoria dinamica con new
    s->vehiculos = new Vehiculo[s->capacidadVehiculos];
    s->mecanicos = new Mecanico[s->capacidadMecanicos];
    s->repuestos = new Repuesto[s->capacidadRepuestos];
    s->ordenes   = new OrdenServicio[s->capacidadOrdenes];

    //5. IDs en 1
    s->siguienteIdVehiculo = 1;
    s->siguienteIdMecanico = 1;
    s->siguienteIdRepuesto = 1;
    s->siguienteIdOrden    = 1;
}

void liberarSistema(SistemaTaller* s) {
    if (s == nullptr) return;

    // Liberar primero la memoria anidada (arreglo de detalles de cada orden)
    for (int i = 0; i < s->numOrdenes; i++) {
        if (s->ordenes[i].detalles != nullptr) {
            delete[] s->ordenes[i].detalles;
            s->ordenes[i].detalles = nullptr;
        }
    }

    // Liberar los arreglos principales de entidades
    delete[] s->vehiculos; s->vehiculos = nullptr;
    delete[] s->mecanicos; s->mecanicos = nullptr;
    delete[] s->repuestos; s->repuestos = nullptr;
    delete[] s->ordenes;   s->ordenes   = nullptr;
}

//==========================================
//       CAPA DE LOGICA: VEHICULOS
//==========================================

Vehiculo* agregarVehiculo(SistemaTaller* s, Vehiculo datos){
    // Validamos que los textos obligatorios no esten vacios
    if (datos.placa[0] == '\0' || datos.marca[0] == '\0' || datos.modelo[0] == '\0' ||
        datos.nombrePropietario[0] == '\0' || datos.cedulaPropietario[0] == '\0') {
        return nullptr;
    }

    // Validamos los formatos de placa, cedula y fecha
    if (!validarPlaca(datos.placa) || !validarCedula(datos.cedulaPropietario) || !validarFecha(datos.fechaRegistro)) {
        return nullptr;
    }

    //Validamos anio (1950 - 2027) y kilometraje (>=0)
    if(datos.anio < 1950 || datos.anio > 2027 || datos.kilometraje < 0) return nullptr;
    
    //Validar que la placa no este duplicada
    if(buscarVehiculoPorPlaca(s, datos.placa) != nullptr) return nullptr;
    
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
        if (s->vehiculos[i].id == id) return &(s->vehiculos[i]);
    }
    return nullptr;
}

Vehiculo* buscarVehiculoPorPlaca(SistemaTaller* s, const char* placa){
    for(int i = 0; i < s->numVehiculos; i++){
        if (compararCadenasExactas(s->vehiculos[i].placa, placa)) return &(s->vehiculos[i]);
    }
    return nullptr;
}

Vehiculo** buscarVehiculosPorPropietario(SistemaTaller* s, const char* subcadena, int* cantidad){
    int encontrados = 0;
    for(int i = 0; i < s->numVehiculos; i++){
        if (compararSubcadenaIgnoreCase(s->vehiculos[i].nombrePropietario, subcadena)) encontrados++;   
    }
    *cantidad = 0;
    if (encontrados == 0){
        return nullptr;
    }

    //2.Reservamos memoria dinamica para un arreglo de punteros del tamanio exacto
    Vehiculo** resultados = new Vehiculo*[encontrados];

    //3.Volvemos a recorrer para llenar el arreglo con las direcciones de memoria
    int index = 0;
    for(int i = 0; i < s->numVehiculos; i++){
        if (compararSubcadenaIgnoreCase(s->vehiculos[i].nombrePropietario, subcadena)){
            resultados[index++] = &(s->vehiculos[i]);
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
    //Buscamos el vehiculo en el arreglo
    Vehiculo* v = buscarVehiculoPorID(s, id);

    //Si no existe, retornamos falso
    if (v == nullptr) return false;

    // Validar que los nuevos datos sean validos
    if (nuevosDatos.anio < 1950 || nuevosDatos.anio > 2027) return false;
    if (nuevosDatos.marca[0] == '\0' || nuevosDatos.modelo[0] == '\0' ||
        nuevosDatos.nombrePropietario[0] == '\0' || !validarCedula(nuevosDatos.cedulaPropietario) ||
        nuevosDatos.telefonoPropietario[0] == '\0') {
        return false;
        }
    //Actualizamos los campos numericos directamente
    v->anio = nuevosDatos.anio;

    //Copiamos la marca caracter por caracter
    int i = 0;
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

    //Copiamos la cedula del propietario caracter a caracter
    i = 0;
    while (nuevosDatos.cedulaPropietario[i] != '\0' && i < 19) {
        v->cedulaPropietario[i] = nuevosDatos.cedulaPropietario[i];
        i++;
    }
    v->cedulaPropietario[i] = '\0';

    //Copiamos la cedula del propietario caracter a caracter
    i = 0;
    while (nuevosDatos.telefonoPropietario[i] != '\0' && i < 19) {
        v->telefonoPropietario[i] = nuevosDatos.telefonoPropietario[i];
        i++;
    }
    v->telefonoPropietario[i] = '\0';

    return true; //actualizacion exitante
}

bool eliminarVehiculo(SistemaTaller* s, int id){
    //Verificar si el vehiculo tiene ordenes de servicio registrada
    for(int i = 0; i < s->numOrdenes; i ++){
        if(s->ordenes[i].idVehiculo == id) return false;//rechazado:tiene ordenes asociadas
    }
    
    int idx = -1;

    //Buscamos el indice exacto del vehiculo en el arreglo
    for(int i = 0; i < s->numVehiculos; i++){
        if (s->vehiculos[i].id == id){
            idx = i;
            break;
        }
    }

    //Si no se encuentra, retornamos falso
    if(idx == -1) return false;

    //desplazamos los elemntos hacia la izquierda para "rellenar" el espacio
    for(int i = idx; i < s->numVehiculos - 1; i++){
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
    for (int i = 0; i < s->numMecanicos; i++) {
        if (compararCadenasExactas(s->mecanicos[i].cedula, cedula)) return &(s->mecanicos[i]);   
    }
    return nullptr;
}

Mecanico* buscarMecanicoPorID(SistemaTaller* s, int id) {
    for (int i = 0; i < s->numMecanicos; i++) {
        if (s->mecanicos[i].id == id) return &(s->mecanicos[i]);
    }
    return nullptr;
}

Mecanico* registrarMecanico(SistemaTaller* s, Mecanico datos) {
    // Validamos que los campos no estén vacíos y la especialidad sea válida
    if (datos.nombre[0] == '\0' || !validarCedula(datos.cedula) || !validarFecha(datos.fechaIngreso)) {
        return nullptr;
    }
    // Validar que la especialidad sea una de las 5 permitidas caracter por caracter
    const char* permitidas[5] = {"MOTOR", "FRENOS", "ELECTRICIDAD", "SUSPENSION", "GENERAL"};
    bool especialidadValida = false;
    for (int k = 0; k < 5; k++){
            if (compararCadenasExactas(datos.especialidad, permitidas[k])){
                especialidadValida = true;
                break;
        }
    }

    if (!especialidadValida) return nullptr;

    // Validar que la cédula no este duplicada
    if (buscarMecanicoPorCedula(s, datos.cedula) != nullptr) return nullptr;

    // 1. Verificar si el arreglo de mecanicos esta lleno
    if (s->numMecanicos >= s->capacidadMecanicos) {
        // 1.1 Duplicar la capacidad
        int nuevaCap = s->capacidadMecanicos * 2;

        // 1.2 Crear un nuevo arreglo dinamico con la nueva capacidad
        Mecanico* nuevosM = new Mecanico[nuevaCap];

        // 1.3 Copiar los datos del arreglo viejo al nuevo
        for (int i = 0; i < s->numMecanicos; i++) nuevosM[i] = s->mecanicos[i];

        // 1.4 Liberar la memoria del arreglo viejo
        delete[] s->mecanicos;

        // 1.5 Actualizar el puntero y la capacidad
        s->mecanicos = nuevosM;
        s->capacidadMecanicos = nuevaCap;
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
       if (s->ordenes[i].idMecanico == idMecanico && compararCadenasExactas(s->ordenes[i].estado, "ABIERTA")) {
            contador++;
       }
    }
    return contador;
}

//==========================================
//       CAPA DE LOGICA: REPUESTOS
//==========================================

Repuesto* buscarRepuestoPorCodigo(SistemaTaller* s, const char* codigo){
    for (int i = 0; i < s->numRepuestos; i++){
        if (compararCadenasExactas(s->repuestos[i].codigo, codigo)) return &(s->repuestos[i]);
    }
    return nullptr;
}

Repuesto* buscarRepuestoPorID(SistemaTaller* s, int id) {
    for (int i = 0; i < s->numRepuestos; i++) {
        if (s->repuestos[i].id == id) return &(s->repuestos[i]);
    }
    return nullptr;
}

Repuesto** buscarRepuestosPorNombreParcial(SistemaTaller* s, const char* subcadena, int* cantidad){
    int encontrados = 0;
    for (int i = 0; i < s->numRepuestos; i++){
        if (compararSubcadenaIgnoreCase(s->repuestos[i].nombre, subcadena)) encontrados++;
    }
    *cantidad = 0;
    if (encontrados == 0) return nullptr;

    Repuesto** resultados = new Repuesto*[encontrados];
    int index = 0;
    for (int i = 0; i <s->numRepuestos; i ++) {
        if (compararSubcadenaIgnoreCase(s->repuestos[i].nombre, subcadena)){
            resultados[index++] = &(s->repuestos[i]);
        }
    }
    *cantidad = encontrados;
    return resultados;
}

Repuesto* registrarRepuesto(SistemaTaller* s, Repuesto datos){
    //Validaciones de negocio (Precio > 0, Stock >= 0, Stockminimo >=0, codigo unico???)
    if (datos.codigo[0] == '\0' || datos.nombre[0] == '\0' ||
        datos.precio <= 0.0f || datos.stock < 0 || datos.stockMinimo < 0 || !validarFecha(datos.fechaRegistro)) {
        return nullptr;
    }
    if  (buscarRepuestoPorCodigo(s, datos.codigo) != nullptr) return nullptr;

    //Redimensionamiento automatico (capacidad inicial 4)
    if (s->numRepuestos >= s->capacidadRepuestos){
        int nuevaCap = s->capacidadRepuestos * 2;
        Repuesto* nuevosR = new Repuesto[nuevaCap];
        for (int i = 0; i< s->numRepuestos; i++)nuevosR[i] = s->repuestos[i];
        delete[] s->repuestos;
        s->repuestos = nuevosR;
        s->capacidadRepuestos = nuevaCap;
    }

    datos.id = s->siguienteIdRepuesto++;
    s->repuestos[s->numRepuestos] = datos;
    s->numRepuestos++;
    return &(s->repuestos[s->numRepuestos - 1]);
}

bool registrarEntradaMercancia(SistemaTaller* s, int idRepuesto, int cantidad){
    if (cantidad <= 0) return false;
    Repuesto* rep = buscarRepuestoPorID(s, idRepuesto);
    if (rep == nullptr) return false;

    rep->stock += cantidad;
    return true;
}

Repuesto** listarRepuestos(SistemaTaller* s, int* cantidad){
    if (s->numRepuestos == 0){
        *cantidad = 0;
        return nullptr;
    }
    *cantidad = s->numRepuestos;
    Repuesto** lista = new Repuesto*[s->numRepuestos];
    for (int i = 0; i< s->numRepuestos; i++) lista[i] = &(s->repuestos[i]);
    return lista;
}

bool cambiarPrecioRepuesto(SistemaTaller* s, int idRepuesto, float nuevoPrecio){
    if (nuevoPrecio <= 0.0f) return false;
    Repuesto* rep = buscarRepuestoPorID(s, idRepuesto);
    if (rep == nullptr) return false;

    rep->precio = nuevoPrecio; //no afecta ordenes historicas ya cerradas
    return true;
}

bool eliminarRepuesto(SistemaTaller* s, int idRepuesto){
    //REchazado si aparece en detalles de alguna orden
    for (int i = 0; i < s->numOrdenes; i++){
        for (int j = 0; j < s->ordenes[i].numDetalles; j++){
            if (s->ordenes[i].detalles[j].idRepuesto == idRepuesto) return false;
        }
    }

    int idx = -1;
    for (int i = 0; i < s->numRepuestos; i++){
        if (s->repuestos[i].id == idRepuesto){ idx = i; break; }
    }
    if (idx == -1) return false;

    for (int i = idx; i < s->numRepuestos - 1; i++){
        s->repuestos[i] = s->repuestos[i + 1];
    }
    s->numRepuestos--;
    return true;
}

//==========================================
//       CAPA DE LOGICA: ORDENES
//==========================================

// Funcion auxiliar que verifica si un vehiculo ya tiene una orden abierta
OrdenServicio* buscarOrdenAbiertaVehiculo(SistemaTaller* s, int idVehiculo){
    for (int i = 0; i < s->numOrdenes; i++) {
        if (s->ordenes[i].idVehiculo == idVehiculo && compararCadenasExactas(s->ordenes[i].estado, "ABIERTA")){
                return &(s->ordenes[i]);
        }
    }
    return nullptr; // No tiene ordenes abiertas
}

OrdenServicio* abrirOrden (SistemaTaller* s, int idVehiculo, int idMecanico, const char* fechaApertura, const char* descripcionFalla, int kilometrajeIngreso) {
    // Validamos que la fecha y la descripcion de la falla no esten vacias
    if (!validarFecha(fechaApertura) || descripcionFalla[0] == '\0') {
        return nullptr;
    }
    // Verificamos la existencia de vehículos y mecánicos
    Vehiculo* v = buscarVehiculoPorID(s, idVehiculo);
    Mecanico* m = buscarMecanicoPorID(s, idMecanico);
    if (v == nullptr || m == nullptr) return nullptr;

    if (buscarOrdenAbiertaVehiculo(s, idVehiculo) != nullptr) return nullptr;
    if (contarOrdenesAbiertasMecanico(s, idMecanico) >= 3) return nullptr;
    if (kilometrajeIngreso < v->kilometraje) return nullptr;

    // Redimensionar el arreglo de ordenes del sistema si esta lleno
    if (s->numOrdenes >= s->capacidadOrdenes) {
        int nuevaCap = s->capacidadOrdenes * 2;
        OrdenServicio* nuevasO = new OrdenServicio[nuevaCap];
        for (int i = 0; i < s->numOrdenes; i++) nuevasO[i] = s->ordenes[i];
        delete[] s->ordenes;
        s->ordenes = nuevasO;
        s->capacidadOrdenes = nuevaCap;
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
        if (s->ordenes[i].id == id) return &(s->ordenes[i]);
    }
    return nullptr;
}

bool agregarRepuestoAOrden(SistemaTaller* s, int idOrden, int idRepuesto, int cantidad) {
    // La cantidad debe ser mayor a 0
    if (cantidad <= 0) return false;

    OrdenServicio* orden = buscarOrdenPorID(s, idOrden);
    if (orden == nullptr || !compararCadenasExactas(orden->estado, "ABIERTA")) return false;

    // Verificamos que el repuesto exista y tenga stock suficiente
    Repuesto* rep = buscarRepuestoPorID(s, idRepuesto);
    if (rep == nullptr || rep->stock < cantidad) return false;

    rep->stock -= cantidad; //Descontar stock

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
        int nuevaCap = orden->capacidadDetalles * 2;
        DetalleOrden* nuevosD = new DetalleOrden[nuevaCap];
        for (int i = 0; i < orden->numDetalles; i++) nuevosD[i] = orden->detalles[i];
        delete[] orden->detalles;
        orden->detalles = nuevosD;
        orden->capacidadDetalles = nuevaCap;
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
    if (orden == nullptr || !compararCadenasExactas(orden->estado, "ABIERTA")) return false;

    // Buscar el repuesto dentro del arreglo de detalles de la orden
    int idx = -1;
    for (int i = 0; i < orden->numDetalles; i++) {
        if (orden->detalles[i].idRepuesto == idRepuesto) {
            idx = i;
            break;
        }
    }

    if (idx == -1) return false; // El repuesto no estaba en la orden

    // Devolver las unidades al inventario de repuestos
    Repuesto* rep = buscarRepuestoPorID(s, idRepuesto);
    if (rep != nullptr) rep->stock += orden->detalles[idx].cantidad;

    // Desplazar los detalles siguientes hacia la izquierda para eliminar la linea
    for (int i = idx; i < orden->numDetalles - 1; i++) {
        orden->detalles[i] = orden->detalles[i + 1];
    }

    orden->numDetalles--;
    return true;
}

bool cerrarOrden(SistemaTaller* s, int idOrden, float manoObra, const char* fechaCierre) {
    // Validar mano de obra >= 0 y fecha no vacia
    if (manoObra < 0.0f || !validarFecha(fechaCierre)) return false;

    OrdenServicio* orden = buscarOrdenPorID(s, idOrden);
    if (orden == nullptr || !compararCadenasExactas(orden->estado, "ABIERTA")) return false;

    // Debe quedar con al menos una linea de detalle o mano de obra > 0
    if (orden->numDetalles == 0 && manoObra <= 0.0f) return false;

    // Actualizamos mano de obra, fecha de cierre y cambiamos estado a cerrada
    orden->manoObra = manoObra;

    int i = 0;
    while (fechaCierre[i] != '\0' && i < 10) {
        orden->fechaCierre[i] = fechaCierre[i];
        i++;
    }
    orden->fechaCierre[i] = '\0';

    const char* est = "CERRADA";
    i = 0;
    while (est[i] != '\0' && i < 11) {
        orden->estado[i] = est[i];
        i++;
    }
    orden->estado[i] = '\0';

    return true;
}

bool anularOrden (SistemaTaller* s, int idOrden) {
    OrdenServicio* orden = buscarOrdenPorID(s, idOrden);
    if (orden == nullptr || compararCadenasExactas(orden->estado, "ANULADA")) return false;

    // Devolvemos al inventario todas las unidades de cada linea de detalle
    for (int i = 0; i < orden->numDetalles; i++) {
        Repuesto* rep = buscarRepuestoPorID(s, orden->detalles[i].idRepuesto);
        if (rep != nullptr) rep->stock += orden->detalles[i].cantidad;
    }
    // Cambiamos el estado a "ANULADA"
    const char* est = "ANULADA";
    int i = 0;
    while (est[i] != '\0' && i < 11) {
        orden->estado[i] = est[i];
        i++;
    }
    orden->estado[i] = '\0';

    return true;
}

float calcularTotalOrden(OrdenServicio* orden) {
    if (orden == nullptr) return 0.0f;
    float sumaRepuestos = 0.0f;
    for (int i = 0; i < orden->numDetalles; i++) {
        sumaRepuestos += (orden->detalles[i].cantidad * orden->detalles[i].precioUnitario);
    }

    return orden->manoObra + sumaRepuestos;
}

OrdenServicio** listarOrdenesPorEstado(SistemaTaller* s, const char* estadoBuscado, int* cantidad) {
    // Contar coincidencias de ordenes según estado
    int encontrados = 0;
    for (int i = 0; i < s->numOrdenes; i++) {
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
        encontrados++;
        }
    }

    *cantidad = 0;
    if (encontrados == 0) {
        return nullptr;
    }

    // Reservar arreglo dinámico de ptr
    OrdenServicio** resultados = new OrdenServicio*[encontrados];

    // Llenar con las direcciones de las ordenes encontradas
    int index = 0;
    for (int i = 0; i < s->numOrdenes; i++) {
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
            resultados[index] = &(s->ordenes[i]);
            index++;
        }
    
    }
    *cantidad = encontrados;
    return resultados;
}

OrdenServicio** listarOrdenesPorVehiculo(SistemaTaller* s, int idVehiculo, int* cantidad) {
    // Contar cuantas ordenes pertenecen al vehículo
    int encontrados =  0;
    for (int i = 0; i < s->numOrdenes; i++) {
        if (s->ordenes[i].idVehiculo == idVehiculo) {
            encontrados++;
        }
    }
    *cantidad = 0;
    if (encontrados == 0) {
        return nullptr;
    }
    // Reservar arreglo dinámico de ptr
    OrdenServicio** resultados = new OrdenServicio*[encontrados];
    
    //Llenar con las direcciones de memoria
    int index = 0;
    for (int i = 0; i < s->numOrdenes; i++) {
        if (s->ordenes[i].idVehiculo == idVehiculo) {
            resultados[index] = &(s->ordenes[i]);
            index++;
        }
    }
    *cantidad = encontrados;
    return resultados;
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
    cout << "Seleccione una opcion: ";
}

//Vehiculos UI
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
    cout << "Seleccione una opcion: ";
}

void mostrarVehiculo(Vehiculo* v){
    if (v == nullptr) return;
    cout << "----------------------------------------" << endl;
    cout << " ID          : " << v->id << endl;
    cout << " Placa       : " << v->placa << endl;
    cout << " Marca/Modelo: " << v->marca << " " << v->modelo << " (" << v->anio << ")" << endl;
    cout << " Kilometraje : " << v->kilometraje << " km" << endl;
    cout << " Propietario : " << v->nombrePropietario << "(CI:" << v->cedulaPropietario << ")" << endl;
    cout << " Telefono    : " << v->telefonoPropietario << endl;
    cout << " Registro    : " << v->fechaRegistro << endl;
}

void mostrarListaVehiculos(Vehiculo** vehiculos, int cantidad){
    if (vehiculos == nullptr || cantidad <= 0){
        cout << "\n[!] No hay vehiculos para mostrar." << endl;
        return;
    }
    cout << "\n=== LISTADO DE VEHICULOS (" << cantidad << ") ===" << endl;
    for (int i = 0; i < cantidad; i ++){
        mostrarVehiculo((vehiculos[i]));
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
    cin.ignore(10000, '\n');
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
        cout << "\n No hay vehiculos registrados en el sistema." << endl;
    }
    cin.ignore(10000, '\n');
    pausarylimpiar();
}

void uiRegistrarVehiculo(SistemaTaller* s){
    limpiar();
    cout << "=== REGISTRAR NUEVO VEHICULO ===" << endl;
    cout << "(Escriba CANCELAR para abortar)." << endl;
    Vehiculo nuevo;

    cout << "Placa: "; cin >> nuevo.placa;
    if (esCancelar(nuevo.placa)){
        cout <<"\nRegistro cancelado por el usuario." << endl;
        cin.ignore(10000, '\n');
        pausarylimpiar();
        return;
    }
    cout << "Marca: "; cin >> nuevo.marca;
    if (esCancelar(nuevo.marca)){
        cout <<"\nRegistro cancelado por el usuario." << endl;
        cin.ignore(10000, '\n');
        pausarylimpiar();
        return;
    }
    cout << "Modelo: "; cin >> nuevo.modelo;
    if (esCancelar(nuevo.modelo)){
        cout <<"\nRegistro cancelado por el usuario." << endl;
        cin.ignore(10000, '\n');
        pausarylimpiar();
        return;
    }
    cout << "Año (1950-2027): "; cin >> nuevo.anio;
    cout << "Kilometraje (>= 0): "; cin >> nuevo.kilometraje;
    
    // Limpiamos el salto de linea antes de usar getline
    cin.ignore(10000, '\n');
    cout << "Nombre del Propietario: "; cin.getline(nuevo.nombrePropietario, 100);
    if (esCancelar(nuevo.nombrePropietario)) {
        cout << "\nRegistro cancelado por el usuario." << endl;
        pausarylimpiar();
        return;
    }
    cout << "Cedula del Propietario: "; cin >> nuevo.cedulaPropietario;
    if (esCancelar(nuevo.cedulaPropietario)) {
        cout << "\nRegistro cancelado por el usuario." << endl;
        cin.ignore(10000, '\n');
        pausarylimpiar();
        return;
    }
    cout << "Telefono del Propietario: "; cin >> nuevo.telefonoPropietario;
    cout << "Fecha de Registro (YYYY-MM-DD): "; cin >> nuevo.fechaRegistro;

    if (buscarVehiculoPorPlaca(s, nuevo.placa) != nullptr) {
        cout << "\nERROR: La placa '" << nuevo.placa << "' ya está registrada." << endl;
        cin.ignore(10000, '\n');
        pausarylimpiar();
        return;
    }

    if (!pedirConfirmacion("\n¿Confirmar el registro del vehículo?")) {
        cout << "\nRegistro cancelado." << endl;
        cin.ignore(10000, '\n');
        pausarylimpiar();
        return;
    }

    Vehiculo* creado = agregarVehiculo(s, nuevo);
    if (creado != nullptr) {
        cout << "\nVehiculo '" << creado->placa << "' registrado exitosamente con ID #" << creado->id << "." << endl;
    } else {
        cout << "\nERROR: No se pudo registrar el vehiculo (verifique los campos anteriores)." << endl;
    }
    cin.ignore(10000, '\n');
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
        case 1:{
            int idBuscado;
            cout << "Ingrese un ID registrado: ";
            cin >> idBuscado;
            cin.ignore(10000, '\n');
    
            Vehiculo* ubicado = buscarVehiculoPorID(s, idBuscado);
            if (ubicado != nullptr) {
                mostrarVehiculo(ubicado);
            } else {
                cout << "\nERROR: No existe ningún vehículo con el ID #" << idBuscado << "."<< endl;
            }
            break; 
        }case 2:{
            char placaBuscada[10];
            cout << "Ingrese la placa exacta: ";
            cin >> placaBuscada;
            cin.ignore(10000, '\n');

            Vehiculo* ubicado = buscarVehiculoPorPlaca(s, placaBuscada);
            if (ubicado != nullptr){
                mostrarVehiculo(ubicado);
            } else{
                cout << "\nError: No se encontro ningun vehiculo con la placa '" << placaBuscada << "'." << endl;
            }
            break;
        }case 3:{
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
        }default:{
            cin.ignore(10000, '\n');
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
        cout << "\nError: No existe un vehiculo registrado con el ID " << id << "." << endl;
        cin.ignore(10000, '\n');
        pausarylimpiar();
        return;
    }

    cout << "\nVehiculo actual encontrado: " << endl;
    mostrarVehiculo(v);

    Vehiculo nuevosDatos;
    cout << "\nIngrese los nuevos datos (Placa y kilometraje no se modifican): " << endl;
    cout << "Nueva Marca: "; cin >> nuevosDatos.marca;
    cout << "Nuevo Modelo: "; cin >> nuevosDatos.modelo;
    cout << "Nuevo anio (1950 - 2027): "; cin >> nuevosDatos.anio;

    cin.ignore(10000, '\n');
    cout << "Nuevo Nombre del Propietario: "; cin.getline(nuevosDatos.nombrePropietario, 100);
    cout << "Nueva Cédula del Propietario: "; cin >> nuevosDatos.cedulaPropietario;
    cout << "Nuevo Teléfono del Propietario: "; cin >> nuevosDatos.telefonoPropietario;
    
    if (!pedirConfirmacion("\nConfirmar la actualizacion del vehiculo?")){
        cout << "\nActualización cancelada." << endl;
        cin.ignore(10000, '\n');
        pausarylimpiar();
        return; 
    }

    if (actualizarVehiculo(s, id, nuevosDatos)){
        cout << "\nVehiculo actualizado sasticfactoriamente." << endl;
    } else{
        cout << "\nError: No se pudo actualizar (verifique el año entre 1950-2027) y cédula válida." << endl;
    }
    cin.ignore(10000, '\n');
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
    cin.ignore(10000, '\n');
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
    cin.ignore(10000, '\n');
    pausarylimpiar();
}

//Mecanicos UI
void menuMecánicos () {
    cout << "╔═══════════════════════════════════════════╗" << endl;
    cout << "║            GESTIÓN DE MECÁNICOS           ║" << endl;
    cout << "╠═══════════════════════════════════════════╣" << endl;
    cout << "║ 1. Registrar mecánico                     ║" << endl;
    cout << "║ 2. Buscar por ID                          ║" << endl;
    cout << "║ 3. Listar todos los mecánicos             ║" << endl;
    cout << "║ 0. Volver al menú principal               ║" << endl;
    cout << "╚═══════════════════════════════════════════╝" << endl;
    cout << "Seleccione una opcion: ";
}

void uiRegistrarMecanico(SistemaTaller* s){
    limpiar();
    cout << "=== REGISTRAR NUEVO MECANICO ===" << endl;
    Mecanico nuevo;

    // Limpiamos el salto de linea antes de usar getline
    cin.ignore(10000, '\n');
    cout << "Nombre del Mecanico: "; cin.getline(nuevo.nombre, 100);
    cout << "Cedula: "; cin >> nuevo.cedula;
    cout << "Especialidad (MOTOR, FRENOS, ELECTRICIDAD, SUSPENSION, GENERAL): "; cin >> nuevo.especialidad;
    cout << "Fecha de Ingreso (YYYY-MM-DD): "; cin >> nuevo.fechaIngreso;

    if (registrarMecanico(s, nuevo) != nullptr) {
    cout << "\nMecanico registrado exitosamente con ID asignado!" << endl;
    } else {
        cout << "\nERROR: No se pudo registrar el mecanico." << endl;
    }
    cin.ignore(10000, '\n');
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
    cin.ignore(10000, '\n');
    pausarylimpiar();
}

//Repuestos UI
void menuRepuestos () {
    cout << "╔═══════════════════════════════════════════╗" << endl;
    cout << "║          INVENTARIO DE REPUESTOS          ║" << endl;
    cout << "╠═══════════════════════════════════════════╣" << endl;
    cout << "║ 1. Registrar repuesto                     ║" << endl;
    cout << "║ 2. Buscar repuesto (ID / Código / Nombre) ║" << endl;
    cout << "║ 3. Registrar entrada de mercancía         ║" << endl;
    cout << "║ 4. Listar todos los repuestos             ║" << endl;
    cout << "║ 5. Cambiar precio                         ║" << endl;
    cout << "║ 6. Eliminar repuesto                      ║" << endl;
    cout << "║ 0. Volver al menú principal               ║" << endl;
    cout << "╚═══════════════════════════════════════════╝" << endl;
    cout << "Seleccione una opcion: ";
}

// Funcion auxiliar para mostrar un repuesto individual con su puntero
void mostrarRepuesto(Repuesto* r) {
    if (r == nullptr) return;
    cout << "----------------------------------------" << endl;
    cout << " ID           : " << r->id << endl;
    cout << " Código       : " << r->codigo << endl;
    cout << " Nombre       : " << r->nombre << endl;
    cout << " Marca        : " << r->marca << endl;
    cout << " Precio Unit. : $" << r->precio << endl;
    cout << " Stock Actual : " << r->stock << " (Mínimo: " << r->stockMinimo << ")";

    // Alerta de bajo stock
    if (r->stock <= r->stockMinimo) {
        cout << " <-- [ALERTA: BAJO STOCK]";
    }
    cout << endl;
    cout << " Registro     : " << r->fechaRegistro << endl;
}
// Funcion auxiliar para mostrar un arreglo de punteros a repuestos
void mostrarListaRepuestos(Repuesto** repuestos, int cantidad) {
    if (repuestos == nullptr || cantidad <= 0) {
        cout << "\n No hay repuestos para mostrar." << endl;
        return;
    }
    cout << "\n=== LISTADO DE REPUESTOS (" << cantidad <<") ===" << endl;
    for (int i = 0; i < cantidad; i++) {
        mostrarRepuesto(repuestos[i]);
    }
    cout << "----------------------------------------" << endl;
    cout << "Total de repuestos mostrados: " << cantidad << endl;
}

void uiRegistrarRepuesto(SistemaTaller* s) {
    limpiar();
    cout << "=== REGISTRAR NUEVO REPUESTO ===" << endl;
    Repuesto nuevo;

    cout << "Código (único, sin espacios, ej. FRN-001): ";
    cin >> nuevo.codigo;

    // Limpiamos el salto de linea para leer nombres con espacios
    cin.ignore(10000, '\n');

    cout << "Nombre del repuesto: "; cin.getline(nuevo.nombre, 100);
    cout << "Marca: "; cin.getline(nuevo.marca, 50);
    cout << "Precio unitario (>0): "; cin >> nuevo.precio;
    cout << "Stock inicial (>= 0): "; cin >> nuevo.stock;
    cout << "Stock mínimo para alerta (>= 0): "; cin >> nuevo.stockMinimo;
    cout << "Fecha de registro (YYYY-MM-DD): "; cin >> nuevo.fechaRegistro;
    cin.ignore(10000, '\n');

    // Verificamos si el código ya existe para dar un mensaje de error
    if (buscarRepuestoPorCodigo(s, nuevo.codigo) != nullptr) {
        cout << "\nERROR: El código de repuesto '" << nuevo.codigo << "' ya está registrado." << endl;
        pausarylimpiar();
        return;
    }

    Repuesto* creado = registrarRepuesto(s, nuevo);
    if (creado != nullptr) {
        cout << "\nRepuesto '" << creado->nombre << "' registrado exitosamente con ID #" << creado->id << endl;
    } else {
        cout << "\nERROR: No se pudo registrar el repuesto (verifique precio, stock y campos no vacíos)." << endl;
    }
    pausarylimpiar();
}

void uiBuscarRepuesto(SistemaTaller* s) {
    limpiar();
    cout << "=== BUSCAR REPUESTO ===" << endl;
    int op = 0;
    cout << "Seleccione el modo de búsqueda:" << endl;
    cout << "1. Buscar por ID" << endl;
    cout << "2. Buscar por Código exacto" << endl;
    cout << "3. Buscar por Nombre parcial" << endl;
    cout << "Opción: ";
    cin >> op;

    switch (op) {
        case 1: {
            int idBuscado;
            cout << "Ingrese el ID del repuesto: ";
            cin >> idBuscado;
            cin.ignore(10000, '\n');

            Repuesto* ubicado = buscarRepuestoPorID (s, idBuscado);
            if (ubicado != nullptr) {
                cout << "\n¡Repuesto encontrado!" << endl;
                mostrarRepuesto(ubicado);
            } else {
                cout << "\nERROR: No existe ningún repuesto con el ID #" << idBuscado << "." << endl;
            }
            break;
        }
        case 2: {
            char codigoBuscado[20];
            cout << "Ingrese el código exacto: ";
            cin >> codigoBuscado;
            cin.ignore(10000, '\n');

            Repuesto* ubicado = buscarRepuestoPorCodigo(s, codigoBuscado);
            if (ubicado != nullptr) {
                cout << "\n¡Repuesto encontrado!" << endl;
                mostrarRepuesto(ubicado);
            } else {
                cout << "\nERROR: No se encontró ningún repuesto con el código '" << codigoBuscado << "'." << endl;
            }
            break;
        }
        case 3: {
            char subcadena[100];
            cin.ignore(10000, '\n');
            cout << "Ingrese parte del nombre del repuesto: ";
            cin.getline(subcadena, 100);

            int cantidad = 0;
            Repuesto** resultados = buscarRepuestosPorNombreParcial(s, subcadena, &cantidad);
            if (resultados != nullptr) {
                mostrarListaRepuestos(resultados, cantidad);
                // Regla de memoria: liberamos el arreglo temporal de punteros
                delete[] resultados;
            } else {
                cout << "\nNo se encontraron repuestos que coincidan con '" << subcadena << "'." << endl;
            }
            break;
        }
        default: {
            cin.ignore(10000, '\n');
            cout << "\nOpción de búsqueda inválida." << endl;
            break;
        }
    }
    pausarylimpiar();
}

void uiEntradaMercancia(SistemaTaller* s) {
    limpiar();
    cout << "=== REGISTRAR ENTRADA DE MERCANCÍA ===" << endl;
    int idRepuesto, cantidad;

    cout << "Ingrese el ID del repuesto: ";
    cin >> idRepuesto;

    Repuesto* rep = buscarRepuestoPorID(s, idRepuesto);
    if (rep == nullptr) {
        cin.ignore(10000, '\n');
        cout << "\nERROR: No existe un repuesto con el ID #" << idRepuesto << "." << endl;
        pausarylimpiar();
        return;
    }   
    cout << "Repuesto seleccionado: " << rep->nombre << " (Stock actual: " << rep->stock << ")" << endl;
    cout << "Ingrese la cantidad de unidades que ingresan (> 0): ";
    cin >> cantidad;
    cin.ignore(10000, '\n');

    if (registrarEntradaMercancia(s, idRepuesto, cantidad)) {
        cout << "\n¡Entrada registrada! Nuevo stock de '" << rep->nombre << "': " << rep->stock << "unidades." << endl;
    } else { 
        cout << "\nERROR: La cantidad a ingresar debe ser mayor a 0." << endl;
    }
    pausarylimpiar();
}

void uiListarRepuestos(SistemaTaller* s) {
    limpiar();
    int cantidad = 0;
    Repuesto** lista = listarRepuestos(s, &cantidad);

    if (lista != nullptr) {
        mostrarListaRepuestos(lista, cantidad);
        // Liberamos el arreglo de ptrs retornado por la lógica
        delete[] lista;
    } else {
        cout << "\nNo hay repuestos registrados en el inventario." << endl;
    }
    cin.ignore(10000, '\n');
    pausarylimpiar();
}

void uiCambiarPrecioRepuesto(SistemaTaller* s) {
    limpiar();
    cout << "=== CAMBIAR PRECIO DE REPUESTO ===" << endl;
    int idRepuesto;
    float nuevoPrecio;

    cout << "Ingrese el ID del repuesto: ";
    cin >> idRepuesto;

    Repuesto* rep = buscarRepuestoPorID(s, idRepuesto);
    if (rep == nullptr) {
        cin.ignore(10000, '\n');
        cout <<"\nERROR: No existe un repuesto con el ID #" << idRepuesto << "." << endl;
        pausarylimpiar();
        return;
    }

    cout << "Repuesto: " << rep->nombre << " | Precio actual: $" << rep->precio << "." << endl;
    cout << "Ingrese el nuevo precio unitario (> 0): ";
    cin >> nuevoPrecio;
    cin.ignore(10000, '\n');

    if (cambiarPrecioRepuesto(s, idRepuesto, nuevoPrecio)) {
        cout << "\nPrecio actualizado exitosamente a $" << rep->precio  << "." << endl;
        cout << "(Nota: Las órdenes registradas previamente mantienen su precio histórico)." << endl;
    } else {
        cout << "\nERRROR: El precio debe ser mayor o igual a 0." << endl;
    }
    pausarylimpiar();
}

void uiEliminarRepuesto(SistemaTaller* s) {
    limpiar();
    cout << "=== ELIMINAR REPUESTO ===" << endl;
    int idRepuesto;

    cout << "Ingrese el ID del repuesto a eliminar: ";
    cin >> idRepuesto;
    cin.ignore(10000, '\n');

    Repuesto* rep = buscarRepuestoPorID(s, idRepuesto);
    if (rep == nullptr) {
        cout << "\nERROR: No existe ningún repuesto con el ID #" << idRepuesto << "." << endl;
        pausarylimpiar();
        return;
    }

    if (eliminarRepuesto(s, idRepuesto)) {
        cout << "\nRepuesto eliminado del inventario." << endl;
    } else {
        cout << "\nERROR: El repuesto '" << rep->nombre << "' está registrado en una o más órdenes de servicio y no puede eliminarse." << endl;
    }
    pausarylimpiar();
}

//Ordenes UI

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
    cout << "║ 7. Listar órdenes                         ║" << endl;
    cout << "║ 0. Volver al menú principal               ║" << endl;
    cout << "╚═══════════════════════════════════════════╝" << endl;
    cout << "Seleccione una opcion: ";
}

void uiAbrirOrden(SistemaTaller* s){
    limpiar();
    cout << "=== ABRIR ORDEN DE SERVICIO ===" << endl;
    int idV, idM, km;
    char fecha[11], falla[200];

    cout << "ID del vehiculo: "; cin >> idV;
    cout << "ID del mecanico: "; cin >> idM;
    cout << "Kilometraje de ingreso: "; cin >> km;
    cout << "Fecha (YYY-MM-DD): "; cin >> fecha;
    cin.ignore(10000, '\n');
    cout << "Descripcion de la falla: "; cin.getline(falla, 200);
    
    Vehiculo* v = buscarVehiculoPorID(s, idV);
    if (v == nullptr) {
        cout << "\nERROR: No existe ningún vehículo con el ID #" << idV << "." << endl;
        pausarylimpiar();
        return;
    }

    Mecanico* m = buscarMecanicoPorID(s, idM);
    if (m == nullptr){
        cout << "\nError: Noexiste ningún mecánico con el ID #" << idM << "." << endl;
        pausarylimpiar();
        return;
    }
    
    OrdenServicio* abierta = buscarOrdenAbiertaVehiculo(s, idV);
    if (abierta != nullptr){
        cout << "\nError: El vehiculo '" << v->placa << "' ya tiene la orden #" << abierta->id << " ABIERTA." << endl;
        pausarylimpiar();
        return;
    }
    if (contarOrdenesAbiertasMecanico(s, idM) >= 3) {
        cout << "\nERROR: El mecánico '" << m->nombre << "' ya tiene 3 órdenes abiertas." << endl;
        pausarylimpiar();
        return;
    }
    
    if (km < v->kilometraje){
        cout << "\nError: El kilometraje de ingreso (" << km << ") es menor al registrado (" << v->kilometraje << " km)." << endl;
        pausarylimpiar();
        return;
    }
    
    if (!pedirConfirmacion("\n¿Confirmar la apertura de la orden?")) {
        cout << "\nApertura cancelada." << endl;
        cin.ignore(10000, '\n');
        pausarylimpiar();
        return;
    }
    cin.ignore(10000, '\n');

    OrdenServicio* o = abrirOrden(s, idV, idM, fecha, falla, km);
    if (o){
        cout << "\nOrden #" << o->id << " abierta exitosamente." << endl;
    } else{
        cout << "\nError: Verifique IDs, formato de fecha, limite de 3 ordenes por mecanico, kilometraje y falla no vacía." << endl;
    }
    pausarylimpiar();
}

void uiAgregarRepuestoOrden(SistemaTaller* s){
    limpiar();
    cout << "=== AGREGRAR REPUESTO A ORDEN ===" << endl;
    int idO, idR, cant;
    cout << "ID de la orden: "; cin >> idO;
    cout << "ID del repuesto: "; cin >> idR;
    cout << "Cantidad: "; cin >> cant;
    cin.ignore(10000, '\n');

    OrdenServicio* o = buscarOrdenPorID(s, idO);
    if (o == nullptr){
        cout << "\nERROR: No existe la orden #" << idO << "." << endl;
        pausarylimpiar();
        return;
    }
    if (!compararCadenasExactas(o->estado, "ABIERTA")){
        cout << "\nERROR: La orden #" << idO << " esta " << o->estado << " y no puede modificarse." << endl;
        pausarylimpiar();
        return;
    }
    
    Repuesto* r = buscarRepuestoPorID(s, idR);
    if (r == nullptr) {
        cout << "\nERROR: No existe el repuesto con ID #" << idR << "." << endl;
        pausarylimpiar();
        return;
    }
    
    if (r->stock < cant) {
        cout << "\nERROR: Stock insuficiente de '" << r->nombre << "'. Disponible: " << r->stock << ", solicitado: " << cant << "." << endl;
        pausarylimpiar();
        return;
    }

    if (agregarRepuestoAOrden(s, idO, idR,cant)){
        cout << "\nRepuesto '" << r->nombre << "agregado con exito a la orden #"  << idO << endl;
    } else{
        cout << "\nERROR: Stock insuficiente o la orden no esta aierta." << endl;
        pausarylimpiar();
    }
}

void uiQuitarRepuestoOrden(SistemaTaller* s){
    limpiar();
    cout << "=== QUITAR REPUESTO DE ORDEN ===" << endl;
    int idO, idR;
    cout << "ID de la orden: "; cin >> idO;
    cout << "ID del repuesto: "; cin >> idR;
    cin.ignore(10000, '\n');

    OrdenServicio* o = buscarOrdenPorID(s, idO);
    if ( o != nullptr && !compararCadenasExactas(o->estado, "ABIERTA")){
        cout << "\nERROR: La orden #" << idO << " está " << o->estado << " y no puede modificarse." << endl;
        pausarylimpiar();
        return;
    }
    if (quitarRepuestoDeOrden(s, idO, idR)){
        cout << "Repuesto removido y stock devuelto." << endl;
    } else{
        cout << "Error al quitar el repuesto." << endl;
    }
    pausarylimpiar();
}

void uiCerrarOrden(SistemaTaller* s){
    limpiar();
    cout << "=== CERRAR ORDEN ===" << endl;
    int idO; float manoObra; char fecha[11];
    cout << "ID de la orden: "; cin >> idO;
    cout << "Costo de mano de mano de obra: "; cin >> manoObra;
    cout << "Fecha cierre (YYYY-MM-DD): "; cin >> fecha;

    if (!pedirConfirmacion("\n¿Confirmar el cierre de la orden?")) {
        cout << "\nCierre cancelado." << endl;
        cin.ignore(10000, '\n');
        pausarylimpiar();
        return;
    }
    cin.ignore(10000, '\n');
    
    if (cerrarOrden(s, idO, manoObra, fecha)) {
        cout << "Orden #" << idO << "cerrada con éxito." << endl;
    } else{
        cout << "\nERROR: Verifique que la orden esté ABIERTA, tenga detalles o mano de obra > 0, y fecha válida." << endl;
    }
    pausarylimpiar();
}

void uiAnularOrden(SistemaTaller* s){
    limpiar();
    cout << "=== ANULAR ORDEN ===" << endl;
    int idO;
    cout << "ID de la orden a anular: "; cin >> idO;

    if (!pedirConfirmacion("\nConfirmar la anulacion de la orden? (Se devolvera el stock al inventario)")){
        cout << "\nAnulación cancelada." << endl;
        cin.ignore(10000, '\n');
        pausarylimpiar();
        return;
    }
    cin.ignore(10000, '\n');

    if (anularOrden(s, idO)){
        cout <<"\nOrden #" << idO << " ANULADA y stock devuelto al inventario" << endl;
    } else{
        cout << "\nERROR: Orden inexistente o ya anulada." << endl;
    }
    pausarylimpiar();
}

void uiVerDetalleOrden(SistemaTaller* s){
    limpiar();
    cout << "=== VER DETALLE ORDEN ===" << endl;
    int idO;
    cout << "Ingrese el ID de la orden: "; cin >> idO;
    cin.ignore(10000, '\n');
    
    OrdenServicio* o = buscarOrdenPorID(s, idO);
    if (o == nullptr){
        cout << "Error: No se encontro ninguna orden con el ID #" << idO << "." << endl;
        pausarylimpiar();
        return;
    }

    Vehiculo* v = buscarVehiculoPorID(s, o->idVehiculo);
    Mecanico* m = buscarMecanicoPorID(s, o->idMecanico);

    cout << "\n╔════════════════════════════════════════════════════════════════╗" << endl;
    cout << "║                     ORDEN DE SERVICIO #" << o->id << "                       ║" << endl;
    cout << "╠════════════════════════════════════════════════════════════════╣" << endl;
    cout << "║ Estado   : " << o->estado << "          Apertura: " << o->fechaApertura << "          ║" << endl;
    if (compararCadenasExactas(o->estado, "CERRADA")) {
        cout << "║ Cierre   : " << o->fechaCierre << "                                          ║" << endl;
    }
    if (v != nullptr) {
        cout << "║ Vehículo : " << v->placa << " - " << v->marca << " " << v->modelo << " " << v->anio << " (" << o->kilometrajeIngreso << " km)" << endl;
        cout << "║ Dueño    : " << v->nombrePropietario << endl;
    }
    if (m != nullptr) {
        cout << "║ Mecánico : " << m->nombre << " (" << m->especialidad << ")" << endl;
    }
    cout << "║ Falla    : " << o->descripcionFalla << endl;
    cout << "╠══════════╦═══════════════════════════╦══════╦═════════╦════════╣" << endl;
    cout << "║ Código   ║ Repuesto                  ║ Cant ║ P.Unit  ║ Total  ║" << endl;
    cout << "╠══════════╬═══════════════════════════╬══════╬═════════╬════════╣" << endl;

    float sumaRepuestos = 0.0f;
    for (int i = 0; i < o->numDetalles; i++) {
        Repuesto* rep = buscarRepuestoPorID(s, o->detalles[i].idRepuesto);
        float subTotalLinea = o->detalles[i].cantidad * o->detalles[i].precioUnitario;
        sumaRepuestos += subTotalLinea;

        if (rep != nullptr) {
            cout << "║ " << rep->codigo << "  ║ "
                 << rep->nombre << " ║  "
                 << o->detalles[i].cantidad << "   ║  "
                 << o->detalles[i].precioUnitario << "  ║ "
                 << subTotalLinea << " ║" << endl;
        }
    }
    cout << "╠══════════╩═══════════════════════════╩══════╩═════════╬════════╣" << endl;
    cout << "║                                            Repuestos  ║ " << sumaRepuestos << " ║" << endl;
    cout << "║                                            Mano obra  ║ " << o->manoObra << " ║" << endl;
    cout << "║                                            TOTAL      ║ " << calcularTotalOrden(o) << " ║" << endl;
    cout << "╚═══════════════════════════════════════════════════════╩════════╝" << endl;
    pausarylimpiar();
}

void uiListarOrdenes(SistemaTaller *s){
    limpiar();
    cout << "=== LISTAR ORDENES ===" << endl;
    cout << "1. Listar por Estado (ABIERTA, CERRADA, ANULADA)" << endl;
    cout << "2. Listar por Vehiculo (Historial)" << endl;
    cout << "Seleccione una opcion: ";
    int op;
    cin >> op;

    switch (op) {
        case 1:{
            char estadoBuscado[15];
            cout << "Ingrese el estado (ABIERTA / CERRADA / ANULADA): ";
            cin >> estadoBuscado;
            cin.ignore(10000, '\n');
            int cantidad = 0;
    
            OrdenServicio** lista = listarOrdenesPorEstado(s, estadoBuscado, &cantidad);
            if (lista != nullptr && cantidad > 0){
                cout << "\n--- ORDENES CON ESTADO: " << estadoBuscado << " (" << cantidad << ") ---" << endl;
                for (int i = 0; i < cantidad; i++){
                    Vehiculo* v = buscarVehiculoPorID(s, lista[i]->idVehiculo); 
                    cout << "Orden #" << lista[i]->id << " | Vehiculo: #" << lista[i]->idVehiculo
                         << " (" << (v ? v->placa : "N/D") << ")"
                         << " | Apertura: " << lista[i]->fechaApertura
                         << " | Total: $" << calcularTotalOrden(lista[i]) << endl;
                }
                delete[] lista;
            } else{
                cout << "\nNo se encontraron ordenes con ese estado." << endl;
            }
            break; 
        }case 2:{
            int idV;
            cout << "Ingrese el ID del vehiculo: ";
            cin >> idV;
            cin.ignore(10000, '\n');
            int cantidad = 0;

            OrdenServicio** lista = listarOrdenesPorVehiculo(s, idV, &cantidad);
            if (lista != nullptr && cantidad > 0){
                cout << "\n--- HISTORIAL DE ÓRDENES DEL VEHÍCULO ID " << idV << " (" << cantidad << ") ---" << endl;
                for (int i = 0; i < cantidad; i++) {
                    cout << "Orden #" << lista[i]->id << " | Estado: " << lista[i]->estado
                         << " | Apertura: " << lista[i]->fechaApertura
                         << " | Total: $" << calcularTotalOrden(lista[i]) << endl;
                }
                delete[] lista;
            } else{
                cout << "\nNo hay ordenes registradas para ese vehiculo." << endl;
            }
            break;
        }default:{
            cin.ignore(10000, '\n');
            cout << "\nOpcion inválida." << endl;
            break;
        }
    } 
    pausarylimpiar();
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
                        case 1: 
                            uiRegistrarVehiculo(&miTaller);
                            break;
                        case 2:
                            uiBuscarVehiculo(&miTaller);
                            break;
                        case 3:
                            uiActualizarVehiculos(&miTaller);
                            break;
                        case 4:
                            uiEliminarVehiculo(&miTaller);
                            break;  
                        case 5:
                            uiListarVehiculos(&miTaller);
                            break;
                        case 6:
                            mostrarVehiculoIndividual(&miTaller);
                            break;
                        case 7:
                            mostrarListaVehiculosCompleta(&miTaller);
                            break;
                        case 0:
                            break;
                        default:
                            cin.ignore(10000, '\n');
                            cout << "Opcion invalida." << endl;
                            pausarylimpiar();
                            break;
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
                        case 1: 
                            uiRegistrarMecanico(&miTaller);
                            break;
                        case 2:
                            uiBuscarMecanicoID(&miTaller);
                            break;
                        case 3:
                            uiListarMecanicos(&miTaller);
                            break;
                        case 0:
                            break;  
                        default: 
                            cin.ignore(10000, '\n');
                            cout << "Opcion invalida." << endl;
                            pausarylimpiar();
                            break;
                    }
                }while(opcion2 != 0);
                limpiar();
                break;
            
            } case 3: {
                do {
                    limpiar();
                    menuRepuestos();
                    cin >> opcion2;

                    switch (opcion2) {
                        case 1:
                            uiRegistrarRepuesto(&miTaller);
                            break;
                        case 2:
                            uiBuscarRepuesto(&miTaller);
                            break;
                        case 3:
                            uiEntradaMercancia(&miTaller);
                            break;
                        case 4:
                            uiListarRepuestos(&miTaller);
                            break;
                        case 5:
                            uiCambiarPrecioRepuesto(&miTaller);
                            break;
                        case 6:
                            uiEliminarRepuesto(&miTaller);
                            break;
                        case 0:
                            break;
                        default:
                            cin.ignore(10000, '\n');
                            cout << "Opcion invalida." << endl;
                            pausarylimpiar();
                            break;
                    }
                } while(opcion2 != 0);
                limpiar();
                break;
            } case 4: {
                do{
                    limpiar();
                    menuÓrdenes();
                    cin >> opcion2;
                    switch (opcion2){
                        case 1:
                            uiAbrirOrden(&miTaller);
                            break; 
                        case 2:
                            uiAgregarRepuestoOrden(&miTaller);
                            break;
                        case 3:
                            uiQuitarRepuestoOrden(&miTaller);
                            break;
                        case 4:
                            uiCerrarOrden(&miTaller);
                            break;
                        case 5:
                            uiAnularOrden(&miTaller);
                            break;
                        case 6:
                            uiVerDetalleOrden(&miTaller);
                            break;
                        case 7:
                            uiListarOrdenes(&miTaller);
                            break;
                        case 0:
                            break;
                        default:
                            cout << "Opcion invalida." << endl;
                            pausarylimpiar();
                            break;
                    }
                }while(opcion2 != 0);
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

liberarSistema(&miTaller);
//miau
return 0;
}