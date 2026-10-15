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
    int id;
    char placa[10];
    char marca[50];
    char modelo[50];
    int anio;
    int kilometraje;
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


//==========================================
//3.0 CAPA DE LOGICA Funciones del sistema
//==========================================

//3.0 Inializar el sistema (reserva capacidad 4 para entidades y 2 para detalles)
void inicalizarSistema(SistemaTaller* s, const char* nombre, const char* rif, const char* direccion, const char* telefono){
    
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

Vehiculo* agregarVehiculo(SistemaTaller* s, Vehiculo datos);

Vehiculo* buscarVehiculoPorID(SistemaTaller* s, int id);

Vehiculo* buscarVehiculoPorPlaca(SistemaTaller* s, const char* placa);

Vehiculo** buscarVehiculosPorPropietario(SistemaTaller* s, const char* subcadena, int* cantidad);

Vehiculo** listarVehiculos(SistemaTaller* s, int* cantidad);

bool actualizarVehiculo(SistemaTaller* s, int id, Vehiculo datos);

bool eliminarVehiculo(SistemaTaller* s, int id);

//menus
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

void menuMecánicos () {
    cout << "╔═══════════════════════════════════════════╗" << endl;
    cout << "║            GESTIÓN DE MECÁNICOS           ║" << endl;
    cout << "╠═══════════════════════════════════════════╣" << endl;
    cout << "║ 1. Registrar mecánico                     ║" << endl;
    cout << "║ 2. Buscar por ID                          ║" << endl;
    cout << "║ 3. Registrar entrada de mercancía         ║" << endl;
    cout << "║ 4. Listar todos los mecánicos             ║" << endl;
    cout << "║ 0. Volver al menú principal               ║" << endl;
    cout << "╚═══════════════════════════════════════════╝" << endl;
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
    cout << "║ 7. Listar órdenes                         ║" << endl;
    cout << "║ 0. Volver al menú principal               ║" << endl;
    cout << "╚═══════════════════════════════════════════╝" << endl;
}

void limpiarPantalla () {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

int main () {
    SetConsoleOutputCP(65001);
    
    int opcion = 0;

    do {
    menuPrincipal();
    cin >> opcion;

    /*if (!(cin >> opcion)) {
        cout << "Entrada Inválida." << endl;
        cin.clear();
        cin.ignore(10000,'\n');
    }*/

    switch (opcion) {
        case 1: {
            limpiarPantalla();
            menuVehículos();
            cin >> opcion;
            switch (opcion) {
                case 1: {
                    limpiarPantalla();
                    cout << "Probandou comandou carrou1" << endl;
                    break;
                } case 2: {
                    limpiarPantalla();
                    cout << "Probandou comandou carrou2" << endl;
                    break;
                } case 3: {
                    limpiarPantalla();
                    cout << "Probandou comandou carrou3" << endl;
                    break;
                } case 4: {
                    limpiarPantalla();
                    cout << "Probandou comandou carrou4" << endl;
                    break;
                } case 5: {
                    limpiarPantalla();
                    cout << "Probandou comandou carrou5" << endl;
                    break;
                } case 6: {
                    limpiarPantalla();
                    cout << "Probandou comandou carrou6" << endl;
                    break;
                } case 7: {
                    limpiarPantalla();
                    cout << "Probandou comandou carrou7" << endl;
                    break;
                } case 0: {
                    limpiarPantalla();
                    break;
                }
            break;

        } case 2: {
            limpiarPantalla();
            menuMecánicos();
            cin >> opcion;
            limpiarPantalla();
            break;

        } case 3: {
            limpiarPantalla();
            menuRepuestos();
            cin >> opcion;
            limpiarPantalla();
            break;

        } case 4: {
            limpiarPantalla();
            menuÓrdenes();
            cin >> opcion;
            limpiarPantalla();
            break;

        } case 0: {
            cout << "Cerrando programa. Hasta pronto..." << endl;
            break;
        } default:
            limpiarPantalla();
            cout << "Escoja una opción válida (0-4).\n" << endl;
        break;
    }
}
} while (opcion != 0);
return 0;
}
