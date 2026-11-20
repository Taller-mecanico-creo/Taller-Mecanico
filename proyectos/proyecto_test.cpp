#include <iostream>
#include <fstream> 
#include <ctime>
#include <windows.h>
#include <direct.h> // mkdir en Windows
using namespace std;

// const y rutas

const int MAX_DETALLES_ORDEN = 15; // Mínimo 15 líneas de detalle por orden
const int MAX_RESULTADOS     = 20; // Límite de resultados para búsquedas múltiples

const char* RUTA_TALLER    = "datos/taller.bin";
const char* RUTA_VEHICULOS = "datos/vehiculos.bin";
const char* RUTA_MECANICOS = "datos/mecanicos.bin";
const char* RUTA_REPUESTOS = "datos/repuestos.bin";
const char* RUTA_ORDENES   = "datos/ordenes.bin";

// enum (Códigos de retorno para especificar tipo de error)
enum CodigoResultado {
    RES_OK = 0,
    RES_ERROR_ARCHIVO,
    RES_NO_EXISTE,
    RES_ELIMINADO,
    RES_DUPLICADO,
    RES_DATOS_INVALIDOS,
    RES_TIENE_RELACIONES,
    RES_VEHICULO_NO_EXISTE,
    RES_VEHICULO_ELIMINADO,
    RES_MECANICO_NO_EXISTE,
    RES_MECANICO_DE_BAJA,
    RES_REPUESTO_NO_EXISTE,
    RES_REPUESTO_ELIMINADO,
    RES_ORDEN_YA_ABIERTA,
    RES_LIMITE_ORDENES_MECANICO,
    RES_KM_MENOR,
    RES_ORDEN_NO_ABIERTA,
    RES_ORDEN_YA_ANULADA,
    RES_STOCK_INSUFICIENTE,
    RES_LIMITE_DETALLES,
    RES_DETALLE_VACIO_SIN_MANO_OBRA
};

// structs

struct ArchivoHeader { // Registra de forma independiente en cada .bin (excepto taller)
    int cantidadRegistros; // Total histórico incluyendo borrado lógico
    int proximoID;
    int registrosActivos;  // Total sin borrado lógico (false)
    int version;
};

struct Taller{
    char nombre[100];
    char rif[20];
    char direccion[200];
    char telefono[20];
    time_t fechaCreacion;
    time_t fechaUltimaModificacion;
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

    bool eliminado;
    time_t fechaCreacion;
    time_t fechaUltimaModificacion;
};

struct Mecanico{
    int  id;
    char nombre[100];
    char cedula[20];
    char especialidad[20];
    char fechaIngreso[11];

    bool eliminado;        // true = dado de baja
    time_t fechaCreacion;
    time_t fechaUltimaModificacion;    
};

struct Repuesto {
    int     id;
    char    codigo[20];
    char    nombre[100];
    char    marca[50];
    float   precio;
    int     stock;
    int     stockMinimo;

    bool eliminado;        // true = dado de baja
    time_t fechaCreacion;
    time_t fechaUltimaModificacion; 
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

    //    
    DetalleOrden detalles [MAX_DETALLES_ORDEN];
    int           numDetalles;

    bool eliminado;        // no se elimina: siempre false
    time_t fechaCreacion;
    time_t fechaUltimaModificacion; 
};

//=============================
//      CAPA PERSISTENCIA
//=============================

void crearDirectorio(const char* ruta) {
    _mkdir(ruta);
    // Requiere validación?
}

long calcularOffset(int id, size_t tamanioEstructura) {
    int indiceFisico = id - 1;
    return static_cast<long>(sizeof(ArchivoHeader) + indiceFisico * tamanioEstructura);
}
// Inicializar archivo de entidad con su ArchivoHeader si no existe
bool inicializarArchivoEntidad(const char* ruta) {
    ifstream taller(ruta, ios::binary);
    if (taller.is_open()) {
        // Si existe verificamos que tenga el tamaño de al menos un ArchivoHeader
        taller.seekg(0, ios::end);
        streampos tam = taller.tellg();
        taller.close();
        if (tam >= static_cast<streampos>(sizeof(ArchivoHeader))) {
            return true; // Existe, listo para usar
        }
    }

    ofstream crear(ruta, ios::binary | ios::out);
    if (!crear.is_open()) {
        return false;
    }
    // Valores Iniciales
    ArchivoHeader headerInicial;
    headerInicial.cantidadRegistros = 0;
    headerInicial.proximoID         = 1;
    headerInicial.registrosActivos  = 0;
    headerInicial.version           = 1;
    
    crear.write(reinterpret_cast<const char*>(&headerInicial), sizeof(ArchivoHeader));
    bool exito = crear.good();
    crear.close();
    return exito;
}

bool leerHeader(const char* ruta, ArchivoHeader &header) {
    ifstream archivo(ruta, ios::binary);
    if (!archivo.is_open()) {
        return false;
    }
    archivo.seekg(0, ios::beg);
    archivo.read(reinterpret_cast<char*>(&header), sizeof(ArchivoHeader));
    bool exito = archivo.good();
    archivo.close();
    return exito;
}

bool actualizarHeader(const char* ruta, const ArchivoHeader &header) {
    fstream archivo(ruta, ios::binary | ios::in | ios::out);
    if (!archivo.is_open()) {
        return false;
    }
    archivo.seekp(0, ios::beg);
    archivo.write(reinterpret_cast<const char*>(&header), sizeof(ArchivoHeader));
    bool exito = archivo.good();
    archivo.close();
    return exito;
}

// Auxiliar que retorna el total histórico de registros desde el Header
int persistenciaObtenerTotalRegistros(const char* ruta) {
    ArchivoHeader h;
    if (!leerHeader(ruta, h)) {
        return 0;
    }
    return h.cantidadRegistros;
}

bool existeArchivoTaller() {
    ifstream archivo(RUTA_TALLER, ios::binary);
    if (!archivo.is_open()) return false;
    archivo.seekg(0, ios::end);
    bool valido = (archivo.tellg()) >= static_cast<streampos>(sizeof(Taller));
    archivo.close();
    return valido;
}

bool guardarTaller(Taller &datos) {
    ofstream archivo(RUTA_TALLER, ios::binary | ios::out);
    if (!archivo.is_open()) return false;

    time_t ahora = time(nullptr);
    datos.fechaCreacion           = ahora;
    datos.fechaUltimaModificacion = ahora;

    archivo.write(reinterpret_cast<const char*>(&datos), sizeof(Taller));
    bool exito = archivo.good();
    archivo.close();
    return exito;
}

bool leerTaller(Taller &datos) {
    ifstream archivo(RUTA_TALLER,ios::binary);
    if (!archivo.is_open()) return false;

    archivo.seekg(0, ios::beg);
    archivo.read(reinterpret_cast<char*>(&datos), sizeof(Taller));
    bool exito = archivo.good();
    archivo.close();
    return exito;
}

//============================================
//         CAPA DE PERSISTENCIA: VEHICULOS
//============================================

CodigoResultado persistenciaGuardarVehiculo(Vehiculo &nuevo, int &idAsignado){
    ArchivoHeader h;
    if (!leerHeader(RUTA_VEHICULOS, h)) return RES_ERROR_ARCHIVO;

    time_t ahora = time(nullptr);
    nuevo.id                      = h.proximoID;
    nuevo.eliminado               = false;
    nuevo.fechaCreacion           = ahora;
    nuevo.fechaUltimaModificacion = ahora;

    fstream archivo(RUTA_VEHICULOS, ios::binary | ios::in | ios::out); 
    if (!archivo.is_open()) return RES_ERROR_ARCHIVO;
    
    long pos = calcularOffset(nuevo.id, sizeof(Vehiculo));
    archivo.seekp(pos, ios::beg);
    archivo.write(reinterpret_cast<const char*>(&nuevo), sizeof(Vehiculo));
    if (!archivo.good()) {
        archivo.close();
        return RES_ERROR_ARCHIVO;
    }
    archivo.close();

    h.cantidadRegistros++;
    h.registrosActivos++;
    h.proximoID++;
    if (!actualizarHeader(RUTA_VEHICULOS, h)) return RES_ERROR_ARCHIVO;

    idAsignado = nuevo.id;
    return RES_OK;
}

CodigoResultado persistenciaLeerVehiculoPorID(int id, Vehiculo &registro, long* posBytesLeida = nullptr){
    ArchivoHeader h;
    if (!leerHeader(RUTA_VEHICULOS, h)) return RES_ERROR_ARCHIVO;
    if (id < 1 || id > h.cantidadRegistros) return RES_NO_EXISTE;

    ifstream archivo(RUTA_VEHICULOS, ios::binary);
    if (!archivo.is_open()) return RES_ERROR_ARCHIVO;

    long pos = calcularOffset(id, sizeof(Vehiculo));
    if (posBytesLeida != nullptr) *posBytesLeida = pos;

    archivo.seekg(pos, ios::beg);
    archivo.read(reinterpret_cast<char*>(&registro), sizeof(Vehiculo));
    bool ok = archivo.good();
    archivo.close();

    return ok ? RES_OK : RES_ERROR_ARCHIVO;
}

CodigoResultado persistenciaSobrescribirVehiculo(Vehiculo &registro){
    ArchivoHeader h;
    if (!leerHeader(RUTA_VEHICULOS, h)) return RES_ERROR_ARCHIVO;
    if (registro.id < 1 || registro.id > h.cantidadRegistros) return RES_NO_EXISTE;

    registro.fechaUltimaModificacion = time(nullptr);

    fstream archivo(RUTA_VEHICULOS, ios::binary | ios::in | ios::out);
    if (!archivo.is_open()) return RES_ERROR_ARCHIVO;

    long pos = calcularOffset(registro.id, sizeof(Vehiculo));
    archivo.seekp(pos, ios::beg);
    archivo.write(reinterpret_cast<const char*>(&registro), sizeof(Vehiculo));
    bool ok= archivo.good();
    archivo.close();

    return ok ? RES_OK : RES_ERROR_ARCHIVO;
}

CodigoResultado persistenciaEliminarVehiculo(int id){
    Vehiculo v;
    CodigoResultado res = persistenciaLeerVehiculoPorID(id, v);
    if (res != RES_OK) return res;
    if (v.eliminado) return RES_ELIMINADO;

    v.eliminado = true;
    res = persistenciaSobrescribirVehiculo(v);
    if (res != RES_OK) return res;

    ArchivoHeader h;
    if (!leerHeader(RUTA_VEHICULOS, h)) return RES_ERROR_ARCHIVO;
    if (h.registrosActivos > 0) h.registrosActivos--;
    if (!actualizarHeader(RUTA_VEHICULOS, h)) return RES_ERROR_ARCHIVO;

    return RES_OK;
}

CodigoResultado persistenciaContarVehiculosActivos(int &cantidad){
    ArchivoHeader h;
    if (!leerHeader(RUTA_VEHICULOS, h)) return RES_ERROR_ARCHIVO;
    cantidad = h.registrosActivos;
    return RES_OK;
}

//==========================================
//       CAPA DE PERSISTENCIA: MECANICOS
//==========================================

CodigoResultado persistenciaGuardarMecanico(Mecanico &nuevo, int &idAsignado) {
    ArchivoHeader h;
    if (!leerHeader(RUTA_MECANICOS, h)) return RES_ERROR_ARCHIVO;
    // Definimos el momento exacto en el que se guardó
    time_t ahora = time(nullptr);
    nuevo.id                      = h.proximoID;
    nuevo.eliminado               = false;
    nuevo.fechaCreacion           = ahora;
    nuevo.fechaUltimaModificacion = ahora;
    
    fstream archivo(RUTA_MECANICOS, ios::binary | ios::in | ios::out); 
    if (!archivo.is_open()) return RES_ERROR_ARCHIVO;
    // Calculamos la posición exacta en bytes para guardar
    long pos = calcularOffset(nuevo.id, sizeof(Mecanico));
    archivo.seekp(pos, ios::beg);
    archivo.write(reinterpret_cast<const char*>(&nuevo), sizeof(Mecanico));
    if (!archivo.good()) {
        archivo.close();
        return RES_ERROR_ARCHIVO;
    }
    archivo.close();
    // Aumentamos contadores del header post guardado
    h.cantidadRegistros++;
    h.registrosActivos++;
    h.proximoID++;
    if (!actualizarHeader(RUTA_MECANICOS, h)) return RES_ERROR_ARCHIVO;

    idAsignado = nuevo.id;
    return RES_OK;
}

