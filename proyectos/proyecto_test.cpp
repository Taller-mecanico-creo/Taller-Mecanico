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

CodigoResultado eliminarRepuesto(int idRepuesto){
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

// ============================================================================
//         MÓDULO DE MANTENIMIENTO: INTEGRIDAD Y BACKUP (SECCIÓN 7)
// ============================================================================



/* ojo
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
    if (!(cin >> id)) {
        cin.clear(); cin.ignore(10000, '\n'); return;
    }

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
    limpiar();
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
    limpiar();
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
        limpiar();
        return;
    }
    cout << "Marca: "; cin >> nuevo.marca;
    if (esCancelar(nuevo.marca)){
        cout <<"\nRegistro cancelado por el usuario." << endl;
        cin.ignore(10000, '\n');
        limpiar();
        return;
    }
    cout << "Modelo: "; cin >> nuevo.modelo;
    if (esCancelar(nuevo.modelo)){
        cout <<"\nRegistro cancelado por el usuario." << endl;
        cin.ignore(10000, '\n');
        limpiar();
        return;
    }
    cout << "Año (1950-2027): ";
    if (!(cin >> nuevo.anio)) { cin.clear(); cin.ignore(10000, '\n'); nuevo.anio = 0; }
    
    cout << "Kilometraje (>= 0): ";
    if (!(cin >> nuevo.kilometraje)) { cin.clear(); cin.ignore(10000, '\n'); nuevo.kilometraje = -1; }
    
    // Limpiamos el salto de linea antes de usar getline
    cin.ignore(10000, '\n');
    cout << "Nombre del Propietario: "; cin.getline(nuevo.nombrePropietario, 100);
    if (esCancelar(nuevo.nombrePropietario)) {
        cout << "\nRegistro cancelado por el usuario." << endl;
        limpiar();
        return;
    }
    cout << "Cedula del Propietario: "; cin >> nuevo.cedulaPropietario;
    if (esCancelar(nuevo.cedulaPropietario)) {
        cout << "\nRegistro cancelado por el usuario." << endl;
        cin.ignore(10000, '\n');
        limpiar();
        return;
    }
    cout << "Telefono del Propietario: "; cin >> nuevo.telefonoPropietario;
    cout << "Fecha de Registro (YYYY-MM-DD): "; cin >> nuevo.fechaRegistro;

    if (buscarVehiculoPorPlaca(s, nuevo.placa) != nullptr) {
        cout << "\nERROR: La placa '" << nuevo.placa << "' ya está registrada." << endl;
        cin.ignore(10000, '\n');
        limpiar();
        return;
    }

    if (!pedirConfirmacion("\n¿Confirmar el registro del vehículo?")) {
        cout << "\nRegistro cancelado." << endl;
        cin.ignore(10000, '\n');
        limpiar();
        return;
    }

    Vehiculo* creado = agregarVehiculo(s, nuevo);
    if (creado != nullptr) {
        cout << "\nVehiculo '" << creado->placa << "' registrado exitosamente con ID #" << creado->id << "." << endl;
    } else {
        cout << "\nERROR: No se pudo registrar el vehiculo (verifique los campos anteriores)." << endl;
    }
    cin.ignore(10000, '\n');
    limpiar();
}
    
void uiBuscarVehiculo(SistemaTaller* s) {
    limpiar();
    cout << "=== BUSCAR VEHICULO ===" << endl;
    int op = 0;
    cout << "Seleccione el modo de busqueda: " << endl;
    cout << "1. Buscar por ID" << endl;
    cout << "2. Buscar por Placa" << endl;
    cout << "3. Buscar por Propietario" << endl;
    
    if (!(cin >> op)) {
        cin.clear();
        cin.ignore(10000, '\n');
        return;
    }

    switch (op) {
        case 1:{
            int idBuscado;
            cout << "Ingrese un ID registrado: ";
            if (!(cin >> idBuscado)) { cin.clear(); cin.ignore(10000, '\n'); break; }
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
    limpiar();
}

void uiActualizarVehiculos(SistemaTaller* s){
    limpiar();
    cout << "=== ACTUALIZAR VEHICULO ===" << endl;
    int id;
    cout << "Ingrese el ID del vehiculo a actualizar: ";
    if (!(cin >> id)) {
        cin.clear();
        cin.ignore(10000, '\n');
        return;
    }

    Vehiculo* v = buscarVehiculoPorID(s, id);
    if (v == nullptr){
        cout << "\nError: No existe un vehiculo registrado con el ID " << id << "." << endl;
        cin.ignore(10000, '\n');
        limpiar();
        return;
    }

    cout << "\nVehiculo actual encontrado: " << endl;
    mostrarVehiculo(v);

    Vehiculo nuevosDatos;
    cout << "\nIngrese los nuevos datos (Placa y kilometraje no se modifican): " << endl;
    cout << "Nueva Marca: "; cin >> nuevosDatos.marca;
    cout << "Nuevo Modelo: "; cin >> nuevosDatos.modelo;
    cout << "Nuevo anio (1950 - 2027): ";
    if (!(cin >> nuevosDatos.anio)) { cin.clear(); cin.ignore(10000, '\n'); nuevosDatos.anio = 0; }

    cin.ignore(10000, '\n');
    cout << "Nuevo Nombre del Propietario: "; cin.getline(nuevosDatos.nombrePropietario, 100);
    cout << "Nueva Cédula del Propietario: "; cin >> nuevosDatos.cedulaPropietario;
    cout << "Nuevo Teléfono del Propietario: "; cin >> nuevosDatos.telefonoPropietario;
    
    if (!pedirConfirmacion("\nConfirmar la actualizacion del vehiculo?")){
        cout << "\nActualización cancelada." << endl;
        cin.ignore(10000, '\n');
        limpiar();
        return; 
    }

    if (actualizarVehiculo(s, id, nuevosDatos)){
        cout << "\nVehiculo actualizado sasticfactoriamente." << endl;
    } else{
        cout << "\nError: No se pudo actualizar (verifique el año entre 1950-2027) y cédula válida." << endl;
    }
    cin.ignore(10000, '\n');
    limpiar();
    
}

void uiEliminarVehiculo(SistemaTaller* s){
    limpiar();
    cout << "=== ELIMINAR VEHICULO ===" << endl;
    int id;
    cout << "Ingrese la ID del Vehiculo a eliminar: ";
    if (!(cin >> id)){
        cin.clear();
        cin.ignore(10000, '\n');
        return;
    }

    if (eliminarVehiculo(s, id)){
        cout << "\nVehiculo eliminado fisicamente del sistema." << endl;
    } else{
        cout << "\nNo se pudo eliminar (ID no existe o el vehiculo tiene ordenes de servicio asociadas)." << endl;
    }
    cin.ignore(10000, '\n');
    limpiar();
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
    limpiar();
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
    limpiar();
}

void uiBuscarMecanicoID(SistemaTaller* s) {
    limpiar();
    cout << "=== BUSCAR MECANICO POR ID ===" << endl;
    int idBuscado;

    cout << "Ingrese un ID registrado: ";
    if (!(cin >> idBuscado)){
        cin.clear();
        cin.ignore(10000, '\n');
        return;
    }

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
    limpiar();
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
    limpiar();
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

    cout << "Precio unitario (>0): "; 
    if (!(cin >> nuevo.precio)) { cin.clear(); cin.ignore(10000, '\n'); nuevo.precio = 0.0f; }
    
    cout << "Stock inicial (>= 0): "; 
    if (!(cin >> nuevo.stock)) { cin.clear(); cin.ignore(10000, '\n'); nuevo.stock = -1; }
    
    cout << "Stock mínimo para alerta (>= 0): "; 
    if (!(cin >> nuevo.stockMinimo)) { cin.clear(); cin.ignore(10000, '\n'); nuevo.stockMinimo = -1; }
    
    cout << "Fecha de registro (YYYY-MM-DD): "; cin >> nuevo.fechaRegistro;
    cin.ignore(10000, '\n');

    // Verificamos si el código ya existe para dar un mensaje de error
    if (buscarRepuestoPorCodigo(s, nuevo.codigo) != nullptr) {
        cout << "\nERROR: El código de repuesto '" << nuevo.codigo << "' ya está registrado." << endl;
        limpiar();
        return;
    }

    Repuesto* creado = registrarRepuesto(s, nuevo);
    if (creado != nullptr) {
        cout << "\nRepuesto '" << creado->nombre << "' registrado exitosamente con ID #" << creado->id << endl;
    } else {
        cout << "\nERROR: No se pudo registrar el repuesto (verifique precio, stock y campos no vacíos)." << endl;
    }
    limpiar();
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
            int idBuscado;
            cout << "Ingrese el ID del repuesto: ";
            if (!(cin >> idBuscado)) { cin.clear(); cin.ignore(10000, '\n'); break; }
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
    limpiar();
}

void uiEntradaMercancia(SistemaTaller* s) {
    limpiar();
    cout << "=== REGISTRAR ENTRADA DE MERCANCÍA ===" << endl;
    int idRepuesto, cantidad;

    cout << "Ingrese el ID del repuesto: ";
    if (!(cin >> idRepuesto)){
        cin.clear();
        cin.ignore(10000, '\n');
        return;
    }

    Repuesto* rep = buscarRepuestoPorID(s, idRepuesto);
    if (rep == nullptr) {
        cin.ignore(10000, '\n');
        cout << "\nERROR: No existe un repuesto con el ID #" << idRepuesto << "." << endl;
        limpiar();
        return;
    }   
    cout << "Repuesto seleccionado: " << rep->nombre << " (Stock actual: " << rep->stock << ")" << endl;
    cout << "Ingrese la cantidad de unidades que ingresan (> 0): ";
    if (!(cin >> cantidad)) { cin.clear(); cin.ignore(10000, '\n'); return; }
    cin.ignore(10000, '\n');

    if (registrarEntradaMercancia(s, idRepuesto, cantidad)) {
        cout << "\n¡Entrada registrada! Nuevo stock de '" << rep->nombre << "': " << rep->stock << "unidades." << endl;
    } else { 
        cout << "\nERROR: La cantidad a ingresar debe ser mayor a 0." << endl;
    }
    limpiar();
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
    limpiar();
}

void uiCambiarPrecioRepuesto(SistemaTaller* s) {
    limpiar();
    cout << "=== CAMBIAR PRECIO DE REPUESTO ===" << endl;
    int idRepuesto;
    float nuevoPrecio;

    cout << "Ingrese el ID del repuesto: ";
    if (!(cin >> idRepuesto)) { cin.clear(); cin.ignore(10000, '\n'); return; }

    Repuesto* rep = buscarRepuestoPorID(s, idRepuesto);
    if (rep == nullptr) {
        cin.ignore(10000, '\n');
        cout <<"\nERROR: No existe un repuesto con el ID #" << idRepuesto << "." << endl;
        limpiar();
        return;
    }

    cout << "Repuesto: " << rep->nombre << " | Precio actual: $" << rep->precio << "." << endl;
    cout << "Ingrese el nuevo precio unitario (> 0): ";
    if (!(cin >> nuevoPrecio)) { cin.clear(); cin.ignore(10000, '\n'); return; }
    cin.ignore(10000, '\n');

    if (cambiarPrecioRepuesto(s, idRepuesto, nuevoPrecio)) {
        cout << "\nPrecio actualizado exitosamente a $" << rep->precio  << "." << endl;
        cout << "(Nota: Las órdenes registradas previamente mantienen su precio histórico)." << endl;
    } else {
        cout << "\nERRROR: El precio debe ser mayor o igual a 0." << endl;
    }
    limpiar();
}

void uiEliminarRepuesto(SistemaTaller* s) {
    limpiar();
    cout << "=== ELIMINAR REPUESTO ===" << endl;
    int idRepuesto;

    cout << "Ingrese el ID del repuesto a eliminar: ";
    if (!(cin >> idRepuesto)) { cin.clear(); cin.ignore(10000, '\n'); return; }
    cin.ignore(10000, '\n');

    Repuesto* rep = buscarRepuestoPorID(s, idRepuesto);
    if (rep == nullptr) {
        cout << "\nERROR: No existe ningún repuesto con el ID #" << idRepuesto << "." << endl;
        limpiar();
        return;
    }

    if (eliminarRepuesto(s, idRepuesto)) {
        cout << "\nRepuesto eliminado del inventario." << endl;
    } else {
        cout << "\nERROR: El repuesto '" << rep->nombre << "' está registrado en una o más órdenes de servicio y no puede eliminarse." << endl;
    }
    limpiar();
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