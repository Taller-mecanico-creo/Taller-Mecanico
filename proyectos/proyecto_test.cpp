#include <iostream>
#include <windows.h>
using namespace std;

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