CodigoResultado persistenciaLeerMecanicoPorID(int id, Mecanico &registro, long*posBytesLeida = nullptr) {
    ArchivoHeader h;
    if (!leerHeader(RUTA_MECANICOS, h)) return RES_ERROR_ARCHIVO;
    // Validamos que el id existe 
    if (id < 1 || id > h.cantidadRegistros) return RES_NO_EXISTE;

    ifstream archivo(RUTA_MECANICOS, ios::binary);
    if (!archivo.is_open()) return RES_ERROR_ARCHIVO;
    // Calculamos la posición en la que se encuentra el registro a buscar
    long pos = calcularOffset(id, sizeof(Mecanico));
    if (posBytesLeida != nullptr) *posBytesLeida = pos;
    // Leemos solamente los bytes correspondientes a ese registro
    archivo.seekg(pos, ios::beg);
    archivo.read(reinterpret_cast<char*>(&registro), sizeof(Mecanico));
    bool ok = archivo.good();
    archivo.close();

    return ok ? RES_OK : RES_ERROR_ARCHIVO;
}

CodigoResultado persistenciaSobrescribirMecanico(Mecanico &registro) {
    ArchivoHeader h;

    if (!leerHeader(RUTA_MECANICOS, h)) return RES_ERROR_ARCHIVO;
    if (registro.id < 1 || registro.id > h.cantidadRegistros) return RES_NO_EXISTE;
    
    registro.fechaUltimaModificacion = time(nullptr);

    fstream archivo(RUTA_MECANICOS, ios::binary | ios::in | ios::out);
    if (!archivo.is_open()) return RES_ERROR_ARCHIVO;
    // Calculamos la posición en la que se encuentra el registro a buscar
    long pos = calcularOffset(registro.id, sizeof(Mecanico));
    // Abrimos desde el principio hasta recorrer de forma exacta lo calculado
    archivo.seekp(pos, ios::beg);
    // Sobreescribimos solamente los bytes correspondientes a ese registro
    archivo.write(reinterpret_cast<const char*>(&registro), sizeof(Mecanico));
    bool ok= archivo.good();
    archivo.close();

    return ok ? RES_OK : RES_ERROR_ARCHIVO;
}

CodigoResultado persistenciaEliminarMecanico(int id) {
    Mecanico m;
    CodigoResultado res = persistenciaLeerMecanicoPorID(id, m);
    if (res != RES_OK) return res;
    if (m.eliminado) return RES_ELIMINADO;

    m.eliminado = true;
    res = persistenciaSobrescribirMecanico(m);
    if (res != RES_OK) return res;

    ArchivoHeader h;
    if (!leerHeader(RUTA_MECANICOS, h)) return RES_ERROR_ARCHIVO;
    if (h.registrosActivos > 0) h.registrosActivos--;
    if (!actualizarHeader(RUTA_MECANICOS, h)) return RES_ERROR_ARCHIVO;

    return RES_OK;
}

int persistenciaContarMecanicosActivos() {
    ArchivoHeader h;
    if (!leerHeader(RUTA_MECANICOS, h)) return 0;
    return h.registrosActivos;
}

//============================================
//       CAPA DE PERSISTENCIA: REPUESTOS
//============================================

CodigoResultado persistenciaGuardarRepuesto(Repuesto &nuevo, int &idAsignado){
    ArchivoHeader h;
    if (!leerHeader(RUTA_REPUESTOS, h)) return RES_ERROR_ARCHIVO;

    time_t ahora = time(nullptr);
    nuevo.id                      = h.proximoID;
    nuevo.eliminado               = false;
    nuevo.fechaCreacion           = ahora;
    nuevo.fechaUltimaModificacion = ahora;

    fstream archivo(RUTA_REPUESTOS, ios::binary | ios::in | ios::out);
    if (!archivo.is_open()) return RES_ERROR_ARCHIVO;

    long pos = calcularOffset(nuevo.id, sizeof(Repuesto));
    archivo.seekp(pos, ios::beg);
    archivo.write(reinterpret_cast<const char*>(&nuevo), sizeof(Repuesto));
    if (!archivo.good()) {
        archivo.close();
        return RES_ERROR_ARCHIVO;
    }
    archivo.close();

    h.cantidadRegistros++;
    h.registrosActivos++;
    h.proximoID++;
    if (!actualizarHeader(RUTA_REPUESTOS, h)) return RES_ERROR_ARCHIVO;

    idAsignado = nuevo.id;
    return RES_OK;
}

CodigoResultado persistenciaLeerRepuestoPorID(int id, Repuesto &registro, long* posBytesLeida = nullptr){
    ArchivoHeader h;
    if (!leerHeader(RUTA_REPUESTOS, h)) return RES_ERROR_ARCHIVO;
    if (id < 1 || id > h.cantidadRegistros) return RES_NO_EXISTE;

    ifstream archivo(RUTA_REPUESTOS, ios::binary);
    if (!archivo.is_open()) return RES_ERROR_ARCHIVO;

    long pos = calcularOffset(id, sizeof(Repuesto));
    if (posBytesLeida != nullptr) *posBytesLeida = pos;

    archivo.seekg(pos, ios::beg);
    archivo.read(reinterpret_cast<char*>(&registro), sizeof(Repuesto));
    bool ok = archivo.good();
    archivo.close();

    return ok ? RES_OK : RES_ERROR_ARCHIVO;
}

CodigoResultado persistenciaSobrescribirRepuesto(Repuesto &registro){
    ArchivoHeader h;
    if (!leerHeader(RUTA_REPUESTOS, h)) return RES_ERROR_ARCHIVO;
    if (registro.id < 1 || registro.id > h.cantidadRegistros) return RES_NO_EXISTE;

    registro.fechaUltimaModificacion = time(nullptr);

    fstream archivo(RUTA_REPUESTOS, ios::binary | ios::in | ios::out);
    if (!archivo.is_open()) return RES_ERROR_ARCHIVO;
    
    long pos = calcularOffset(registro.id, sizeof(Repuesto));
    archivo.seekp(pos, ios::beg);
    archivo.write(reinterpret_cast<const char*>(&registro), sizeof(Repuesto));
    bool ok = archivo.good();
    archivo.close();

    return ok ? RES_OK : RES_ERROR_ARCHIVO;
}

CodigoResultado persistenciaEliminarRepuesto(int id){
    Repuesto r;
    CodigoResultado res = persistenciaLeerRepuestoPorID(id, r);
    if (res != RES_OK) return res;
    if (r.eliminado) return RES_ELIMINADO;

    r.eliminado = true;
    res = persistenciaSobrescribirRepuesto(r);
    if (res != RES_OK) return res;

    ArchivoHeader h;
    if (!leerHeader(RUTA_REPUESTOS, h)) return RES_ERROR_ARCHIVO;
    if (h.registrosActivos > 0) h.registrosActivos--;
    if (!actualizarHeader(RUTA_REPUESTOS, h)) return RES_ERROR_ARCHIVO;

    return RES_OK;
}

int persistenciaContarRepuestosActivos(){
    ArchivoHeader h;
    if (!leerHeader(RUTA_REPUESTOS, h)) return 0;
    return h.registrosActivos;
}

// verificar si orden abierta mecanico deberia ir por aca

//=========================================
//       CAPA DE PERSISTENCIA: ORDENES
//=========================================

CodigoResultado persistenciaGuardarOrden(OrdenServicio &nuevo, int &idAsignado){
    ArchivoHeader h;
    if (!leerHeader(RUTA_ORDENES, h)) return RES_ERROR_ARCHIVO;

    time_t ahora = time(nullptr);
    nuevo.id                      = h.proximoID;
    nuevo.eliminado               = false; //las ordenes no se eleminan fisicamente (siempre flase, se anulan o se cierran) 
    nuevo.fechaCreacion           = ahora;
    nuevo.fechaUltimaModificacion = ahora;

    fstream archivo(RUTA_ORDENES, ios::binary | ios::in | ios::out);
    if (!archivo.is_open()) return RES_ERROR_ARCHIVO;

    long pos = calcularOffset(nuevo.id, sizeof(OrdenServicio));
    archivo.seekp(pos, ios::beg);
    archivo.write(reinterpret_cast<const char*>(&nuevo), sizeof(OrdenServicio));
    if (!archivo.good()) {
        archivo.close();
        return RES_ERROR_ARCHIVO;
    }
    archivo.close();

    h.cantidadRegistros++;
    h.registrosActivos++;
    h.proximoID++;
    if (!actualizarHeader(RUTA_ORDENES, h)) return RES_ERROR_ARCHIVO;

    idAsignado = nuevo.id;
    return RES_OK;
}

CodigoResultado persistenciaLeerOrdenPorID(int id, OrdenServicio &registro, long* posBytesLeida = nullptr){
    ArchivoHeader h;
    if (!leerHeader(RUTA_ORDENES, h)) return RES_ERROR_ARCHIVO;
    if (id < 1 || id > h.cantidadRegistros) return RES_NO_EXISTE;

    ifstream archivo(RUTA_ORDENES, ios::binary);
    if (!archivo.is_open()) return RES_ERROR_ARCHIVO;

    long pos = calcularOffset(id, sizeof(OrdenServicio));
    if (posBytesLeida != nullptr) *posBytesLeida = pos;

    archivo.seekg(pos, ios::beg);
    archivo.read(reinterpret_cast<char*>(&registro), sizeof(OrdenServicio));
    bool ok = archivo.good();
    archivo.close();

    return ok ? RES_OK : RES_ERROR_ARCHIVO;
}

CodigoResultado persistenciaSobrescribirOrden(OrdenServicio &registro){
    ArchivoHeader h;
    if (!leerHeader(RUTA_ORDENES, h)) return RES_ERROR_ARCHIVO;
    if (registro.id < 1 || registro.id > h.cantidadRegistros) return RES_NO_EXISTE;

    registro.fechaUltimaModificacion = time(nullptr);

    fstream archivo(RUTA_ORDENES, ios::binary | ios::in | ios::out);
    if (!archivo.is_open()) return RES_ERROR_ARCHIVO;
    
    long pos = calcularOffset(registro.id, sizeof(OrdenServicio));
    archivo.seekp(pos, ios::beg);
    archivo.write(reinterpret_cast<const char*>(&registro), sizeof(OrdenServicio));
    bool ok = archivo.good();
    archivo.close();

    return ok ? RES_OK : RES_ERROR_ARCHIVO;
}

CodigoResultado persistenciaContarOrdenesActivas(int &cantidad){
    ArchivoHeader h;
    if (!leerHeader(RUTA_ORDENES, h)) return RES_ERROR_ARCHIVO;
    cantidad = h.registrosActivos;
    return RES_OK;
}

bool inicializarArchivosSistema() {
    crearDirectorio("datos");
    return inicializarArchivoEntidad(RUTA_VEHICULOS) &&
           inicializarArchivoEntidad(RUTA_MECANICOS) &&
           inicializarArchivoEntidad(RUTA_REPUESTOS) &&
           inicializarArchivoEntidad(RUTA_ORDENES);
}

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
    if (!(cin >> resp)) {
        cin.clear();
        cin.ignore(10000, '\n');
        return false;
    }
    return (resp == 'S' || resp == 's');
}

//==========================================
//       CAPA DE LOGICA: VEHICULOS
//==========================================

// Declaración previa para que lógica la reconozca
CodigoResultado buscarVehiculoPorPlaca(const char* placa, Vehiculo &salida);

CodigoResultado agregarVehiculo(Vehiculo datos, int &idAsignado){
    idAsignado = 0;
    
    if (datos.placa[0] == '\0' || datos.marca[0] == '\0' || datos.modelo[0] == '\0' ||
        datos.nombrePropietario[0] == '\0' || datos.cedulaPropietario[0] == '\0' || 
        datos.telefonoPropietario[0] == '\0') {
        return RES_DATOS_INVALIDOS;
    }

    // Validamos los formatos de placa, cedula y fecha
    if (!validarPlaca(datos.placa) || !validarCedula(datos.cedulaPropietario)){
        return RES_DATOS_INVALIDOS;
    }

    //Validamos anio (1950 - 2027) y kilometraje (>=0)
    if(datos.anio < 1950 || datos.anio > 2027 || datos.kilometraje < 0){
        return RES_DATOS_INVALIDOS;
    }   
    
    //Validar que la placa no este duplicada
    Vehiculo existente;
    if(buscarVehiculoPorPlaca(datos.placa, existente) == RES_OK){
        return RES_DUPLICADO;
    }

    return persistenciaGuardarVehiculo(datos, idAsignado);
}

CodigoResultado buscarVehiculoPorID(int id, Vehiculo &salida, long* posBytesLeida){
    CodigoResultado res = persistenciaLeerVehiculoPorID(id, salida, posBytesLeida);
    if (res != RES_OK) return res; 
    if (salida.eliminado) return RES_ELIMINADO;
    return RES_OK;
}

CodigoResultado buscarVehiculoPorPlaca(const char* placa, Vehiculo &salida){
    if (placa == nullptr || placa[0] == '\0') return RES_DATOS_INVALIDOS;

    int totalVehiculos = persistenciaObtenerTotalRegistros(RUTA_VEHICULOS);
    for (int i = 1; i <= totalVehiculos; i++) {
        Vehiculo v;
        if (persistenciaLeerVehiculoPorID(i, v) == RES_OK){
            if (!v.eliminado && compararCadenasExactas(v.placa, placa)) {
                salida = v;
                return RES_OK;
            }
        }
    }
    return RES_NO_EXISTE;
}

