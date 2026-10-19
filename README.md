# Retrac OGFN

Cheat Fortnite (OGFN) livre en **un seul fichier `RetracOGFN.exe`**.

- Le launcher attend Fortnite, s'eleve en administrateur (UAC), puis injecte la DLL du cheat.
- La DLL (hook DXGI Present, overlay ImGui, ESP / Aimbot / Bullet TP) est **embarquee dans l'exe**
  et extraite dans `%LOCALAPPDATA%\RetracOGFN`.
- Aucune dependance utilisateur : runtime MSVC statique, pas de Redistributable.

## Compiler

    start.bat            build Release puis lance
    start.bat build      build seulement (build\bin\RetracOGFN.exe)
    start.bat clean      repart de zero

Prerequis : Visual Studio 2022 (C++), CMake, git (pour ImGui au premier configure).

En manuel :

    cmake -S . -B build -A x64
    cmake --build build --config Release --parallel

## Utilisation

1. Fortnite en **DirectX 11** (Parametres > Rendu > Mode de rendu).
2. Attends d'etre **dans le lobby**.
3. Double-clic sur `RetracOGFN.exe` : UAC, puis injection.
4. `Inser` ouvre / ferme le menu.

## Mise a jour des offsets

Les offsets sont dans `src/cheat/SDK/Offsets.hpp` et surchargeables sans recompiler par un
`offsets.json` a cote de la DLL, c'est-a-dire dans `%LOCALAPPDATA%\RetracOGFN` :

    {
      "GWorld": 45300712,
      "World_OwningGameInstance": 568,
      "Pawn_PlayerState": 656,
      "Pawn_CurrentWeapon": 2464,
      "Pawn_Mesh": 752,
      "PlayerState_PlayerName": 776
    }

Toutes les cles portent le nom des champs de `Offsets.hpp`. Un exemple est ecrit au premier
lancement.

## Structure

    start.bat              build + lancement
    tools/deploy.mjs       push GitHub + build cloud + recuperation du .exe
    src/common/            logger, memoire, scanner AOB, JSON, chemins
    src/injector/          le launcher .exe (elevation, injection, ressource DLL)
    src/cheat/             la DLL
      SDK/                 maths, offsets, lecture des classes UE
      Render/              hook Present / ResizeBuffers, helpers de dessin
      Features/            ESP, Aimbot, BulletTP
      Menu/                panel ImGui

## Depannage

| Symptome | Cause | Solution |
|---|---|---|
| `OpenProcess a echoue (code 5)` | process non eleve | accepte l'UAC, ou clic droit > Executer en tant qu'administrateur |
| `code 5` malgre l'admin | overlay qui tient le handle | ferme Overwolf / Afterburner / RivaTuner, injecte dans le lobby |
| `le process a disparu (code 87)` | jeu ferme pendant l'injection | relance Fortnite puis l'exe |
| rien a l'ecran en jeu | rendu DirectX 12 | passe en DirectX 11 |
| ESP decale / vide | offsets perimes | corrige `%LOCALAPPDATA%\RetracOGFN\offsets.json` |

## Notes

- DirectX 11 obligatoire : le hook DXGI ne matche pas le rendu DirectX 12.
- Les valeurs d'offsets fournies sont un point de depart : apres un gros patch, ajuste-les.
- Les indices de bones du squelette sont approximatifs et documentes dans `Features/ESP.cpp`.
