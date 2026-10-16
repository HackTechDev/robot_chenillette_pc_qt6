#!/usr/bin/env bash
# Lance la télécommande PC du robot BatMobile (compile au besoin).
set -euo pipefail

# Dossier du script, pour pouvoir le lancer depuis n'importe où
cd "$(dirname "$(readlink -f "$0")")"

[ -x build/batmobile ] || ./build.sh
exec build/batmobile "$@"