CodigoResultado buscarVehiculosPorPropietario(const char* subcadena, Vehiculo resultados[], int &cantidad, bool &hayMas){
    cantidad = 0;
    hayMas = false;

    if (subcadena == nullptr || subcadena[0] == '\0') return RES_DATOS_INVALIDOS;

    int totalVehiculos = persistenciaObtenerTotalRegistros(RUTA_VEHICULOS);
    if (totalVehiculos == 0) return RES_NO_EXISTE;

    for (int id = 1; id <= totalVehiculos; id++) {
        Vehiculo v;
        if (persistenciaLeerVehiculoPorID(id, v) == RES_OK) {
            if (!v.eliminado && compararSubcadenaIgnoreCase(v.nombrePropietario, subcadena)) {
                if (cantidad < MAX_RESULTADOS) {
                    resultados[cantidad] = v;
                    cantidad++;
                } else {
                    hayMas = true;
                    break;
                }
            }
        }
    }
    return (cantidad > 0) ? RES_OK : RES_NO_EXISTE;
}

CodigoResultado listarVehiculos(Vehiculo resultados[], int &cantidad, bool &hayMas){
    cantidad = 0;
    hayMas = false;

    int totalVehiculos = persistenciaObtenerTotalRegistros(RUTA_VEHICULOS);
    if (totalVehiculos == 0) return RES_NO_EXISTE;

    for (int id = 1; id <= totalVehiculos; id++) {
        Vehiculo v;
        if (persistenciaLeerVehiculoPorID(id, v) == RES_OK) {
            if (!v.eliminado) {
                if (cantidad < MAX_RESULTADOS) {
                    resultados[cantidad] = v;
                    cantidad++;
                } else {
                    hayMas = true;
                    break;
                }
            }
        }
    }
    return (cantidad > 0) ? RES_OK : RES_NO_EXISTE;
}

CodigoResultado actualizarVehiculo(int id, Vehiculo nuevosDatos){
    Vehiculo actual;
    CodigoResultado res = persistenciaLeerVehiculoPorID(id, actual);
    if (res != RES_OK) return res;

    // Validar que los nuevos datos sean validos
    if (nuevosDatos.anio < 1950 || nuevosDatos.anio > 2027){
        return RES_DATOS_INVALIDOS;
    }

    if (nuevosDatos.marca[0] == '\0' || nuevosDatos.modelo[0] == '\0' ||
        nuevosDatos.nombrePropietario[0] == '\0' || !validarCedula(nuevosDatos.cedulaPropietario) ||
        nuevosDatos.telefonoPropietario[0] == '\0') {
        return RES_DATOS_INVALIDOS;
    }
    //Actualizamos los campos numericos directamente
    actual.anio = nuevosDatos.anio;

    //Copiamos la marca caracter por caracter
    int i = 0;
    while(nuevosDatos.marca[i] != '\0' && i < 49){
        actual.marca[i] = nuevosDatos.marca[i];
        i++;
    }
    actual.marca[i] = '\0';

    //Copiamos el modelo caracter por caracter
    i = 0;
    while(nuevosDatos.modelo[i] != '\0' && i < 49){
        actual.modelo[i] = nuevosDatos.modelo[i];
        i++;
    }
    actual.modelo[i] = '\0';

    //Copiamos el nombre del propietario caracter a caracter
    i = 0;
    while(nuevosDatos.nombrePropietario[i] != '\0' && i < 99){
        actual.nombrePropietario[i] = nuevosDatos.nombrePropietario[i];
        i++;
    }
    actual.nombrePropietario[i] = '\0';

    //Copiamos la cedula del propietario caracter a caracter
    i = 0;
    while (nuevosDatos.cedulaPropietario[i] != '\0' && i < 19) {
        actual.cedulaPropietario[i] = nuevosDatos.cedulaPropietario[i];
        i++;
    }
    actual.cedulaPropietario[i] = '\0';

    //Copiamos la cedula del propietario caracter a caracter
    i = 0;
    while (nuevosDatos.telefonoPropietario[i] != '\0' && i < 19) {
        actual.telefonoPropietario[i] = nuevosDatos.telefonoPropietario[i];
        i++;
    }
    actual.telefonoPropietario[i] = '\0';

    return persistenciaSobrescribirVehiculo(actual);
}

CodigoResultado eliminarVehiculo(int id){
    Vehiculo v;
    CodigoResultado res = persistenciaLeerVehiculoPorID(id, v);
    if (res != RES_OK) return res;

    int totalOrdenes = persistenciaObtenerTotalRegistros(RUTA_ORDENES);
    for (int idOrden = 1; idOrden <= totalOrdenes; idOrden++){
        OrdenServicio o;
        if (persistenciaLeerOrdenPorID(idOrden, o) == RES_OK){
            if (o.idVehiculo == id){
                return RES_TIENE_RELACIONES;
            }
        }
    }
    return persistenciaEliminarVehiculo(id);
}

//==========================================
//       CAPA DE LOGICA: MECANICOS
//==========================================

int ContarOrdenesAbiertasMecanico(int idMecanico) {
    // Le preguntamos a la persistencia cuántas órdenes históricas hay en total
    int totalOrdenes = persistenciaObtenerTotalRegistros(RUTA_ORDENES);
    int contador = 0;

    // Recorremos por ID (1 a totalOrdenes) cargando una orden a la vez
    for (int id = 1; id <= totalOrdenes; id++) {
        OrdenServicio o;
        if (persistenciaLeerOrdenPorID(id, o) == RES_OK) {
            // Evalúa que pertenezca al mecánico y esté ABIERTA
            if (!o.eliminado && o.idMecanico == idMecanico && compararCadenasExactas(o.estado, "ABIERTA")) {
                contador++;
            }
        }
    }
    return contador;
}
// Auxiliar que verifica que no existen duplicados (busca mecánico activo)
CodigoResultado buscarMecanicoPorCedula(const char* cedula, Mecanico &salida) {
    int totalMecanicos = persistenciaObtenerTotalRegistros(RUTA_MECANICOS);
    for (int id = 1; id <= totalMecanicos; id++) {
        Mecanico m;
        if (persistenciaLeerMecanicoPorID(id, m) == RES_OK) {
            if (!m.eliminado && compararCadenasExactas(m.cedula, cedula)) {
                salida = m;
                return RES_OK;
            }
        }
    }
    return RES_NO_EXISTE;
}
// Distingue si es activo (RES_OK) o dado de baja (RES_ELIMINADO). Entrega con 'salida'.
CodigoResultado ObtenerMecanicoPorID(int id, Mecanico &salida, long* posBytesLeida = nullptr) {   
    CodigoResultado res = persistenciaLeerMecanicoPorID(id, salida, posBytesLeida);
    if (res != RES_OK) return res;
    if (salida.eliminado) return RES_ELIMINADO; // 'salida' ya tiene los datos para órdenes históricas
    return RES_OK;
}

CodigoResultado registrarMecanico(Mecanico datos, int &idAsignado) {
    // Validamos que los campos no estén vacíos y la especialidad sea válida
    if (datos.nombre[0] == '\0' || !validarCedula(datos.cedula) || !validarFecha(datos.fechaIngreso)) {
        return RES_DATOS_INVALIDOS;
    }
    // Validar que la especialidad sea una de las 5 permitidas caracter por caracter
    const char* permitidas[5] = {"MOTOR", "FRENOS", "ELECTRICIDAD", "SUSPENSION", "GENERAL"};
    bool especialidadValida = false;
    for (int k = 0; k < 5; k++){
        if (compararCadenasExactas(datos.especialidad, permitidas[k])) {
            especialidadValida = true;
            break;
        }
    }
    if (!especialidadValida) return RES_DATOS_INVALIDOS;
    // Validar que la cédula no este duplicada
    Mecanico existente;
    if (buscarMecanicoPorCedula(datos.cedula, existente) == RES_OK) {
        return RES_DUPLICADO; 
    }
    return persistenciaGuardarMecanico(datos, idAsignado);
}
// Activos y Dados de Baja
CodigoResultado listarMecanicos(Mecanico resultados[], int &cantidad, bool &hayMas) {
    // Definimos cantidad de mecánicos en 0 y negamos que hayan más
        cantidad = 0;
        hayMas   = false;

    int totalMecanicos = persistenciaObtenerTotalRegistros(RUTA_MECANICOS);
    if (totalMecanicos == 0) return RES_NO_EXISTE;
    // Solicitamos un mecánico a la vez por su ID, mediante la persistencia
    for(int id = 1; id <= totalMecanicos; id++){
        Mecanico m;
        if (persistenciaLeerMecanicoPorID(id, m) == RES_OK) {
            if (cantidad < MAX_RESULTADOS) {
                resultados[cantidad] = m;
                cantidad++;
            } else {
                hayMas = true;
                break; // Límite alcanzado, dejamos de leer y avisamos que hay más
            }  
        }
    }
    return (cantidad > 0) ? RES_OK : RES_NO_EXISTE;
}
// Dar de baja a un mecánico (Borrado lógico si no tiene órdenes abiertas)
CodigoResultado DarDeBajaMecanico(int id) {
    Mecanico m;
    CodigoResultado res = ObtenerMecanicoPorID(id, m);
    if (res != RES_OK) return res; // Retorna RES_NO_EXISTE o RES_ELIMINADO si ya estaba de baja

    // Rechazado si tiene órdenes abiertas
    if (ContarOrdenesAbiertasMecanico(id) > 0) {
        return RES_TIENE_RELACIONES;
    }
    return persistenciaEliminarMecanico(id);
}

//==========================================
//       CAPA DE LOGICA: REPUESTOS
//==========================================

CodigoResultado buscarRepuestoPorID(int id, Repuesto &salida, long* posBytesLeida = nullptr) {
    CodigoResultado res = persistenciaLeerRepuestoPorID(id, salida, posBytesLeida);
    if (res != RES_OK) return res;
    if (salida.eliminado) return RES_ELIMINADO;
    return RES_OK;
}

CodigoResultado buscarRepuestoPorCodigo(const char* codigo, Repuesto &salida) {
    int totalRepuestos = persistenciaObtenerTotalRegistros(RUTA_REPUESTOS);
    // Recorremos en busca del código exacto y verificamos que el repuesto está activo
    for(int id = 1; id <= totalRepuestos; id++){
        Repuesto r;
        if (persistenciaLeerRepuestoPorID(id, r) == RES_OK) {
            if (!r.eliminado && compararCadenasExactas(r.codigo, codigo)) {
                salida = r;
                return RES_OK;
            }
        }
    }
    return RES_NO_EXISTE;
}

CodigoResultado buscarRepuestosPorNombreParcial(const char* subcadena, Repuesto resultados[], int &cantidad, bool &hayMas) {
    // Definimos cantidad de repuestos en 0 y negamos que hayan más
        cantidad = 0;
        hayMas   = false;

    int totalRepuestos = persistenciaObtenerTotalRegistros(RUTA_REPUESTOS);
    if (totalRepuestos == 0) {
        return RES_NO_EXISTE;
    }
    // Solicitamos un repuesto a la vez por su ID, verificamos su estado y buscamos coincidencias
    for (int id = 1; id <= totalRepuestos; id++) {
        Repuesto r;
        if (persistenciaLeerRepuestoPorID(id, r) == RES_OK) {
            if (!r.eliminado && compararSubcadenaIgnoreCase(r.nombre, subcadena)) {
                if (cantidad < MAX_RESULTADOS) {
                    resultados[cantidad] = r;
                    cantidad++;
                } else {
                    hayMas = true;
                    break; // Límite alcanzado, dejamos de leer y avisamos que hay más
                }
            }
        }
    }
    return (cantidad > 0) ? RES_OK : RES_NO_EXISTE;
}

CodigoResultado registrarRepuesto(Repuesto datos, int &idAsignado){
    //Validaciones de negocio (Precio > 0, Stock >= 0, Stock minimo >=0, Código único)
    if (datos.codigo[0] == '\0' || datos.nombre[0] == '\0' || datos.marca[0] == '\0' ||
        datos.precio <= 0.0f || datos.stock < 0 || datos.stockMinimo < 0) {
        return RES_DATOS_INVALIDOS;
    }
    // Verificamos si ya existe en el inventario
    Repuesto existente;
    if (buscarRepuestoPorCodigo(datos.codigo, existente) == RES_OK) return RES_DUPLICADO;

    return persistenciaGuardarRepuesto(datos, idAsignado);
}

CodigoResultado registrarEntradaMercancia(int idRepuesto, int cantidad, int &nuevoStock){
    // Validamos el rango y el id correspondiente a la mercancia
    if (cantidad <= 0) return RES_DATOS_INVALIDOS;
    Repuesto r; 
    CodigoResultado res = buscarRepuestoPorID(idRepuesto, r);
    if (res != RES_OK) return res; // Entrega el error exacto
    // Definimos los nuevos valores de stock y guardamos
    r.stock += cantidad;
    nuevoStock = r.stock;
    return persistenciaSobrescribirRepuesto(r);
}

