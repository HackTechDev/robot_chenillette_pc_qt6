// BatMobile — télécommande PC (Qt6) du robot Zumo, en Bluetooth classique (SPP/RFCOMM).
//
// Même protocole que l'appli Android (../robot_chenillette_telecommande) : un caractère
// envoyé à l'appui d'un bouton (ou d'une touche), un espace (stop) au relâchement,
// aucune répétition pendant l'appui.

#include "window.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Window w;
    w.resize(480, 760);
    w.show();
    return app.exec();
}
