# OpenSCAD 2026.10.29-CS1

Version personnelle de [OpenSCAD](https://github.com/openscad/openscad), basée sur
le commit amont `ee450bb8d`.

## Zoom au trackpad

Dans l’aperçu 3D, écarter deux doigts agrandit le modèle et les rapprocher le
réduit. Le zoom est progressif, y compris pour de très petits mouvements.

Lorsque le centrage sur la souris est activé dans les préférences, le pointeur
sert de centre du zoom. Les coordonnées du geste sont converties de l’écran
vers l’aperçu pour tenir compte de la position de l’éditeur et des panneaux.
Le réglage de centrage existant reste applicable à la molette et au pincement.

Cette version conserve les commandes de zoom existantes et fonctionne dans les
vues en perspective et orthogonale. `VERSION.txt` fixe le nom de version ; le
suffixe `CS1` apparaît aussi dans l’écran de démarrage et la boîte « À propos »,
y compris lors d’une compilation sans métadonnées Git.

## Vérifications

Compilation et essais effectués sur macOS 27.0.1, Apple Silicon, avec Qt 6.12.0 :

- 56 tests unitaires réussis, soit 75 145 assertions.
- 24 tests de caméra réussis.
- Suite complète d’interface réussie, dont 15 cas consacrés au zoom : sens du
  pincement, petits incréments, molette, séquences de gestes et stabilité du
  point visé en perspective et en vue orthogonale.
- Les six cas de centrage reproduisent le défaut avant correction et passent
  après correction.
- Gestes et centrage confirmés par un essai réel au trackpad.

Les autres plateformes et Qt 5 n’ont pas été testés pour cette version.

## Recompiler sur un Mac Apple Silicon

Installer les dépendances décrites dans le README principal, puis récupérer le
dépôt et ses sous-modules :

```sh
git clone --recurse-submodules https://github.com/CSab80/openscad.git
cd openscad
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/opt/homebrew \
  -DUSE_BUILTIN_OPENCSG=ON \
  -DENABLE_GUI_TESTS=ON \
  -DBISON_EXECUTABLE=/opt/homebrew/opt/bison/bin/bison \
  -DFLEX_EXECUTABLE=/opt/homebrew/opt/flex/bin/flex
cmake --build build -j 6
build/OpenSCAD.app/Contents/MacOS/OpenSCAD --version
```

L’application est produite dans `build/OpenSCAD.app`. Cette compilation locale
utilise les bibliothèques Homebrew du Mac.

Pour les vérifications automatisées :

```sh
build/OpenSCADUnitTests
ctest --test-dir build --output-on-failure -j 4 -R '^openscad-(imgsize|imgstretch|cam)'
cd build
./OpenSCAD.app/Contents/MacOS/OpenSCAD --run-all-gui-tests
```