CodigoResultado listarRepuestosActivos(Repuesto resultados[], int &cantidad, bool &hayMas) {
    cantidad = 0;
    hayMas   = false;

    int totalRepuestos = persistenciaObtenerTotalRegistros(RUTA_REPUESTOS);

    for (int id = 1; id <= totalRepuestos; id++) {
        Repuesto r;
        if (persistenciaLeerRepuestoPorID(id, r) == RES_OK) {
            if (!r.eliminado) {
                if (cantidad < MAX_RESULTADOS) {
                    resultados[cantidad] = r;
                    cantidad++;
                } else {
                    hayMas = true;
                    break;
                }
            }
        }
    }
    return (cantidad > 0) ? RES_OK : RES_NO_EXISTE;
}

CodigoResultado cambiarPrecioRepuesto(int idRepuesto, float nuevoPrecio){
    if (nuevoPrecio <= 0.0f) return RES_DATOS_INVALIDOS;
    Repuesto r;
    CodigoResultado res = buscarRepuestoPorID(idRepuesto, r);
    if (res != RES_OK) return res;

    r.precio = nuevoPrecio; //no afecta ordenes historicas ya cerradas
    return persistenciaSobrescribirRepuesto(r);
}

CodigoResultado eliminarRepuesto(int idRepuesto) {
    Repuesto r;
    CodigoResultado res = buscarRepuestoPorID(idRepuesto, r);
    if (res != RES_OK) return res;
    
    // Verificamos a través de la persistencia si aparece en alguna orden
    int totalOrdenes = persistenciaObtenerTotalRegistros(RUTA_ORDENES);
    for (int idOrden = 1; idOrden <= totalOrdenes; idOrden++) {
        OrdenServicio o;
        if (persistenciaLeerOrdenPorID(idOrden, o) == RES_OK) {
            if (!o.eliminado) {
                for (int j = 0; j < o.numDetalles; j++) {
                    if (o.detalles[j].idRepuesto == idRepuesto) {
                        return RES_TIENE_RELACIONES; // Rechazado: usado en una orden
                    }
                }
            }
        }
    }
    return persistenciaEliminarRepuesto(idRepuesto);
}

//==========================================
//       CAPA DE LOGICA: ORDENES
//==========================================

// Funcion auxiliar que verifica si un vehiculo ya tiene una orden abierta
CodigoResultado buscarOrdenAbiertaVehiculo(int idVehiculo, OrdenServicio &salida, int* idOrdenEncontrada = nullptr){
    if (idOrdenEncontrada != nullptr) {
        *idOrdenEncontrada = 0;
    }

    if (idVehiculo <= 0) {
        return RES_DATOS_INVALIDOS;
    }

    int totalOrdenes = persistenciaObtenerTotalRegistros(RUTA_ORDENES);
    if (totalOrdenes == 0) {
        return RES_NO_EXISTE;
    }

    for (int id = 1; id <= totalOrdenes; id++) {
        OrdenServicio o;
        if (persistenciaLeerOrdenPorID(id, o) == RES_OK) {
            if (!o.eliminado && o.idVehiculo == idVehiculo && compararCadenasExactas(o.estado, "ABIERTA")) {
                salida = o;
                if (idOrdenEncontrada != nullptr) {
                    *idOrdenEncontrada = o.id;
                }
                return RES_OK;
            }
        }
    }

    return RES_NO_EXISTE;
}

CodigoResultado abrirOrden (int idVehiculo, int idMecanico, const char* fechaApertura, const char* descripcionFalla, int kilometrajeIngreso, int &idAsignado){
    idAsignado = 0;

    // Validamos que la fecha y la descripcion de la falla no esten vacias
    if (fechaApertura == nullptr || descripcionFalla == nullptr ||
        !validarFecha(fechaApertura) || descripcionFalla[0] == '\0'){
        return RES_DATOS_INVALIDOS;
    }

    Vehiculo v;
    CodigoResultado resV = persistenciaLeerVehiculoPorID(idVehiculo, v);
    if (resV == RES_NO_EXISTE || resV == RES_ERROR_ARCHIVO) return RES_VEHICULO_NO_EXISTE;
    if (v.eliminado) return RES_VEHICULO_ELIMINADO;

    Mecanico m;
    CodigoResultado resM = persistenciaLeerMecanicoPorID(idMecanico, m);
    if (resM == RES_NO_EXISTE || resM == RES_ERROR_ARCHIVO) return RES_MECANICO_NO_EXISTE;
    if (m.eliminado) return RES_MECANICO_DE_BAJA;

    OrdenServicio ordenAbierta;
    if (buscarOrdenAbiertaVehiculo(idVehiculo, ordenAbierta) == RES_OK){
        return RES_ORDEN_YA_ABIERTA; // El vehículo ya tiene una orden abierta
    }

    if (ContarOrdenesAbiertasMecanico(idMecanico) >= 3) {
        return RES_LIMITE_ORDENES_MECANICO; // El mecánico tiene 3 órdenes abiertas
    }
    if (kilometrajeIngreso < v.kilometraje) {
        return RES_KM_MENOR; // El kilometraje de ingreso es menor al actual del vehículo
    }

    v.kilometraje = kilometrajeIngreso;
    CodigoResultado resActV = persistenciaSobrescribirVehiculo(v);
    if (resActV != RES_OK) return resActV;

    OrdenServicio nueva{};
    nueva.idVehiculo = idVehiculo;
    nueva.idMecanico = idMecanico;
    nueva.kilometrajeIngreso = kilometrajeIngreso;
    nueva.manoObra = 0.0f;
    nueva.numDetalles = 0;
    nueva.eliminado = false;
    nueva.fechaCierre[0] = '\0';

    int i = 0;
    while (fechaApertura[i] != '\0' && i < 10) {
        nueva.fechaApertura[i] = fechaApertura[i];
        i++;
    }
    nueva.fechaApertura[i] = '\0';

    const char* estInicial = "ABIERTA";
    i = 0;
    while (estInicial[i] != '\0' && i < 11) {
        nueva.estado[i] = estInicial[i];
        i++;
    }
    nueva.estado[i] = '\0';

    i = 0;
    while (descripcionFalla[i] != '\0' && i < 199) {
        nueva.descripcionFalla[i] = descripcionFalla[i];
        i++;
    }
    nueva.descripcionFalla[i] = '\0';

    return persistenciaGuardarOrden(nueva, idAsignado);
}

CodigoResultado buscarOrdenPorID(int id, OrdenServicio &salida, long* posBytesLeida = nullptr){
    if(id <= 0){
        return RES_DATOS_INVALIDOS;
    }

    CodigoResultado res = persistenciaLeerOrdenPorID(id, salida, posBytesLeida);
    if (res != RES_OK){
        return res;
    }

    return RES_OK;
}

CodigoResultado agregarRepuestoAOrden(int idOrden, int idRepuesto, int cantidad){
    // La cantidad debe ser mayor a 0
    if (cantidad <= 0){
        return RES_DATOS_INVALIDOS;
    }

    OrdenServicio orden;
    CodigoResultado resO = buscarOrdenPorID(idOrden, orden);
    if (resO != RES_OK){
        return resO;
    }

    if (!compararCadenasExactas(orden.estado, "ABIERTA")){
        return RES_ORDEN_NO_ABIERTA;
    }
    // Verificamos que el repuesto exista y tenga stock suficiente
    Repuesto rep;
    CodigoResultado resR = buscarRepuestoPorID(idRepuesto, rep);
    if (resR == RES_NO_EXISTE || resR == RES_ERROR_ARCHIVO){
        return RES_REPUESTO_NO_EXISTE;
    }
    if (rep.eliminado) {
        return RES_REPUESTO_ELIMINADO;
    }
    if (rep.stock < cantidad) {
        return RES_STOCK_INSUFICIENTE;
    }

    int idxExistente = -1;
    for (int i = 0; i < orden.numDetalles; i++) {
        if (orden.detalles[i].idRepuesto == idRepuesto) {
            idxExistente = i;
            break;
        }
    }

    if (idxExistente == -1 && orden.numDetalles >= MAX_DETALLES_ORDEN) {
        return RES_LIMITE_DETALLES;
    }

    rep.stock -= cantidad;
    CodigoResultado resRep = persistenciaSobrescribirRepuesto(rep);
    if (resRep != RES_OK) {
        return resRep;
    }

    if (idxExistente != -1) {
        orden.detalles[idxExistente].cantidad += cantidad;
    } else {
        orden.detalles[orden.numDetalles].idRepuesto = idRepuesto;
        orden.detalles[orden.numDetalles].cantidad = cantidad;
        orden.detalles[orden.numDetalles].precioUnitario = rep.precio;
        orden.numDetalles++;
    }

    return persistenciaSobrescribirOrden(orden);
}

CodigoResultado quitarRepuestoDeOrden(int idOrden, int idRepuesto){
    if (idOrden <= 0 || idRepuesto <= 0) {
        return RES_DATOS_INVALIDOS;
    }

    OrdenServicio orden;
    CodigoResultado resO = buscarOrdenPorID(idOrden, orden);
    if (resO != RES_OK) {
        return resO;
    }

    if (!compararCadenasExactas(orden.estado, "ABIERTA")) {
        return RES_ORDEN_NO_ABIERTA;
    }

    int idx = -1;
    for (int i = 0; i < orden.numDetalles; i++) {
        if (orden.detalles[i].idRepuesto == idRepuesto) {
            idx = i;
            break;
        }
    }

    if (idx == -1) {
        return RES_NO_EXISTE;
    }

    Repuesto rep;
    CodigoResultado resR = persistenciaLeerRepuestoPorID(idRepuesto, rep);
    if (resR == RES_OK) {
        rep.stock += orden.detalles[idx].cantidad;
        CodigoResultado resRep = persistenciaSobrescribirRepuesto(rep);
        if (resRep != RES_OK) {
            return resRep;
        }
    } else if (resR == RES_ERROR_ARCHIVO) {
        return resR;
    }

    for (int i = idx; i < orden.numDetalles - 1; i++) {
        orden.detalles[i] = orden.detalles[i + 1];
    }
    orden.numDetalles--;

    return persistenciaSobrescribirOrden(orden);
}

CodigoResultado cerrarOrden(int idOrden, float manoObra, const char* fechaCierre){
    if (idOrden <= 0 || manoObra < 0.0f || fechaCierre == nullptr || !validarFecha(fechaCierre)) {
        return RES_DATOS_INVALIDOS;
    }

    OrdenServicio orden;
    CodigoResultado resO = buscarOrdenPorID(idOrden, orden);
    if (resO != RES_OK) {
        return resO;
    }

    if (!compararCadenasExactas(orden.estado, "ABIERTA")) {
        return RES_ORDEN_NO_ABIERTA;
    }

    if (orden.numDetalles == 0 && manoObra <= 0.0f) {
        return RES_DETALLE_VACIO_SIN_MANO_OBRA;
    }

    orden.manoObra = manoObra;

    int i = 0;
    while (fechaCierre[i] != '\0' && i < 10) {
        orden.fechaCierre[i] = fechaCierre[i];
        i++;
    }
    orden.fechaCierre[i] = '\0';

    const char* estCerrada = "CERRADA";
    i = 0;
    while (estCerrada[i] != '\0' && i < 11) {
        orden.estado[i] = estCerrada[i];
        i++;
    }
    orden.estado[i] = '\0';

    return persistenciaSobrescribirOrden(orden);
}

CodigoResultado anularOrden (int idOrden){
    if (idOrden <= 0) {
        return RES_DATOS_INVALIDOS;
    }

    OrdenServicio orden;
    CodigoResultado resO = buscarOrdenPorID(idOrden, orden);
    if (resO != RES_OK) {
        return resO;
    }

    if (compararCadenasExactas(orden.estado, "ANULADA")) {
        return RES_ORDEN_YA_ANULADA;
    }

    for (int i = 0; i < orden.numDetalles; i++) {
        Repuesto rep;
        CodigoResultado resR = persistenciaLeerRepuestoPorID(orden.detalles[i].idRepuesto, rep);
        if (resR == RES_OK) {
            rep.stock += orden.detalles[i].cantidad;
            CodigoResultado resRep = persistenciaSobrescribirRepuesto(rep);
            if (resRep != RES_OK) {
                return resRep;
            }
        } else if (resR == RES_ERROR_ARCHIVO) {
            return resR;
        }
    }

    const char* estAnulada = "ANULADA";
    int i = 0;
    while (estAnulada[i] != '\0' && i < 11) {
        orden.estado[i] = estAnulada[i];
        i++;
    }
    orden.estado[i] = '\0';

    return persistenciaSobrescribirOrden(orden);
}

