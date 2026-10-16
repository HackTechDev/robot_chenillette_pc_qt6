#pragma once

#include "link.h"

#include <QList>
#include <QWidget>

class Pairing;
class QComboBox;
class QLabel;
class QPushButton;
class QSlider;

class Window : public QWidget
{
    Q_OBJECT

public:
    explicit Window(QWidget *parent = nullptr);

    // Accès pour les tests hors écran (pas de robot)
    Link &link() { return link_; }
    void setConnected(bool on);

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    void closeEvent(QCloseEvent *e) override;

private:
    struct DirButton {
        QPushButton *button;
        char cmd;  // 0 = bouton stop
    };

    QPushButton *dirButton(const QString &label, char cmd, int fontSize = 0);

    void refreshDevices();
    QString selectedAddress() const;
    bool autoPairingSelected() const;
    void toggleConnection();
    void connectTo(const QString &address);
    void startPairing();
    void onConnected(const QString &address);
    void onFailed(const QString &error);
    void onLost();
    void setButtonsEnabled(bool on);

    void updateSpeedLabel();
    void onSpeed(int level);

    void setHeld(char cmd, bool held);
    bool confirmQuit();

    Link link_;
    bool connected_ = false;
    QString connectedName_;
    int heldKey_ = 0;  // 0 = aucune touche maintenue
    Pairing *pairing_ = nullptr;

    QComboBox *deviceBox_;
    QPushButton *btButton_;
    QLabel *status_;
    QSlider *speed_;
    QLabel *speedValue_;
    QList<DirButton> buttons_;
};
