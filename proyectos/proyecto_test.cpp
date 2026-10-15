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
    //Validamos anio (1950 - 2027) y kilometraje (>0)
    if(datos.anio < 1950 || datos.anio > 2027 || datos.kilometraje <= 0){
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
    
    cin.ignore(10000, '\n');
    
    cout << "Nombre del Propietario: ";
    cin.getline(nuevo.nombrePropietario, 100);

    agregarVehiculo(s, nuevo);
    cout << "\nVehiculo registrado exitosamente con ID asignado!" << endl;
    pausarylimpiar();

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
    

    //Iniciamos el sistema
    SistemaTaller miTaller;
    inicializarSistema(&miTaller, "Autoservicio El Wikiti", "J-12345678-9", "Maracaibo", "0414-1234567");
    
    int opcion = 0;
    int opcion2 = 0;

    do {
        limpiar();
        menuPrincipal();
        cin >> opcion;

    /*if (!(cin >> opcion)) {
        cout << "Entrada Inválida." << endl;
        cin.clear();
        cin.ignore(10000,'\n');
    }*/

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

    } while (opcion != 0);

return 0;
}