float calcularTotalOrden(const OrdenServicio &orden) {
    float sumaRepuestos = 0.0f;

    for (int i = 0; i < orden.numDetalles && i < MAX_DETALLES_ORDEN; i++) {
        if (orden.detalles[i].cantidad > 0 && orden.detalles[i].precioUnitario >= 0.0f) {
            sumaRepuestos += static_cast<float>(orden.detalles[i].cantidad) * orden.detalles[i].precioUnitario;
        }
    }

    float manoObraValida = (orden.manoObra >= 0.0f) ? orden.manoObra : 0.0f;
    return sumaRepuestos + manoObraValida;
}

CodigoResultado listarOrdenesPorEstado(const char* estadoBuscado, OrdenServicio resultados[], int &cantidad, bool &hayMas){
    cantidad = 0;
    hayMas   = false;

    if (estadoBuscado == nullptr || estadoBuscado[0] == '\0'){
        return RES_DATOS_INVALIDOS;
    }

    int totalOrdenes = persistenciaObtenerTotalRegistros(RUTA_ORDENES);
    if (totalOrdenes == 0) {
        return RES_NO_EXISTE;
    }

    for (int id = 1; id <= totalOrdenes; id++) {
        OrdenServicio o;
        if (persistenciaLeerOrdenPorID(id, o) == RES_OK) {
            if (!o.eliminado && compararCadenasExactas(o.estado, estadoBuscado)) {
                if (cantidad < MAX_RESULTADOS) {
                    resultados[cantidad] = o;
                    cantidad++;
                } else {
                    hayMas = true;
                    break;
                }
            }
        }
    }

    return (cantidad > 0) ? RES_OK : RES_NO_EXISTE;
}

CodigoResultado listarOrdenesPorVehiculo(int idVehiculo, OrdenServicio resultados[], int &cantidad, bool &hayMas){
    cantidad = 0;
    hayMas   = false;

    if (idVehiculo <= 0) {
        return RES_DATOS_INVALIDOS;
    }

    int totalOrdenes = persistenciaObtenerTotalRegistros(RUTA_ORDENES);
    if (totalOrdenes == 0) {
        return RES_NO_EXISTE;
    }

    for (int id = 1; id <= totalOrdenes; id++) {
        OrdenServicio o;
        if (persistenciaLeerOrdenPorID(id, o) == RES_OK) {
            if (!o.eliminado && o.idVehiculo == idVehiculo) {
                if (cantidad < MAX_RESULTADOS) {
                    resultados[cantidad] = o;
                    cantidad++;
                } else {
                    hayMas = true;
                    break;
                }
            }
        }
    }

    return (cantidad > 0) ? RES_OK : RES_NO_EXISTE;
}



//=========================
//      CAPA VISUAL
//=========================

