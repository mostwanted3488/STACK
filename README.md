# STACK

Plugin VST3 hote : 20 slots dans un seul slot de mixer FL Studio.

- Charge des VST3 en serie (les plugins natifs FL ne sont pas supportes)
- Bypass par slot, ouverture de la fenetre de chaque plugin
- Latence cumulee reportee au host, dry/wet global compense
- Sauvegarde de l'etat de tous les plugins dans le projet

## Build

Le workflow GitHub Actions compile le VST3 Windows a chaque push sur `main`.
Telecharge l'artefact `STACK-windows-vst3` dans l'onglet Actions, puis copie
`STACK.vst3` dans `C:\Program Files\Common Files\VST3`.

Build local : `cmake -B build -A x64 && cmake --build build --config Release`
