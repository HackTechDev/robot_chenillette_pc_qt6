#include "window.h"

#include "bluez.h"
#include "config.h"
#include "pairing.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHash>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSlider>
#include <QStyle>
#include <QVBoxLayout>

namespace {

// Touches clavier → commande maintenue (Qt::Key ne dépend pas de la disposition AZERTY/QWERTY)
const QHash<int, char> HoldKeys = {
    {Qt::Key_Z, 'z'}, {Qt::Key_Up, 'z'},
    {Qt::Key_S, 's'}, {Qt::Key_Down, 's'},
    {Qt::Key_Q, 'q'}, {Qt::Key_Left, 'q'},
    {Qt::Key_D, 'd'}, {Qt::Key_Right, 'd'},
    {Qt::Key_V, 'v'},
    {Qt::Key_C, 'c'},
    {Qt::Key_W, 'w'},
    {Qt::Key_X, 'x'},
};

const char *const Style = R"(
QWidget { background: #111; color: #fff; }
QLabel#title { color: #FFD700; font-size: 28px; font-weight: bold; letter-spacing: 2px; }
QLabel#hint { color: #888; font-size: 11px; }
QLabel#speedValue { color: #FFD700; font-size: 13px; font-weight: bold; }
QComboBox { background: #2a2a2a; border: 1px solid #555; border-radius: 6px; padding: 6px; }
QPushButton#bt { border: 2px solid #555; border-radius: 18px; padding: 8px 24px; font-size: 15px; }
QPushButton#bt[connected="true"] { border-color: #4caf50; }
QPushButton#small { border: 1px solid #555; border-radius: 6px; padding: 6px 10px; }
QPushButton#quit { border: 1px solid #555; border-radius: 16px; color: #888; font-size: 15px; font-weight: bold; }
QPushButton#quit:hover { border-color: #e53935; color: #e53935; }
QMessageBox QPushButton { border: 1px solid #555; border-radius: 6px; padding: 6px 16px; min-width: 60px; }
QPushButton#dir { background: #FFD700; color: #111; border-radius: 14px; font-size: 30px; font-weight: bold; }
QPushButton#dir:pressed, QPushButton#dir[held="true"] { background: #c9a800; }
QPushButton#dir:disabled { background: #2a2a2a; color: #555; }
QSlider::groove:vertical { background: #2a2a2a; width: 28px; border-radius: 14px; }
QSlider::sub-page:vertical { background: #2a2a2a; border-radius: 14px; }
QSlider::add-page:vertical { background: #FFD700; border-radius: 14px; }
QSlider::handle:vertical { background: #fff; height: 14px; margin: 0 2px; border-radius: 7px; }
)";

QLabel *label(const QString &text, const QString &name)
{
    auto *l = new QLabel(text);
    l->setObjectName(name);
    l->setAlignment(Qt::AlignCenter);
    return l;
}

QPushButton *button(const QString &text, const QString &name, const QString &toolTip = {})
{
    auto *b = new QPushButton(text);
    b->setObjectName(name);
    b->setToolTip(toolTip);
    return b;
}

void repolish(QWidget *w)
{
    w->style()->unpolish(w);
    w->style()->polish(w);
}

QHBoxLayout *centered(const QList<QWidget *> &widgets)
{
    auto *row = new QHBoxLayout;
    row->addStretch();
    for (QWidget *w : widgets)
        row->addWidget(w);
    row->addStretch();
    return row;
}

}  // namespace

Window::Window(QWidget *parent) : QWidget(parent)
{
    setWindowTitle(RobotName);
    setStyleSheet(Style);
    setFocusPolicy(Qt::StrongFocus);

    connect(&link_, &Link::connected, this, &Window::onConnected);
    connect(&link_, &Link::failed, this, &Window::onFailed);
    connect(&link_, &Link::lost, this, &Window::onLost);

    QLabel *title = label(RobotName, QStringLiteral("title"));

    // Choix de l'appareil
    deviceBox_ = new QComboBox;
    deviceBox_->setEditable(true);
    deviceBox_->setInsertPolicy(QComboBox::NoInsert);  // une adresse saisie reste du texte libre
    deviceBox_->setMinimumWidth(260);
    deviceBox_->lineEdit()->setPlaceholderText(QStringLiteral("Adresse (ex. 00:18:A1:12:16:C5)"));
    QPushButton *refresh = button(QStringLiteral("⟳"), QStringLiteral("small"),
                                  QStringLiteral("Rafraîchir les appareils appairés"));
    connect(refresh, &QPushButton::clicked, this, &Window::refreshDevices);
    btButton_ = button(QStringLiteral("Connecter"), QStringLiteral("bt"));
    btButton_->setMinimumWidth(240);
    connect(btButton_, &QPushButton::clicked, this, &Window::toggleConnection);
    status_ = label(QString(), QStringLiteral("hint"));

    // Vitesse (slider vertical, à gauche des déplacements) : haut = rapide
    speed_ = new QSlider(Qt::Vertical);
    speed_->setRange(0, SpeedLevels - 1);
    speed_->setValue(SpeedLevelDefault);
    speed_->setPageStep(1);
    connect(speed_, &QSlider::valueChanged, this, &Window::onSpeed);
    speedValue_ = label(QString(), QStringLiteral("speedValue"));
    auto *speedCol = new QVBoxLayout;
    speedCol->addWidget(speedValue_);
    speedCol->addWidget(speed_, 1, Qt::AlignHCenter);
    speedCol->addWidget(label(QStringLiteral("Vit."), QStringLiteral("hint")));
    updateSpeedLabel();

    auto *dpad = new QGridLayout;
    dpad->setSpacing(10);
    dpad->addWidget(dirButton(QStringLiteral("▲"), 'z'), 0, 1);
    dpad->addWidget(dirButton(QStringLiteral("↺"), 'q'), 1, 0);
    dpad->addWidget(dirButton(QStringLiteral("■"), 0), 1, 1);
    dpad->addWidget(dirButton(QStringLiteral("↻"), 'd'), 1, 2);
    dpad->addWidget(dirButton(QStringLiteral("▼"), 's'), 2, 1);
    auto *drive = new QHBoxLayout;
    drive->addStretch();
    drive->addLayout(speedCol);
    drive->addSpacing(12);
    drive->addLayout(dpad);
    drive->addSpacing(52);  // contrepoids du slider pour garder le pad centré
    drive->addStretch();

    QHBoxLayout *claw = centered({dirButton(QStringLiteral("🤏"), 'v'), dirButton(QStringLiteral("✋"), 'c')});
    QHBoxLayout *head = centered({dirButton(QStringLiteral("Tête ▲"), 'w', 18),
                                  dirButton(QStringLiteral("Tête ▼"), 'x', 18)});

    QLabel *hint = label(QStringLiteral("Clavier : Z/S/Q/D ou flèches = déplacement · V/C = pince · W/X = tête · "
                                        "Espace = stop · 0–9 = vitesse"),
                         QStringLiteral("hint"));
    hint->setWordWrap(true);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 20, 16, 16);
    root->setSpacing(10);
    root->addWidget(title);
    root->addLayout(centered({deviceBox_, refresh}));
    root->addLayout(centered({btButton_}));
    root->addWidget(status_);
    root->addSpacing(20);
    root->addLayout(drive);
    root->addLayout(claw);
    root->addLayout(head);
    root->addStretch();

    // Bas de fenêtre : aide clavier + bouton « ✕ » (quitter) en bas à droite
    QPushButton *quit = button(QStringLiteral("✕"), QStringLiteral("quit"), QStringLiteral("Quitter"));
    quit->setFixedSize(32, 32);
    connect(quit, &QPushButton::clicked, this, &QWidget::close);  // confirmation dans closeEvent
    auto *bottom = new QHBoxLayout;
    bottom->addSpacing(32);  // contrepoids du bouton pour garder l'aide centrée
    bottom->addWidget(hint, 1);
    bottom->addWidget(quit, 0, Qt::AlignBottom);
    root->addLayout(bottom);

    // Le clavier pilote le robot : aucun widget ne doit garder le focus (Espace activerait un bouton)
    for (QPushButton *b : findChildren<QPushButton *>())
        b->setFocusPolicy(Qt::NoFocus);
    speed_->setFocusPolicy(Qt::NoFocus);
    deviceBox_->setFocusPolicy(Qt::ClickFocus);
    connect(deviceBox_->lineEdit(), &QLineEdit::returnPressed, this, [this] { setFocus(); });

    refreshDevices();
    setButtonsEnabled(false);
}

QPushButton *Window::dirButton(const QString &text, char cmd, int fontSize)
{
    // Bouton maintenu : cmd à l'appui, stop au relâchement (cmd 0 = stop seul)
    QPushButton *b = button(text, QStringLiteral("dir"));
    b->setFixedSize(90, 90);
    if (fontSize)
        b->setStyleSheet(QStringLiteral("font-size: %1px;").arg(fontSize));
    if (cmd == 0) {
        connect(b, &QPushButton::pressed, this, [this] { link_.send(Stop); });
    } else {
        connect(b, &QPushButton::pressed, this, [this, cmd] { link_.send(cmd); });
        connect(b, &QPushButton::released, this, [this] { link_.send(Stop); });
    }
    buttons_.append({b, cmd});
    return b;
}

// --- Connexion ---

void Window::refreshDevices()
{
    const QString current = deviceBox_->currentText();
    deviceBox_->clear();
    const QList<bluez::Device> devices = bluez::pairedDevices();
    int robot = -1;
    for (const bluez::Device &d : devices) {
        if (robot < 0 && d.name == QLatin1String(RobotName))
            robot = deviceBox_->count();
        deviceBox_->addItem(QStringLiteral("%1 — %2").arg(d.name, d.address), d.address);
    }
    if (robot >= 0) {
        deviceBox_->setCurrentIndex(robot);
    } else {
        // Robot pas encore appairé : « Connecter » lancera l'appairage automatique
        deviceBox_->insertItem(0, QStringLiteral("%1 — appairage automatique").arg(RobotName), QVariant());
        deviceBox_->setCurrentIndex(0);
        if (!current.isEmpty() && current != deviceBox_->itemText(0))
            deviceBox_->setEditText(current);
    }
}

QString Window::selectedAddress() const
{
    const QString text = deviceBox_->currentText();
    const int i = deviceBox_->currentIndex();
    if (i >= 0 && text == deviceBox_->itemText(i))
        return deviceBox_->currentData().toString();
    static const QRegularExpression re(QStringLiteral("([0-9A-Fa-f]{2}(:[0-9A-Fa-f]{2}){5})"));
    const QRegularExpressionMatch m = re.match(text);
    return m.hasMatch() ? m.captured(1).toUpper() : QString();
}

bool Window::autoPairingSelected() const
{
    const int i = deviceBox_->currentIndex();
    return i >= 0 && !deviceBox_->currentData().isValid() && deviceBox_->currentText() == deviceBox_->itemText(i);
}

void Window::toggleConnection()
{
    if (connected_) {
        link_.disconnectFromRobot();
        setConnected(false);
        return;
    }
    if (autoPairingSelected())
        return startPairing();
    const QString address = selectedAddress();
    if (address.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Appareil"),
                             QStringLiteral("Choisir un appareil appairé ou saisir une adresse Bluetooth."));
        return;
    }
    connectTo(address);
}

void Window::connectTo(const QString &address)
{
    btButton_->setText(QStringLiteral("Connexion…"));
    btButton_->setEnabled(false);
    deviceBox_->setEnabled(false);
    status_->clear();
    link_.connectTo(address, char('0' + speed_->value()));
}

void Window::startPairing()
{
    // Recherche, PIN, confiance, puis connexion au robot
    btButton_->setText(QStringLiteral("Appairage…"));
    btButton_->setEnabled(false);
    deviceBox_->setEnabled(false);
    status_->setText(QStringLiteral("Recherche de %1…").arg(RobotName));
    repaint();  // les premiers appels D-Bus sont bloquants

    pairing_ = new Pairing(false, 30, this);
    connect(pairing_, &Pairing::progress, status_, &QLabel::setText);
    connect(pairing_, &Pairing::succeeded, this, [this](const QString &address) {
        pairing_->deleteLater();
        pairing_ = nullptr;
        refreshDevices();
        connectTo(address);
    });
    connect(pairing_, &Pairing::failed, this, [this](const QString &error) {
        pairing_->deleteLater();
        pairing_ = nullptr;
        setConnected(false);
        status_->clear();
        QMessageBox::critical(this, QStringLiteral("Appairage échoué"), error);
    });
    pairing_->start();
}

void Window::onConnected(const QString &address)
{
    connectedName_ = deviceBox_->currentText().section(QStringLiteral(" — "), 0, 0);
    if (connectedName_.isEmpty())
        connectedName_ = address;
    setConnected(true);
}

void Window::onFailed(const QString &error)
{
    setConnected(false);
    status_->clear();
    QMessageBox::critical(this, QStringLiteral("Connexion échouée"), error);
}

void Window::onLost()
{
    setConnected(false);
    QMessageBox::warning(this, QStringLiteral("Liaison perdue"),
                         QStringLiteral("La connexion Bluetooth avec le robot a été perdue."));
}

void Window::setConnected(bool on)
{
    connected_ = on;
    heldKey_ = 0;
    btButton_->setEnabled(true);
    btButton_->setText(on ? QStringLiteral("Connecté — %1").arg(connectedName_) : QStringLiteral("Connecter"));
    btButton_->setProperty("connected", on);
    repolish(btButton_);
    deviceBox_->setEnabled(!on);
    setButtonsEnabled(on);
    setFocus();
}

void Window::setButtonsEnabled(bool on)
{
    for (const DirButton &d : buttons_)
        d.button->setEnabled(on);
}

// --- Vitesse ---

void Window::updateSpeedLabel()
{
    speedValue_->setText(QString::number((speed_->value() + 1) * SpeedPerLevel));
}

void Window::onSpeed(int level)
{
    updateSpeedLabel();
    link_.send(char('0' + level));
}

// --- Clavier ---

void Window::setHeld(char cmd, bool held)
{
    for (const DirButton &d : buttons_) {
        if (d.cmd == cmd) {
            d.button->setProperty("held", held);
            repolish(d.button);
        }
    }
}

void Window::keyPressEvent(QKeyEvent *e)
{
    if (e->isAutoRepeat() || !connected_)
        return QWidget::keyPressEvent(e);
    const int key = e->key();
    if (const auto it = HoldKeys.constFind(key); it != HoldKeys.cend()) {
        heldKey_ = key;
        link_.send(*it);
        setHeld(*it, true);
    } else if (key == Qt::Key_Space) {
        link_.send(Stop);
    } else if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        speed_->setValue(key - Qt::Key_0);  // envoie le chiffre via onSpeed
    } else {
        QWidget::keyPressEvent(e);
    }
}

void Window::keyReleaseEvent(QKeyEvent *e)
{
    const int key = e->key();
    const auto it = HoldKeys.constFind(key);
    if (e->isAutoRepeat() || it == HoldKeys.cend())
        return QWidget::keyReleaseEvent(e);
    setHeld(*it, false);
    // Stop uniquement si c'est la dernière touche pressée (relâcher une ancienne touche ne coupe pas)
    if (key == heldKey_) {
        heldKey_ = 0;
        link_.send(Stop);
    }
}

void Window::focusOutEvent(QFocusEvent *e)
{
    // Fenêtre quittée touche enfoncée : on ne recevra pas le relâchement
    if (heldKey_ != 0) {
        setHeld(HoldKeys.value(heldKey_), false);
        heldKey_ = 0;
        link_.send(Stop);
    }
    QWidget::focusOutEvent(e);
}

bool Window::confirmQuit()
{
    QMessageBox box(QMessageBox::Question, QStringLiteral("Quitter"),
                    QStringLiteral("Quitter la télécommande %1 ?").arg(RobotName), QMessageBox::NoButton, this);
    QPushButton *yes = box.addButton(QStringLiteral("Quitter"), QMessageBox::AcceptRole);
    box.setDefaultButton(box.addButton(QStringLiteral("Annuler"), QMessageBox::RejectRole));
    box.exec();
    return box.clickedButton() == yes;
}

void Window::closeEvent(QCloseEvent *e)
{
    // Bouton ✕ comme croix de la barre de titre : confirmation avant de quitter
    if (!confirmQuit()) {
        e->ignore();
        return;
    }
    delete pairing_;  // annule un appairage en cours
    pairing_ = nullptr;
    link_.disconnectFromRobot();  // envoie un stop avant de fermer
    QWidget::closeEvent(e);
}