void menuPrincipal (){
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
void menuVehículos (){
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

void mostrarMensajeResultadoVehiculo(CodigoResultado res, const char* msgExito){
    switch (res) {
        case RES_OK:
            cout << "\n[OK] " << msgExito << endl;
            break;
        case RES_DATOS_INVALIDOS:
            cout << "\n[ERROR] Datos invalidos. Verifique placa (6-7 alfanumericos), cedula, anio (1950-2027), kilometraje (>= 0) y campos obligatorios." << endl;
            break;
        case RES_DUPLICADO:
            cout << "\n[ERROR] Ya existe un vehiculo activo con esa placa." << endl;
            break;
        case RES_NO_EXISTE:
            cout << "\n[AVISO] No se encontro el vehiculo solicitado." << endl;
            break;
        case RES_ELIMINADO:
            cout << "\n[AVISO] El vehiculo consultado fue dado de baja." << endl;
            break;
        case RES_TIENE_RELACIONES:
            cout << "\n[ERROR] No se puede eliminar el vehiculo porque tiene ordenes de servicio asociadas." << endl;
            break;
        case RES_ERROR_ARCHIVO:
            cout << "\n[ERROR] Error de lectura/escritura en el archivo de vehiculos." << endl;
            break;
        default:
            cout << "\n[ERROR] Operacion no completada (codigo: " << res << ")." << endl;
            break;
    }
}

void mostrarVehiculo(const Vehiculo &v){
    cout << "----------------------------------------" << endl;
    cout << " ID          : " << v.id << endl;
    cout << " Placa       : " << v.placa << endl;
    cout << " Marca/Modelo: " << v.marca << " " << v.modelo << " (" << v.anio << ")" << endl;
    cout << " Kilometraje : " << v.kilometraje << " km" << endl;
    cout << " Propietario : " << v.nombrePropietario << " (CI: " << v.cedulaPropietario << ")" << endl;
    cout << " Telefono    : " << v.telefonoPropietario << endl;    cout << "----------------------------------------" << endl;
}

void mostrarListaVehiculos(const Vehiculo resultados[], int cantidad, bool hayMas){
    if (cantidad <= 0) {
        cout << "\n[!] No hay vehiculos para mostrar." << endl;
        return;
    }

    cout << "\n=== LISTADO DE VEHICULOS (" << cantidad << ") ===" << endl;
    for (int i = 0; i < cantidad; i++) {
        mostrarVehiculo(resultados[i]);
    }
    if (hayMas) {
        cout << "[AVISO] Se muestran los primeros " << MAX_RESULTADOS << " registros; existen mas en el archivo." << endl;
    }
}

void mostrarVehiculoIndividual(){
    limpiar();
    cout << "=== MOSTRAR VEHICULO ===" << endl;
    int id = 0;
    cout << "Ingrese el ID del vehiculo: ";
    if (!(cin >> id)) {
        cin.clear();
        cin.ignore(10000, '\n');
        return;
    }
    cin.ignore(10000, '\n');

    Vehiculo v{};
    CodigoResultado res = buscarVehiculoPorID(id, v, nullptr);
    if (res == RES_OK) {
        mostrarVehiculo(v);
    } else {
        mostrarMensajeResultadoVehiculo(res, "");
    }
}

void mostrarListaVehiculosCompleta(){
    limpiar();
    cout << "=== LISTA DE VEHICULOS ===" << endl;

    Vehiculo resultados[MAX_RESULTADOS];
    int cantidad = 0;
    bool hayMas = false;

    CodigoResultado res = listarVehiculos(resultados, cantidad, hayMas);

    if (res == RES_OK && cantidad > 0) {
        for (int i = 0; i < cantidad; i++) {
            const Vehiculo &v = resultados[i];
            cout << "----------------------------------------" << endl;
            cout << "ID          : " << v.id << endl;
            cout << "Placa       : " << v.placa << endl;
            cout << "Vehiculo    : " << v.marca << " " << v.modelo << " (" << v.anio << ")" << endl;
            cout << "Kilometraje : " << v.kilometraje << " km" << endl;
            cout << "Propietario : " << v.nombrePropietario << " (C.I: " << v.cedulaPropietario << ")" << endl;
            cout << "Telefono    : " << v.telefonoPropietario << endl;
        }
        cout << "----------------------------------------" << endl;
        cout << "Total de vehiculos mostrados: " << cantidad << endl;
        if (hayMas) {
            cout << "[AVISO] Existen mas vehiculos registrados (limite de " << MAX_RESULTADOS << " por consulta)." << endl;
        }
    } else {
        mostrarMensajeResultadoVehiculo(res, "");
    }

    cin.ignore(10000, '\n');
}

void uiRegistrarVehiculo(){
    limpiar();
    cout << "=== REGISTRAR NUEVO VEHICULO ===" << endl;
    cout << "(Escriba CANCELAR en cualquier campo para abortar)" << endl;

    Vehiculo nuevo{};
    cin.ignore(10000, '\n');

    cout << "Placa: ";
    cin.getline(nuevo.placa, 10);
    if (esCancelar(nuevo.placa)) {
        cout << "\nRegistro cancelado por el usuario." << endl;
        return;
    }

    cout << "Marca: ";
    cin.getline(nuevo.marca, 50);
    if (esCancelar(nuevo.marca)) {
        cout << "\nRegistro cancelado por el usuario." << endl;
        return;
    }

    cout << "Modelo: ";
    cin.getline(nuevo.modelo, 50);
    if (esCancelar(nuevo.modelo)) {
        cout << "\nRegistro cancelado por el usuario." << endl;
        return;
    }

    cout << "Anio (1950-2027): ";
    if (!(cin >> nuevo.anio)) {
        cin.clear();
        cin.ignore(10000, '\n');
        nuevo.anio = 0;
    } else {
        cin.ignore(10000, '\n');
    }

    cout << "Kilometraje (>= 0): ";
    if (!(cin >> nuevo.kilometraje)) {
        cin.clear();
        cin.ignore(10000, '\n');
        nuevo.kilometraje = -1;
    } else {
        cin.ignore(10000, '\n');
    }

    cout << "Nombre del Propietario: ";
    cin.getline(nuevo.nombrePropietario, 100);
    if (esCancelar(nuevo.nombrePropietario)) {
        cout << "\nRegistro cancelado por el usuario." << endl;
        return;
    }

    cout << "Cedula del Propietario: ";
    cin.getline(nuevo.cedulaPropietario, 20);
    if (esCancelar(nuevo.cedulaPropietario)) {
        cout << "\nRegistro cancelado por el usuario." << endl;
        return;
    }

    cout << "Telefono del Propietario: ";
    cin.getline(nuevo.telefonoPropietario, 20);
    if (esCancelar(nuevo.telefonoPropietario)) {
        cout << "\nRegistro cancelado por el usuario." << endl;
        return;
    }

    if (!pedirConfirmacion("\nConfirmar registro del vehiculo?")) {
        cin.ignore(10000, '\n');
        cout << "\nRegistro cancelado." << endl;
        return;
    }
    cin.ignore(10000, '\n');

    int idAsignado = 0;
    CodigoResultado res = agregarVehiculo(nuevo, idAsignado);
    mostrarMensajeResultadoVehiculo(res, "Vehiculo registrado exitosamente.");
    if (res == RES_OK) {
        cout << "ID asignado: #" << idAsignado << endl;
    }
}
    
void uiBuscarVehiculo(){
    limpiar();
    cout << "=== BUSCAR VEHICULO ===" << endl;
    cout << "1. Buscar por ID" << endl;
    cout << "2. Buscar por Placa" << endl;
    cout << "3. Buscar por Propietario" << endl;
    cout << "Seleccione una opcion: ";

    int op = 0;
    if (!(cin >> op)) {
        cin.clear();
        cin.ignore(10000, '\n');
        return;
    }

    if (op == 1) {
        int id = 0;
        cout << "Ingrese el ID del vehiculo: ";
        if (!(cin >> id)) {
            cin.clear();
            cin.ignore(10000, '\n');
            return;
        }
        cin.ignore(10000, '\n');

        Vehiculo v{};
        CodigoResultado res = buscarVehiculoPorID(id, v, nullptr);
        if (res == RES_OK) {
            mostrarVehiculo(v);
        } else {
            mostrarMensajeResultadoVehiculo(res, "");
        }
    } else if (op == 2) {
        char placa[10];
        cout << "Ingrese la placa: ";
        cin >> placa;
        cin.ignore(10000, '\n');

        Vehiculo v{};
        CodigoResultado res = buscarVehiculoPorPlaca(placa, v);
        if (res == RES_OK) {
            mostrarVehiculo(v);
        } else {
            mostrarMensajeResultadoVehiculo(res, "");
        }
    } else if (op == 3) {
        char subcadena[100];
        cin.ignore(10000, '\n');
        cout << "Ingrese parte del nombre del propietario: ";
        cin.getline(subcadena, 100);

        Vehiculo resultados[MAX_RESULTADOS];
        int cantidad = 0;
        bool hayMas = false;
        CodigoResultado res = buscarVehiculosPorPropietario(subcadena, resultados, cantidad, hayMas);
        if (res == RES_OK) {
            mostrarListaVehiculos(resultados, cantidad, hayMas);
        } else {
            mostrarMensajeResultadoVehiculo(res, "");
        }
    } else {
        cin.ignore(10000, '\n');
        cout << "\nOpcion invalida." << endl;
    }
}

void uiActualizarVehiculo(){
    limpiar();
    cout << "=== ACTUALIZAR VEHICULO ===" << endl;
    int id = 0;
    cout << "Ingrese el ID del vehiculo: ";
    if (!(cin >> id)) {
        cin.clear();
        cin.ignore(10000, '\n');
        return;
    }

    Vehiculo actual{};
    CodigoResultado resBusq = buscarVehiculoPorID(id, actual, nullptr);
    if (resBusq != RES_OK) {
        cin.ignore(10000, '\n');
        mostrarMensajeResultadoVehiculo(resBusq, "");
        return;
    }

    cout << "\nVehiculo actual:" << endl;
    mostrarVehiculo(actual);

    Vehiculo nuevosDatos{};
    cin.ignore(10000, '\n');
    cout << "Nueva Marca: ";
    cin.getline(nuevosDatos.marca, 50);

    cout << "Nuevo Modelo: ";
    cin.getline(nuevosDatos.modelo, 50);

    cout << "Nuevo Anio (1950-2027): ";
    if (!(cin >> nuevosDatos.anio)) {
        cin.clear();
        cin.ignore(10000, '\n');
        nuevosDatos.anio = 0;
    } else {
        cin.ignore(10000, '\n');
    }

    cout << "Nuevo Nombre del Propietario: ";
    cin.getline(nuevosDatos.nombrePropietario, 100);

    cout << "Nueva Cedula del Propietario: ";
    cin.getline(nuevosDatos.cedulaPropietario, 20);

    cout << "Nuevo Telefono del Propietario: ";
    cin.getline(nuevosDatos.telefonoPropietario, 20);

    if (!pedirConfirmacion("\nConfirmar actualizacion del vehiculo?")) {
        cin.ignore(10000, '\n');
        cout << "\nActualizacion cancelada." << endl;
        return;
    }
    cin.ignore(10000, '\n');

    CodigoResultado resAct = actualizarVehiculo(id, nuevosDatos);
    mostrarMensajeResultadoVehiculo(resAct, "Vehiculo actualizado exitosamente.");
}

void uiEliminarVehiculo(){
    limpiar();
    cout << "=== ELIMINAR VEHICULO ===" << endl;
    int id = 0;
    cout << "Ingrese el ID del vehiculo a eliminar: ";
    if (!(cin >> id)) {
        cin.clear();
        cin.ignore(10000, '\n');
        return;
    }

    if (!pedirConfirmacion("\nConfirma dar de baja este vehiculo?")) {
        cin.ignore(10000, '\n');
        cout << "\nOperacion cancelada." << endl;
        return;
    }
    cin.ignore(10000, '\n');

    CodigoResultado res = eliminarVehiculo(id);
    mostrarMensajeResultadoVehiculo(res, "Vehiculo dado de baja correctamente.");
}

void uiListarVehiculos(){
    limpiar();
    Vehiculo resultados[MAX_RESULTADOS];
    int cantidad = 0;
    bool hayMas = false;

    CodigoResultado res = listarVehiculos(resultados, cantidad, hayMas);
    if (res == RES_OK) {
        mostrarListaVehiculos(resultados, cantidad, hayMas);
    } else {
        mostrarMensajeResultadoVehiculo(res, "");
    }

    cin.ignore(10000, '\n');
}
/* jai papa
//Mecanicos UI
void menuMecánicos () {
    cout << "╔═══════════════════════════════════════════╗" << endl;
    cout << "║            GESTIÓN DE MECÁNICOS           ║" << endl;
    cout << "╠═══════════════════════════════════════════╣" << endl;
    cout << "║ 1. Registrar mecánico                     ║" << endl;
    cout << "║ 2. Buscar por ID                          ║" << endl;
    cout << "║ 3. Listar todos (activos y de baja)       ║" << endl;
    cout << "║ 4. Dar de baja mecánico                   ║" << endl;
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

    if (!pedirConfirmacion("\n¿Confirmar registro del mecánico?")) {
        cout << "\nRegistro cancelado." << endl;
        cin.ignore(10000, '\n'); pausar(); return;
    }
    cin.ignore(10000, '\n');

    int idAsignado = 0;
    CodigoResultado res = registrarMecanico(nuevo, idAsignado);
    if (res == RES_OK) {
        cout << "\n✓ Mecánico '" << nuevo.nombre << "' registrado exitosamente con ID #" << idAsignado << "." << endl;
    } else if (res == RES_DUPLICADO) {
        cout << "\nERROR: Ya existe un mecánico activo con esa cédula." << endl;
    } else {
        cout << "\nERROR: Datos inválidos (verifique cédula, fecha YYYY-MM-DD y especialidad en MAYÚSCULAS)." << endl;
    }
    limpiar();
}

void uiBuscarMecanicoID(SistemaTaller* s) {
    limpiar();
    cout << "=== BUSCAR MECANICO POR ID ===" << endl;
    int id;
    cout << "Ingrese el ID del mecánico: ";
    if (!(cin >> id)){
        cin.clear();
        cin.ignore(10000, '\n');
        return;
    }

    // Limpiamos el búfer despues de cin para evitar errores
    cin.ignore(10000, '\n');
    
    Mecanico m;
    CodigoResultado res = ObtenerMecanicoPorID(id, m);
    if (res == RES_OK || res == RES_ELIMINADO) {
        cout << "\n--- MECÁNICO ENCONTRADO ---" << endl;
        cout << "ID          : #" << m.id << endl;
        cout << "Nombre      : " << m.nombre << (m.eliminado ? " [DADO DE BAJA]" : " [ACTIVO]") << endl;
        cout << "Cédula      : " << m.cedula << endl;
        cout << "Especialidad: " << m.especialidad << endl;
        cout << "Ingreso     : " << m.fechaIngreso << endl;
        cout << "Órdenes Ab. : " << ContarOrdenesAbiertasMecanico(m.id) << endl;
    } else {
        cout << "\nERROR: No existe ningún mecánico con el ID #" << id << "." << endl;
    }
    pausar();
}

void uiDarDeBajaMecanico() {
    limpiar();
    cout << "=== DAR DE BAJA MECÁNICO ===" << endl;
    int id;
    cout << "Ingrese el ID del mecánico a dar de baja: ";
    if (!(cin >> id)) { cin.clear(); cin.ignore(10000, '\n'); return; }

    if (!pedirConfirmacion("\n¿Confirmar la baja del mecánico?")) {
        cout << "\nOperación cancelada." << endl;
        cin.ignore(10000, '\n'); pausar(); return;
    }
    cin.ignore(10000, '\n');

    CodigoResultado res = DarDeBajaMecanico(id);
    if (res == RES_OK) {
        cout << "\n✓ Mecánico dado de baja exitosamente. Su historial en órdenes cerradas se conserva." << endl;
    } else if (res == RES_TIENE_RELACIONES) {
        cout << "\nERROR: El mecánico tiene órdenes ABIERTAS y no puede darse de baja hasta cerrarlas." << endl;
    } else if (res == RES_ELIMINADO) {
        cout << "\nAVISO: Ese mecánico ya se encontraba dado de baja." << endl;
    } else {
        cout << "\nERROR: No existe ningún mecánico con el ID #" << id << "." << endl;
    }
    pausar();
}

void uiListarMecanicos(){
    limpiar();
    Mecanico lista[MAX_RESULTADOS];
    int cantidad = 0;
    bool hayMas  = false;

    if (listarMecanicos(lista, cantidad, hayMas) == RES_OK){
        cout << "\n=== LISTADO DE MECANICOS (" << cantidad << ") ===" << endl;
        for (int i = 0; i < cantidad; i++){
            Mecanico* m = lista[i];
            cout << "----------------------------------------" << endl;
            cout << "ID          : #" << lista[i].id << endl;
            cout << "Nombre      : " << lista[i].nombre << (lista[i].eliminado ? " (BAJA)" : "") << endl;
            cout << "Cédula      : " << lista[i].cedula << endl;
            cout << "Especialidad: " << lista[i].especialidad << endl;
            cout << "Órdenes Ab. : " << ContarOrdenesAbiertasMecanico(lista[i].id) << endl;
        }
        if (hayMas) cout << "\n[!] Hay más mecánicos en disco (mostrando los primeros " << MAX_RESULTADOS << ")." << endl;
    } else {
        cout << "\nNo hay mecánicos registrados en el sistema." << endl;
    }
    cin.ignore(10000, '\n');
    pausar();
}

void uiDarDeBajaMecanico() {
    limpiar();
    cout << "=== DAR DE BAJA MECÁNICO ===" << endl;
    int id;
    cout << "Ingrese el ID del mecánico a dar de baja: ";
    if (!(cin >> id)) { cin.clear(); cin.ignore(10000, '\n'); return; }

    if (!pedirConfirmacion("\n¿Confirmar la baja del mecánico?")) {
        cout << "\nOperación cancelada." << endl;
        cin.ignore(10000, '\n'); pausar(); return;
    }
    cin.ignore(10000, '\n');

    CodigoResultado res = DarDeBajaMecanico(id);
    if (res == RES_OK) {
        cout << "\n✓ Mecánico dado de baja exitosamente. Su historial en órdenes cerradas se conserva." << endl;
    } else if (res == RES_TIENE_RELACIONES) {
        cout << "\nERROR: El mecánico tiene órdenes ABIERTAS y no puede darse de baja hasta cerrarlas." << endl;
    } else if (res == RES_ELIMINADO) {
        cout << "\nAVISO: Ese mecánico ya se encontraba dado de baja." << endl;
    } else {
        cout << "\nERROR: No existe ningún mecánico con el ID #" << id << "." << endl;
    }
    pausar();
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
void mostrarRepuesto(const Repuesto &r) {
    cout << "----------------------------------------" << endl;
    cout << " ID           : " << r->id << endl;
    cout << " Código       : " << r->codigo << endl;
    cout << " Nombre       : " << r->nombre << endl;
    cout << " Marca        : " << r->marca << endl;
    cout << " Precio Unit. : $" << r->precio << endl;
    cout << " Stock Actual : " << r->stock << " (Mínimo: " << r->stockMinimo << ")";

    // Alerta de bajo stock
    if (r.stock <= r.stockMinimo) {
        cout << " <-- [ALERTA: BAJO STOCK]";
    }
    cout << endl;
}

void uiRegistrarRepuesto() {
    limpiar();
    cout << "=== REGISTRAR NUEVO REPUESTO ===" << endl;
    Repuesto nuevo;

    cout << "Código (único, sin espacios): ";
    cin >> nuevo.codigo;

    // Limpiamos el salto de linea para leer nombres con espacios
    cin.ignore(10000, '\n');

    cout << "Nombre del repuesto: "; cin.getline(nuevo.nombre, 100);
    cout << "Marca: "; cin.getline(nuevo.marca, 50);

    cout << "Precio unitario (>0): "; 
    if (!(cin >> nuevo.precio)) { cin.clear(); cin.ignore(10000, '\n'); nuevo.precio = 0.0f; }
    
    cout << "Stock inicial (>= 0): "; 
    if (!(cin >> nuevo.stock)) { cin.clear(); cin.ignore(10000, '\n'); nuevo.stock = -1; }
    
    cout << "Stock mínimo para alerta (>= 0): "; 
    if (!(cin >> nuevo.stockMinimo)) { cin.clear(); cin.ignore(10000, '\n'); nuevo.stockMinimo = -1; }
    
    if (!pedirConfirmacion("\n¿Confirmar registro del repuesto?")) {
        cout << "\nRegistro cancelado." << endl;
        cin.ignore(10000, '\n'); pausar(); return;
    }
    cin.ignore(10000, '\n');

    int idAsignado = 0;
    CodigoResultado res = registrarRepuesto(nuevo, idAsignado);
    if (res == RES_OK) {
        cout << "\n✓ Repuesto '" << nuevo.nombre << "' registrado con ID #" << idAsignado << "." << endl;
    } else if (res == RES_DUPLICADO) {
        cout << "\nERROR: El código '" << nuevo.codigo << "' ya existe en el inventario activo." << endl;
    } else {
        cout << "\nERROR: Datos inválidos (verifique precio > 0 y stock >= 0)." << endl;
    }
    pausar();
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
    if (!(cin >> op)){
        cin.clear();
        cin.ignore(10000, '\n');
        return;
    }

    switch (op) {
        case 1: {
            int id;
            cout << "Ingrese el ID del repuesto: ";
            if (!(cin >> idBuscado)) { cin.clear(); cin.ignore(10000, '\n'); break; }
            cin.ignore(10000, '\n');
            Repuesto r;
            CodigoResultado res = buscarRepuestoPorID(id, r);
            if (res == RES_OK) mostrarRepuesto(r);
            else if (res == RES_ELIMINADO) cout << "\nAVISO: El repuesto ID #" << id << " fue eliminado." << endl;
            else cout << "\nERROR: No existe repuesto con ID #" << id << "." << endl;
            break;
        }
        case 2: {
            char codigoBuscado[20];
            cout << "Ingrese el código exacto: ";
            cin >> codigoBuscado;
            cin.ignore(10000, '\n');

            Repuesto r;
            if (buscarRepuestoPorCodigo(codigoBuscado, r) == RES_OK) mostrarRepuesto(r);
            else cout << "\nERROR: No se encontró repuesto activo con código '" << cod << "'." << endl;
            break;
        }
        case 3: {
            char sub[100];
            cin.ignore(10000, '\n');
            cout << "Ingrese parte del nombre del repuesto: ";
            cin.getline(sub, 100);

            Repuesto resLista[MAX_RESULTADOS];
            int cant = 0; bool hayMas = false;
            if (buscarRepuestosPorNombreParcial(sub, resLista, cant, hayMas) == RES_OK) {
                for (int i = 0; i < cant; i++) mostrarRepuesto(resLista[i]);
                if (hayMas) cout << "\n[!] Hay más coincidencias en disco." << endl;
            } else {
                cout << "\nNo se encontraron repuestos con ese nombre." << endl;
            }
            break;
        }
        default: {
            cin.ignore(10000, '\n');
            cout << "\nOpción de búsqueda inválida." << endl;
            break;
        }
    }
    pausar();
}

void uiEntradaMercancia(SistemaTaller* s) {
    limpiar();
    cout << "=== REGISTRAR ENTRADA DE MERCANCÍA ===" << endl;
    int id, cantidad;

    cout << "Ingrese el ID del repuesto: ";
    if (!(cin >> id)){
        cin.clear();
        cin.ignore(10000, '\n');
        return;
    }
    cout << "Ingrese la cantidad de unidades que ingresan (> 0): ";
    if (!(cin >> cantidad)) { cin.clear(); cin.ignore(10000, '\n'); return; }
    cin.ignore(10000, '\n');

    int nuevoStock = 0;
    CodigoResultado res = registrarEntradaMercancia(id, cant, nuevoStock);
    if (res == RES_OK) {
        cout << "\n✓ Entrada registrada en disco. Nuevo stock: " << nuevoStock << " unidades." << endl;
    } else if (res == RES_ELIMINADO) {
        cout << "\nERROR: El repuesto fue eliminado." << endl;
    } else if (res == RES_NO_EXISTE) {
        cout << "\nERROR: No existe un repuesto con ID #" << id << "." << endl;
    } else {
        cout << "\nERROR: La cantidad debe ser mayor a 0." << endl;
    }
    pausar();
}

void uiListarRepuestos() {
    limpiar();
    Repuesto lista[MAX_RESULTADOS];
    int cant = 0; bool hayMas = false;

    if (listarRepuestosActivos(lista, cant, hayMas) == RES_OK) {
        cout << "=== INVENTARIO DE REPUESTOS (" << cant << ") ===" << endl;
        for (int i = 0; i < cant; i++) mostrarRepuesto(lista[i]);
        if (hayMas) cout << "\n[!] Hay más repuestos en disco (mostrando los primeros " << MAX_RESULTADOS << ")." << endl;
    } else {
        cout << "\nNo hay repuestos activos en el inventario." << endl;
    }
    cin.ignore(10000, '\n');
    pausar();
}

void uiCambiarPrecioRepuesto() {
    limpiar();
    cout << "=== CAMBIAR PRECIO DE REPUESTO ===" << endl;
    int id; float precio;
    cout << "ID del repuesto: ";
    if (!(cin >> id)) { cin.clear(); cin.ignore(10000, '\n'); return; }
    cout << "Nuevo precio (> 0): ";
    if (!(cin >> precio)) { cin.clear(); cin.ignore(10000, '\n'); return; }
    cin.ignore(10000, '\n');

    CodigoResultado res = cambiarPrecioRepuesto(id, precio);
    if (res == RES_OK) {
        cout << "\n✓ Precio actualizado en disco a $" << precio << " (no afecta órdenes históricas)." << endl;
    } else {
        cout << "\nERROR: Verifique que el repuesto exista, esté activo y el precio sea > 0." << endl;
    }
    pausar();
}

void uiEliminarRepuesto() {
    limpiar();
    cout << "=== ELIMINAR REPUESTO (BORRADO LÓGICO) ===" << endl;
    int id;
    cout << "ID del repuesto a eliminar: ";
    if (!(cin >> id)) { cin.clear(); cin.ignore(10000, '\n'); return; }

    if (!pedirConfirmacion("\n¿Confirmar eliminación del repuesto?")) {
        cout << "\nOperación cancelada." << endl;
        cin.ignore(10000, '\n'); pausar(); return;
    }
    cin.ignore(10000, '\n');

    CodigoResultado res = eliminarRepuesto(id);
    if (res == RES_OK) {
        cout << "\n✓ Repuesto marcado como eliminado lógicamente." << endl;
    } else if (res == RES_TIENE_RELACIONES) {
        cout << "\nERROR: El repuesto figura en el detalle de una o más órdenes y no puede eliminarse." << endl;
    } else {
        cout << "\nERROR: El repuesto no existe o ya fue eliminado." << endl;
    }
    pausar();
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

    cout << "ID del vehiculo: "; 
    if (!(cin >> idV)) { cin.clear(); cin.ignore(10000, '\n'); return; }
    
    cout << "ID del mecanico: "; 
    if (!(cin >> idM)) { cin.clear(); cin.ignore(10000, '\n'); return; }
    
    cout << "Kilometraje de ingreso: "; 
    if (!(cin >> km)) { cin.clear(); cin.ignore(10000, '\n'); return; }
    
    cout << "Fecha (YYY-MM-DD): "; cin >> fecha;
    cin.ignore(10000, '\n');
    cout << "Descripcion de la falla: "; cin.getline(falla, 200);
    
    Vehiculo* v = buscarVehiculoPorID(s, idV);
    if (v == nullptr) {
        cout << "\nERROR: No existe ningún vehículo con el ID #" << idV << "." << endl;
        limpiar();
        return;
    }

    Mecanico* m = buscarMecanicoPorID(s, idM);
    if (m == nullptr){
        cout << "\nError: Noexiste ningún mecánico con el ID #" << idM << "." << endl;
        limpiar();
        return;
    }
    
    OrdenServicio* abierta = buscarOrdenAbiertaVehiculo(s, idV);
    if (abierta != nullptr){
        cout << "\nError: El vehiculo '" << v->placa << "' ya tiene la orden #" << abierta->id << " ABIERTA." << endl;
        limpiar();
        return;
    }
    if (contarOrdenesAbiertasMecanico(s, idM) >= 3) {
        cout << "\nERROR: El mecánico '" << m->nombre << "' ya tiene 3 órdenes abiertas." << endl;
        limpiar();
        return;
    }
    
    if (km < v->kilometraje){
        cout << "\nError: El kilometraje de ingreso (" << km << ") es menor al registrado (" << v->kilometraje << " km)." << endl;
        limpiar();
        return;
    }
    
    if (!pedirConfirmacion("\n¿Confirmar la apertura de la orden?")) {
        cout << "\nApertura cancelada." << endl;
        cin.ignore(10000, '\n');
        limpiar();
        return;
    }
    cin.ignore(10000, '\n');

    OrdenServicio* o = abrirOrden(s, idV, idM, fecha, falla, km);
    if (o){
        cout << "\nOrden #" << o->id << " abierta exitosamente." << endl;
    } else{
        cout << "\nError: Verifique IDs, formato de fecha, limite de 3 ordenes por mecanico, kilometraje y falla no vacía." << endl;
    }
    limpiar();
}

void uiAgregarRepuestoOrden(SistemaTaller* s){
    limpiar();
    cout << "=== AGREGRAR REPUESTO A ORDEN ===" << endl;
    int idO, idR, cant;
    
    cout << "ID de la orden: "; 
    if (!(cin >> idO)) { cin.clear(); cin.ignore(10000, '\n'); return; }
    
    cout << "ID del repuesto: "; 
    if (!(cin >> idR)) { cin.clear(); cin.ignore(10000, '\n'); return; }
    
    cout << "Cantidad: "; 
    if (!(cin >> cant)) { cin.clear(); cin.ignore(10000, '\n'); return; }
    
    cin.ignore(10000, '\n');

    OrdenServicio* o = buscarOrdenPorID(s, idO);
    if (o == nullptr){
        cout << "\nERROR: No existe la orden #" << idO << "." << endl;
        limpiar();
        return;
    }
    if (!compararCadenasExactas(o->estado, "ABIERTA")){
        cout << "\nERROR: La orden #" << idO << " esta " << o->estado << " y no puede modificarse." << endl;
        limpiar();
        return;
    }
    
    Repuesto* r = buscarRepuestoPorID(s, idR);
    if (r == nullptr) {
        cout << "\nERROR: No existe el repuesto con ID #" << idR << "." << endl;
        limpiar();
        return;
    }
    
    if (r->stock < cant) {
        cout << "\nERROR: Stock insuficiente de '" << r->nombre << "'. Disponible: " << r->stock << ", solicitado: " << cant << "." << endl;
        limpiar();
        return;
    }

    if (agregarRepuestoAOrden(s, idO, idR,cant)){
        cout << "\nRepuesto '" << r->nombre << "agregado con exito a la orden #"  << idO << endl;
    } else{
        cout << "\nERROR: Stock insuficiente o la orden no esta aierta." << endl;
        limpiar();
    }
}

void uiQuitarRepuestoOrden(SistemaTaller* s){
    limpiar();
    cout << "=== QUITAR REPUESTO DE ORDEN ===" << endl;
    int idO, idR;
    cout << "ID de la orden: ";
    if (!(cin >> idO)) { cin.clear(); cin.ignore(10000, '\n'); return; }
    
    cout << "ID del repuesto: "; 
    if (!(cin >> idR)) { cin.clear(); cin.ignore(10000, '\n'); return; }
    
    cin.ignore(10000, '\n');

    OrdenServicio* o = buscarOrdenPorID(s, idO);
    if ( o != nullptr && !compararCadenasExactas(o->estado, "ABIERTA")){
        cout << "\nERROR: La orden #" << idO << " está " << o->estado << " y no puede modificarse." << endl;
        limpiar();
        return;
    }
    if (quitarRepuestoDeOrden(s, idO, idR)){
        cout << "Repuesto removido y stock devuelto." << endl;
    } else{
        cout << "Error al quitar el repuesto." << endl;
    }
    limpiar();
}

void uiCerrarOrden(SistemaTaller* s){
    limpiar();
    cout << "=== CERRAR ORDEN ===" << endl;
    int idO; float manoObra; char fecha[11];
    
    cout << "ID de la orden: "; 
    if (!(cin >> idO)) { cin.clear(); cin.ignore(10000, '\n'); return; }
    
    cout << "Costo de mano de mano de obra: "; 
    if (!(cin >> manoObra)) { cin.clear(); cin.ignore(10000, '\n'); return; }
    
    cout << "Fecha cierre (YYYY-MM-DD): "; cin >> fecha;

    if (!pedirConfirmacion("\n¿Confirmar el cierre de la orden?")) {
        cout << "\nCierre cancelado." << endl;
        cin.ignore(10000, '\n');
        limpiar();
        return;
    }
    cin.ignore(10000, '\n');
    
    if (cerrarOrden(s, idO, manoObra, fecha)) {
        cout << "Orden #" << idO << "cerrada con éxito." << endl;
    } else{
        cout << "\nERROR: Verifique que la orden esté ABIERTA, tenga detalles o mano de obra > 0, y fecha válida." << endl;
    }
    limpiar();
}

void uiAnularOrden(SistemaTaller* s){
    limpiar();
    cout << "=== ANULAR ORDEN ===" << endl;
    int idO;
    cout << "ID de la orden a anular: ";
    if (!(cin >> idO)){
        cin.clear();
        cin.ignore(10000, '\n');
        return;
    }

    if (!pedirConfirmacion("\nConfirmar la anulacion de la orden? (Se devolvera el stock al inventario)")){
        cout << "\nAnulación cancelada." << endl;
        cin.ignore(10000, '\n');
        limpiar();
        return;
    }
    cin.ignore(10000, '\n');

    if (anularOrden(s, idO)){
        cout <<"\nOrden #" << idO << " ANULADA y stock devuelto al inventario" << endl;
    } else{
        cout << "\nERROR: Orden inexistente o ya anulada." << endl;
    }
    limpiar();
}

void uiVerDetalleOrden(SistemaTaller* s){
    limpiar();
    cout << "=== VER DETALLE ORDEN ===" << endl;
    int idO;
    cout << "Ingrese el ID de la orden: ";
    if (!(cin >> idO)){
        cin.clear();
        cin.ignore(10000, '\n');
        return;
    }
    cin.ignore(10000, '\n');
    
    OrdenServicio* o = buscarOrdenPorID(s, idO);
    if (o == nullptr){
        cout << "Error: No se encontro ninguna orden con el ID #" << idO << "." << endl;
        limpiar();
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
    limpiar();
}

void uiListarOrdenes(SistemaTaller *s){
    limpiar();
    cout << "=== LISTAR ORDENES ===" << endl;
    cout << "1. Listar por Estado (ABIERTA, CERRADA, ANULADA)" << endl;
    cout << "2. Listar por Vehiculo (Historial)" << endl;
    cout << "Seleccione una opcion: ";
    int op;
    if (!(cin >> op)){
        cin.clear();
        cin.ignore(10000, '\n');
        return;
    }

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
            if (!(cin >> idV)){ 
                cin.clear();
                cin.ignore(10000, '\n');
                break;
            }
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
    limpiar();
}

// ==================================================
//     PRE-POBLADO AUTOMÁTICO DE DATOS INICIALES 
// ==================================================

void prepoblarDatosIniciales() {
    // Si ya hay vehículos guardados en disco, no volvemos a pre-poblar
    if (persistenciaObtenerTotalRegistros(RUTA_VEHICULOS) > 0) {
        return;
    }

    // Datos del Taller (si no existe taller.bin)
    if (!existeArchivoTaller()) {
        Taller t;
        copiarCadenaSegura(t.nombre,    "Autoservicio El Wikiti", 100);
        copiarCadenaSegura(t.rif,       "J-12345678-9", 20);
        copiarCadenaSegura(t.direccion, "Av. Bella Vista, Maracaibo", 200);
        copiarCadenaSegura(t.telefono,  "0414-1234567", 20);
        guardarTaller(t);
    }

    int idTemp = 0;

    // Registrar 8 Vehículos
    const char* placas[8] = {"AB123CD", "XY987ZW", "CD456EF", "GH789IJ", "KL012MN", "OP345QR", "ST678UV", "WX901YZ"};
    const char* marcas[8] = {"Toyota", "Chevrolet", "Ford", "Mazda", "Hyundai", "Nissan", "Honda", "Kia"};
    const char* modelos[8] = {"Corolla", "Aveo", "Fiesta", "Mazda3", "Elantra", "Sentra", "Civic", "Rio"};
    int anios[8] = {2015, 2009, 2018, 2019, 2021, 2016, 2014, 2022};
    int kms[8]   = {44000, 128000, 30000, 50000, 20000, 65000, 89000, 15000};
    const char* duenos[8] = {"Maria Gonzalez", "Maria Gonzalez", "Carlos Perez", "Luis Gomez", "Ana Rivas", "Pedro Lara", "Sofia Mendoza", "Diego Torres"};
    const char* cedulas[8] = {"V-12345678", "V-12345678", "V-87654321", "V-22334455", "V-33445566", "V-19283746", "V-25678901", "V-28901234"};
    const char* telefonos[8] = {"0414-1112233", "0414-1112233", "0412-9998877", "0416-5554433", "0424-6667788", "0414-0001122", "0412-3334455", "0424-7778899"};

    for (int i = 0; i < 8; i++) {
        Vehiculo v;
        copiarCadenaSegura(v.placa, placas[i], 10);
        copiarCadenaSegura(v.marca, marcas[i], 50);
        copiarCadenaSegura(v.modelo, modelos[i], 50);
        v.anio        = anios[i];
        v.kilometraje = kms[i];
        copiarCadenaSegura(v.nombrePropietario, duenos[i], 100);
        copiarCadenaSegura(v.cedulaPropietario, cedulas[i], 20);
        copiarCadenaSegura(v.telefonoPropietario, telefonos[i], 20);
        agregarVehiculo(v, idTemp);
    }

    // Registrar 4 Mecánicos (El #4 será dado de baja después de sus órdenes cerradas)
    const char* nomMec[4] = {"Jose Perez", "Ana Villalobos", "Marco Delgado", "Luis Fuenmayor"};
    const char* cedMec[4] = {"V-11223344", "V-14556677", "V-16889900", "V-13445566"};
    const char* espMec[4] = {"FRENOS", "MOTOR", "ELECTRICIDAD", "SUSPENSION"};
    const char* fecMec[4] = {"2023-02-10", "2024-05-15", "2024-08-20", "2022-11-01"};

    for (int i = 0; i < 4; i++) {
        Mecanico m;
        copiarCadenaSegura(m.nombre, nomMec[i], 100);
        copiarCadenaSegura(m.cedula, cedMec[i], 20);
        copiarCadenaSegura(m.especialidad, espMec[i], 20);
        copiarCadenaSegura(m.fechaIngreso, fecMec[i], 11);
        registrarMecanico(m, idTemp);
    }

    // Registrar 12 Repuestos (Incluye variedad de precios y algunos con bajo stock)
    const char* codRep[12] = {"FRN-001", "FRN-014", "MOT-002", "MOT-009", "ELE-005", "SUS-010",
                              "MOT-015", "FRN-020", "ELE-012", "SUS-018", "GEN-001", "GEN-008"};
    const char* nomRep[12] = {"Pastillas de freno", "Liquido de frenos DOT4", "Filtro de aceite", "Bujia de encendido",
                              "Bateria 12V", "Amortiguador delantero", "Correa de tiempo", "Disco de freno",
                              "Bombillo LED H4", "Rotula de suspension", "Aceite sintetico 5W30", "Filtro de aire"};
    const char* marRep[12] = {"Bosch", "Wagner", "Mann", "NGK", "Duncan", "Monroe",
                              "Gates", "Brembo", "Philips", "Moog", "Castrol", "Fram"};
    float preRep[12] = {25.00f, 12.50f, 10.00f, 5.00f, 85.00f, 45.00f, 35.00f, 40.00f, 15.00f, 22.00f, 14.00f, 11.00f};
    int stkRep[12]   = {20, 15, 25, 40, 6, 12, 10, 14, 18, 10, 30, 5};
    int minRep[12]   = {5,  5,  5,  8,  4, 4,  3,  4,  5,  3,  6,  5};

    for (int i = 0; i < 12; i++) {
        Repuesto r;
        copiarCadenaSegura(r.codigo, codRep[i], 20);
        copiarCadenaSegura(r.nombre, nomRep[i], 100);
        copiarCadenaSegura(r.marca,  marRep[i], 50);
        r.precio      = preRep[i];
        r.stock       = stkRep[i];
        r.stockMinimo = minRep[i];
        registrarRepuesto(r, idTemp);
    }

    // Crear 10 Órdenes en los 3 estados (3 de ellas con 4 o más líneas)
    int idOrd = 0;

    // Orden #1 (CERRADA - 4 líneas) - Vehículo 1, Mecánico 1
    abrirOrden(1, 1, "2026-10-05", "Mantenimiento preventivo y frenos", 44500, idOrd);
    agregarRepuestoAOrden(idOrd, 1, 2);
    agregarRepuestoAOrden(idOrd, 2, 1);
    agregarRepuestoAOrden(idOrd, 3, 1);
    agregarRepuestoAOrden(idOrd, 4, 4);
    cerrarOrden(idOrd, 45.00f, "2026-10-06");

    // Orden #2 (CERRADA - 4 líneas) - Vehículo 2, Mecánico 2
    abrirOrden(2, 2, "2026-10-08", "Servicio mayor de motor y suspension", 128500, idOrd);
    agregarRepuestoAOrden(idOrd, 5, 1);
    agregarRepuestoAOrden(idOrd, 6, 2);
    agregarRepuestoAOrden(idOrd, 7, 1);
    agregarRepuestoAOrden(idOrd, 8, 2);
    cerrarOrden(idOrd, 80.00f, "2026-10-10");

    // Orden #3 (CERRADA - 5 líneas) - Vehículo 3, Mecánico 3
    abrirOrden(3, 3, "2026-10-12", "Revision electrica y afinacion", 30500, idOrd);
    agregarRepuestoAOrden(idOrd, 1, 1);
    agregarRepuestoAOrden(idOrd, 3, 1);
    agregarRepuestoAOrden(idOrd, 9, 2);
    agregarRepuestoAOrden(idOrd, 10, 2);
    agregarRepuestoAOrden(idOrd, 11, 4);
    cerrarOrden(idOrd, 60.00f, "2026-10-13");

    // Orden #4 (CERRADA - 2 líneas) - Vehículo 4, Mecánico 4 (antes de su baja)
    abrirOrden(4, 4, "2026-10-14", "Cambio de filtros y liquido de frenos", 50400, idOrd);
    agregarRepuestoAOrden(idOrd, 2, 1);
    agregarRepuestoAOrden(idOrd, 12, 1);
    cerrarOrden(idOrd, 30.00f, "2026-10-15");

    // Orden #5 (CERRADA - 2 líneas) - Vehículo 5, Mecánico 4 (antes de su baja)
    abrirOrden(5, 4, "2026-10-16", "Revision de bujias y amortiguadores", 20500, idOrd);
    agregarRepuestoAOrden(idOrd, 4, 4);
    agregarRepuestoAOrden(idOrd, 6, 2);
    cerrarOrden(idOrd, 50.00f, "2026-10-17");

    // Damos de baja al Mecánico #4 (ahora tiene 0 órdenes abiertas y 2 cerradas)
    DarDeBajaMecanico(4);

    // Orden #6 (ANULADA - 2 líneas) - Vehículo 6, Mecánico 1
    abrirOrden(6, 1, "2026-10-18", "Presupuesto de frenos y bateria", 65200, idOrd);
    agregarRepuestoAOrden(idOrd, 1, 2);
    agregarRepuestoAOrden(idOrd, 5, 1);
    anularOrden(idOrd);

    // Orden #7 (ANULADA - 1 línea) - Vehículo 7, Mecánico 2
    abrirOrden(7, 2, "2026-10-19", "Cambio de filtro cancelado por cliente", 89300, idOrd);
    agregarRepuestoAOrden(idOrd, 3, 2);
    anularOrden(idOrd);

    // Orden #8 (ABIERTA - 2 líneas) - Vehículo 1, Mecánico 1
    abrirOrden(8, 1, "2026-11-15", "Ruido en correa y luces delanteras", 45100, idOrd);
    agregarRepuestoAOrden(idOrd, 7, 1);
    agregarRepuestoAOrden(idOrd, 9, 2);

    // Orden #9 (ABIERTA - 2 líneas) - Vehículo 2, Mecánico 2
    abrirOrden(2, 2, "2026-11-16", "Vibracion al frenar en carretera", 129100, idOrd);
    agregarRepuestoAOrden(idOrd, 8, 2);
    agregarRepuestoAOrden(idOrd, 10, 1);

    // Orden #10 (ABIERTA - 1 línea) - Vehículo 8, Mecánico 3
    abrirOrden(8, 3, "2026-11-18", "Mantenimiento de los 15.000 km", 15500, idOrd);
    agregarRepuestoAOrden(idOrd, 11, 4);
}

int main () {
    SetConsoleOutputCP(65001);

    //Iniciamos el sistema
    SistemaTaller miTaller;
    inicializarSistema(&miTaller, "Autoservicio El Wikiti", "J-12345678-9", "Maracaibo", "0414-1234567");
    
    int mayuya = 0;
    int mayuya2 = 0;

    do {
        limpiar();
        menuPrincipal();

        if (!(cin >> mayuya)){
            if (cin.eof()) {
                mayuya = 0; // Si se acabó el archivo, forzamos la salida limpia
                break;
            }
            cin.clear();
            cin.ignore(10000, '\n');
            mayuya = -1;
        }

        switch (mayuya) {
            case 1: {
                do{
                    limpiar();
                    menuVehículos();
                    
                    if (!(cin >> mayuya2)){
                        if (cin.eof()) {
                            mayuya2 = 0; // Salida limpia si el archivo termina aquí
                            break;
                        }
                        cin.clear();
                        cin.ignore(10000, '\n');
                        mayuya2 = -1;
                    }

                    switch (mayuya2) {
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
                            limpiar();
                            break;
                    }
                }while(mayuya2 != 0);
                limpiar();
                break;
            }case 2: {
                do{
                    limpiar();
                    menuMecánicos();
                
                    if (!(cin >> mayuya2)){
                        if (cin.eof()) {
                            mayuya2 = 0; // Salida limpia si el archivo termina aquí
                            break;
                        }
                        cin.clear();
                        cin.ignore(10000, '\n');
                        mayuya2 = -1;
                    }

                    switch (mayuya2) {
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
                            limpiar();
                            break;
                    }
                }while(mayuya2 != 0);
                limpiar();
                break;
            
            } case 3: {
                do {
                    limpiar();
                    menuRepuestos();

                    if (!(cin >> mayuya2)){
                        if (cin.eof()) {
                            mayuya2 = 0; // Salida limpia si el archivo termina aquí
                            break;
                        }
                        cin.clear();
                        cin.ignore(10000, '\n');
                        mayuya2 = -1;
                    }

                    switch (mayuya2){
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
                            limpiar();
                            break;
                    }
                } while(mayuya2 != 0);
                limpiar();
                break;
            } case 4: {
                do{
                    limpiar();
                    menuÓrdenes();

                    if (!(cin >> mayuya2)){
                        if (cin.eof()) {
                            mayuya2 = 0; // Salida limpia si el archivo termina aquí
                            break;
                        }
                        cin.clear();
                        cin.ignore(10000, '\n');
                        mayuya2 = -1;
                    }
                    
                    switch (mayuya2){
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
                            limpiar();
                            break;
                    }
                }while(mayuya2 != 0);
                break;

            } case 0: {
                cout << "Cerrando programa. Hasta pronto..." << endl;
                break;
            } default:
                limpiar();
                cout << "Escoja una opción válida (0-4).\n" << endl;
            break;
        }
    } while (mayuya != 0);

liberarSistema(&miTaller);
//miau
//opcion y opcion2 pasaron a llamarse mayuya por el homenaje a tan hermoso ser
return 0;
}
*/