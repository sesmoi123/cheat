# Retrac OGFN — correction du LNK2019 (features::aimbot::tick / bullet_tp::tick)

## Ce que dit vraiment l'erreur

```text path=src/cheat/Features/Aimbot.cpp
?tick@aimbot@features@@YAXXZ     ->  features::aimbot::tick(void)
?tick@bullet_tp@features@@YAXXZ  ->  features::bullet_tp::tick(void)
```

`Render.obj` appelle ces deux fonctions, `ESP.obj` fournit bien la sienne. Le
probleme est donc localise : `Aimbot.cpp` et `BulletTP.cpp` ne definissent pas
leur `tick` dans le bon namespace. Les en-tetes, eux, sont corrects.

Le cas typique : le `.cpp` definit `void tick()` a la portee globale, ou dans
`namespace aimbot` **sans** `features::`. Ca compile, et ca casse au link.

## Verification en une commande

```bat path=start.bat
dumpbin /symbols build\src\cheat\RetracCheat.dir\Release\Aimbot.obj | findstr /i tick
```

| Ce que tu vois | Cause | Correction |
| --- | --- | --- |
| rien | `tick` n'est pas defini dans ce fichier | ecrire la definition |
| `?tick@aimbot@@YAXXZ` | il manque `features` | renommer le namespace |
| `?tick@aimbot@features@@YAXXZ` avec `static` | la fonction est interne | retirer `static` |
| `?tick@aimbot@features@@YAXXZ` sans `static` | fichier absent du build | voir la section Rebuild |

## Correction

### Aimbot.hpp

```cpp path=src/cheat/Features/Aimbot.hpp
#pragma once

namespace features::aimbot {

void tick();

} // namespace features::aimbot
```

### Aimbot.cpp

```cpp path=src/cheat/Features/Aimbot.cpp
#include "Features/Aimbot.hpp"

#include <windows.h>
#include <cmath>

#include "Config.hpp"
#include "Input/Input.hpp"
#include "SDK/SDK.hpp"

namespace features::aimbot {

void tick() {
    // ton corps existant, colle ici tel quel
}

} // namespace features::aimbot
```

### BulletTP.hpp

```cpp path=src/cheat/Features/BulletTP.hpp
#pragma once

namespace features::bullet_tp {

void tick();

} // namespace features::bullet_tp
```

### BulletTP.cpp

```cpp path=src/cheat/Features/BulletTP.cpp
#include "Features/BulletTP.hpp"

#include <windows.h>

#include "Config.hpp"
#include "Input/Input.hpp"
#include "SDK/SDK.hpp"

namespace features::bullet_tp {

void tick() {
    // ton corps existant, colle ici tel quel
}

} // namespace features::bullet_tp
```

## Avant / apres, sur le namespace

### Aimbot.cpp

Avant :

```cpp path=src/cheat/Features/Aimbot.cpp
void tick() {
    // corps
}
```

Apres :

```cpp path=src/cheat/Features/Aimbot.cpp
namespace features::aimbot {

void tick() {
    // corps
}

} // namespace features::aimbot
```

### BulletTP.cpp

Avant :

```cpp path=src/cheat/Features/BulletTP.cpp
void tick() {
    // corps
}
```

Apres :

```cpp path=src/cheat/Features/BulletTP.cpp
namespace features::bullet_tp {

void tick() {
    // corps
}

} // namespace features::bullet_tp
```

## Regles a respecter

- **Pas de `static`** devant `tick` : liaison interne, invisible pour le linker.
- **Pas de `inline`** dans le `.cpp` : une definition `inline` non utilisee
  localement ne genere pas forcement le symbole externe.
- Le namespace doit etre **exactement** `features::aimbot` / `features::bullet_tp`.
  `namespace features { namespace aimbot {` est equivalent, `namespace aimbot {`
  ne l'est pas.

## Si le symbole est bien defini mais que ca echoue quand meme

1. Verifie que `src/cheat/CMakeLists.txt` contient bien ces deux lignes :

```cmake path=src/cheat/CMakeLists.txt
Features/Aimbot.hpp Features/Aimbot.cpp
Features/BulletTP.hpp Features/BulletTP.cpp
```

2. Un cache CMake obsolete ne reprend pas les sources ajoutees. Reconfigure proprement :

```bat path=start.bat
start.bat clean
start.bat build
```

Un `cmake --build` seul sur un dossier `build/` deja configure ne suffit pas
toujours a rafraichir la liste des sources.
