#include <iostream>
#include <windows.h>
using namespace std;

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

Vehiculo* agregarVehiculo(SistemaTaller* s, Vehiculo datos){
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

        //comparamos la placa del vehiculo acual caracter por caracter
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

    //4. Llenamos el arreglo con las direcciones de memoria de cada vegiculo existente
    for(int i = 0; i < s->numVehiculos; i++){
        lista[i] = &(s->vehiculos[i]);
    }

    //Retornamos el arreglo de punteros
    return lista;
}

bool actualizarVehiculo(SistemaTaller* s, int id, Vehiculo datos);

bool eliminarVehiculo(SistemaTaller* s, int id);

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

int main () {
    SetConsoleOutputCP(65001);
    
    int opcion = 0;
    int opcion2 = 0;

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
            limpiar();
            menuVehículos();
            cin >> opcion2;
            switch (opcion2) {
                case 1: {
                    pausarylimpiar();
                    limpiar();
                    cout << "Probandou comandou carrou1" << endl;
                    break;
                } case 2: {
                    limpiar();
                    cout << "Probandou comandou carrou2" << endl;
                    break;
                } case 3: {
                    limpiar();
                    cout << "Probandou comandou carrou3" << endl;
                    break;
                } case 4: {
                    pausarylimpiar();
                    cout << "Probandou comandou carrou4" << endl;
                    break;
                } case 5: {
                    pausarylimpiar();
                    cout << "Probandou comandou carrou5" << endl;
                    break;
                } case 6: {
                    pausarylimpiar();
                    cout << "Probandou comandou carrou6" << endl;
                    break;
                } case 7: {
                    pausarylimpiar();
                    cout << "Probandou comandou carrou7" << endl;
                    break;
                } case 0: {
                    pausarylimpiar();
                    break;
                }
            break;

        } case 2: {
            limpiar();
            menuMecánicos();
            cin >> opcion2;
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
}
} while (opcion != 0);
return 0;
}
