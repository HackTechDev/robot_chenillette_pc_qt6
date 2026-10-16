# BatMobile — Télécommande PC (C++ / Qt6)

Application de bureau (C++ / Qt6) pour piloter le robot Zumo Shield **BatMobile** (`../robot_chenillette`) en Bluetooth classique (SPP/RFCOMM). Portage de la version Python ([robot_chenillette_pc](https://github.com/HackTechDev/robot_chenillette_pc)) : même interface, même protocole que l'appli Android (`../robot_chenillette_telecommande`).

## Prérequis

- Linux avec BlueZ (service `bluetooth` actif) et un adaptateur Bluetooth
- Qt ≥ 6.4 (modules Widgets et DBus), CMake ≥ 3.19, un compilateur C++17 :
  `sudo apt install qt6-base-dev cmake g++`

Ni Qt Bluetooth ni `libbluetooth-dev` ne sont nécessaires : la liaison RFCOMM utilise directement les sockets Bluetooth du noyau, et l'appairage passe par BlueZ via D-Bus (QtDBus).

## Compiler

```bash
./build.sh        # → build/batmobile (télécommande) et build/bt_pairing (appairage)
```

## Appairer le robot

**Automatique** : rien à faire. Si BatMobile n'est pas encore appairé, la liste affiche « BatMobile — appairage automatique » et le bouton **Connecter** lance l'appairage (dans l'appli), puis se connecte. Robot allumé (le module est visible ~4 s après le démarrage).

L'appairage peut aussi être lancé seul :

```bash
./runPairing.sh              # allume l'adaptateur, cherche « BatMobile », appaire (PIN 0000), marque de confiance
./runPairing.sh --repair     # supprime l'appairage existant et recommence (après un changement de module, PIN refusé…)
./runPairing.sh --timeout 60
```

Il ne fait rien si le robot est déjà appairé (à part le marquer de confiance). Il répond lui-même au PIN demandé par le module (appairage « legacy »), sans `sudo`.

**Manuel** (si besoin) :

```bash
bluetoothctl
[bluetooth]# scan on          # attendre « BatMobile » (00:18:A1:12:16:C5)
[bluetooth]# pair 00:18:A1:12:16:C5     # code PIN : 0000
[bluetooth]# trust 00:18:A1:12:16:C5
[bluetooth]# scan off
[bluetooth]# quit
```

Ne pas faire `connect` dans `bluetoothctl` : c'est l'application qui ouvre la liaison série.

## Lancer

```bash
./launchRemoteControl.sh     # compile au premier lancement ; ou : build/batmobile
```

1. **BatMobile** est présélectionné : appairé, ou « appairage automatique » s'il ne l'est pas encore (⟳ pour rafraîchir la liste). On peut aussi choisir un autre appareil appairé ou saisir une adresse.
2. Cliquer **Connecter** : appairage si nécessaire (la progression s'affiche sous le bouton), puis connexion. La vitesse du slider est envoyée au robot dès la connexion.
3. Pour quitter : bouton **✕** en bas à droite (ou croix de la barre de titre). Une fenêtre demande confirmation : **Quitter** ou **Annuler** (choix par défaut, donc Entrée ou Échap ne ferment pas l'appli par erreur).

## Commandes

Boutons à la souris (maintenir) ou au clavier :

| Bouton | Clavier | Caractère | Action |
|--------|---------|-----------|--------|
| ▲ | `Z` / ↑ | `z` | Avance (maintenir) |
| ▼ | `S` / ↓ | `s` | Recule (maintenir) |
| ↺ | `Q` / ← | `q` | Pivote à gauche (maintenir) |
| ↻ | `D` / → | `d` | Pivote à droite (maintenir) |
| 🤏 | `V` | `v` | Ferme la pince (maintenir) |
| ✋ | `C` | `c` | Ouvre la pince (maintenir) |
| Tête ▲ | `W` | `w` | Tête (maintenir) |
| Tête ▼ | `X` | `x` | Tête (maintenir) |
| ■ | `Espace` | ` ` | Stop |
| slider Vit. | `0` … `9` | `0` … `9` | Vitesse `(n + 1) × 40` — ne stoppe pas le robot |

Le caractère est envoyé à l'appui, un espace (stop) au relâchement, sans répétition pendant l'appui (le firmware n'a pas de keep-alive). Au clavier, les touches sont reconnues par leur lettre (Z/Q/S/D forment une croix sur un clavier AZERTY ; en QWERTY, utiliser les flèches).

## Sécurité

Par rapport à l'appli Android, quelques garde-fous supplémentaires :

- une erreur d'écriture ou une coupure de liaison est détectée : les boutons se désactivent et un message s'affiche ;
- **Déconnecter** et la fermeture de l'appli (après confirmation, via ✕ ou la barre de titre) envoient un stop avant de couper ;
- si la fenêtre perd le focus pendant qu'une touche est enfoncée (le relâchement ne serait pas reçu), un stop est envoyé ;
- au clavier, relâcher une ancienne touche alors qu'une autre est enfoncée n'envoie pas de stop.

Reste vrai : si la liaison radio est perdue sans que le PC ni le module ne le signalent, un stop peut se perdre.

## Dépannage

- **`Connexion échouée : [Errno 111] Connexion refusée` / `[Errno 112] L'hôte cible est arrêté ou en panne`** : robot éteint, hors de portée, ou pas encore appairé ; vérifier aussi qu'un téléphone n'est pas déjà connecté au module (une seule connexion à la fois).
- **`[Errno 13] Permission non accordée`** : vérifier que le service Bluetooth tourne (`systemctl status bluetooth`) et que l'adaptateur est allumé (`bluetoothctl power on`).
- **`Appairage échoué : BatMobile introuvable`** : robot éteint ou hors de portée, ou module déjà connecté à un téléphone (il n'est alors plus visible).
- **`PIN refusé`** : `./runPairing.sh --repair`.
