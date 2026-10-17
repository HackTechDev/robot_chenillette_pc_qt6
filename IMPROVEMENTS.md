# Améliorations possibles

Pistes classées par priorité. Chaque piste indique ce qu'elle apporte et ce qu'elle touche.

## 1. Sécurité

### Keep-alive (watchdog) côté firmware
**Problème :** le firmware n'a pas de délai d'expiration (`COMMAND_TIMEOUT_MS = 0`). Si la liaison radio tombe sans que le PC ni le module ne le signalent, le stop envoyé au relâchement se perd et le robot continue de rouler.
**Piste :** le firmware s'arrête s'il ne reçoit rien pendant ~300 ms ; l'appli renvoie la commande en cours toutes les ~100 ms tant qu'un bouton ou une touche est maintenu.
**Touche :** firmware (`../robot_chenillette`), appli Android, version Python et version Qt6, à changer ensemble. C'est le garde-fou le plus utile.

### Souris et clavier en même temps
**Problème :** relâcher un bouton à la souris envoie un stop même si une touche du clavier est encore enfoncée, et inversement. Le robot s'arrête alors que l'utilisateur pense encore piloter.
**Piste :** suivre une seule « commande active », quelle qu'en soit la source (souris ou clavier), et n'envoyer le stop que lorsque plus rien n'est maintenu.
**Touche :** `window.cpp` (`dirButton`, `keyPressEvent`, `keyReleaseEvent`).

## 2. Connexion

### Délai de connexion plus court et bouton Annuler
**Problème :** quand le robot est éteint, la connexion met environ 30 s à échouer (délai du noyau), sans moyen d'annuler.
**Piste :** minuteur côté appli (par exemple 10 s) et bouton « Annuler » pendant la connexion.
**Touche :** `Link::connectTo`, `Window::connectTo`.

### Reconnexion automatique
**Problème :** après une liaison perdue, il faut recliquer sur Connecter.
**Piste :** proposer une reconnexion automatique. Elle ne renvoie que la vitesse, jamais une commande de mouvement.
**Touche :** `Window::onLost`.

### Appairage entièrement asynchrone
**Problème :** les premiers appels D-Bus de `Pairing::start()` sont bloquants. L'interface peut se figer un instant, notamment à l'allumage de l'adaptateur (contourné par un `repaint()`).
**Piste :** passer tous les appels en `asyncCall`.
**Touche :** `pairing.cpp`.

### Tester l'appairage complet
**Problème :** la recherche du robot et l'appairage avec PIN n'ont pas encore été testés dans la version Qt6.
**Piste :** lancer `./runPairing.sh --repair`, robot allumé, puis tester l'appairage automatique depuis l'interface.

## 3. Confort d'utilisation

- **Mémoriser les réglages** (`QSettings`) : dernier appareil ou adresse saisie, niveau de vitesse, taille et position de la fenêtre.
- **Manette de jeu :** Qt 6 n'a plus Qt Gamepad. Il faudrait passer par SDL2 (stick → z/s/q/d, gâchettes → pince/tête).
- **Touches configurables**, pour les claviers autres qu'AZERTY.
- **Interface redimensionnable :** les boutons ont une taille fixe de 90 px. Les faire suivre la taille de la fenêtre (utile sur un petit écran ou un écran tactile).
- **État de la liaison plus visible :** voyant de couleur ou barre d'état, et dernière commande envoyée (utile pour le débogage).
- **Traduction :** les textes sont écrits en dur en français. Passer par `tr()` et Qt Linguist permettrait d'autres langues.

## 4. Qualité du code et outillage

- **Tests automatisés (QtTest) :** lecture de l'adresse Bluetooth (`parseAddress`) et logique clavier/boutons de `Window`. Pour tester sans robot, il faudrait pouvoir remplacer `Link` par une fausse liaison qui enregistre les caractères envoyés. Aujourd'hui, les tests sans affichage se font avec un programme écrit à la main.
- **Intégration continue (GitHub Actions) :** compilation sur Ubuntu à chaque push.
- **Avertissements du compilateur :** activer `-Wall -Wextra -Wpedantic` dans `CMakeLists.txt`.
- **Installation :** règle `install` dans CMake, fichier `.desktop` et icône pour lancer l'appli depuis le menu du bureau.
- **Option `--version`** pour `batmobile` et `bt_pairing`.
- **Constantes en double :** les niveaux de vitesse, le PIN et le nom du robot sont recopiés dans 4 projets (firmware, Android, Python, Qt6). Un fichier de référence commun (ou au minimum un commentaire qui renvoie à la source) éviterait les écarts.
