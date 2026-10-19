# Retrac OGFN

Cheat Fortnite (OGFN) en deux morceaux :

| Binaire | Role |
|---|---|
| `RetracOGFN.exe` | Launcher : attend `FortniteClient-Win64-Shipping.exe` puis injecte la DLL dedans |
| `RetracOGFN.dll` | Cheat interne : hook DXGI Present, overlay ImGui, ESP / Aimbot / Bullet TP |

## Fonctionnalites

**ESP**

- Boite, skeleton (bones), snapline, pseudo, distance, arme tenue, barre de vie
- Filtre equipe / distance max / joueur local
- Couleurs configurables

**Aimbot**

- FOV, smooth, choix du os (tete / poitrine / bassin)
- Mode memoire (ecrit `ControlRotation`) ou mode souris (`mouse_event`)
- Vis-check optionnel, FOV affiche a l'ecran
- Touche configurable (clic droit, alt, shift, souris 4/5, caps...)

**Bullet TP**

- Ecrit la position de spawn des projectiles dans l'arme locale
- Touche dediee + mode "toujours actif"

**Divers**

- Menu ImGui stylise (touche `Inser`)
- Watermark + FPS
- Config sauvegardee dans `%LOCALAPPDATA%\RetracOGFN\config.json`
- Offsets surchargeables sans recompiler via `offsets.json`

## Compilation

Prerequis : Visual Studio 2022 (C++), CMake >= 3.20, git.

    cmake -S . -B build -A x64
    cmake --build build --config Release --parallel

Resultat dans `build/bin/` :

    build/bin/RetracOGFN.exe
    build/bin/RetracOGFN.dll

Les dependances (Dear ImGui, nlohmann/json) sont recuperees automatiquement par CMake
(FetchContent), rien a installer a la main.

## Utilisation

1. Lance Fortnite en DirectX 11 (Parametres > Rendu > Mode de rendu > DirectX 11).
2. Lance `RetracOGFN.exe` : il attend le process et injecte la DLL.
3. Appuie sur `Inser` en jeu pour ouvrir/fermer le menu.

Si le jeu tourne en DirectX 12 le hook DXGI ne matchera pas : reste en DX11.

## Mise a jour des offsets

Fortnite casse ses offsets a chaque patch. Ils sont regroupes dans
`src/cheat/SDK/Offsets.hpp` et surchargeables sans recompiler.

Cree un fichier `offsets.json` a cote de `RetracOGFN.dll` :

    {
      "GWorld": 45300712,
      "World_OwningGameInstance": 568,
      "World_PersistentLevel": 56,
      "Character_Mesh": 752,
      "Pawn_PlayerState": 656,
      "Pawn_CurrentWeapon": 2464,
      "PlayerState_PlayerName": 776
    }

Toutes les cles sont visibles dans `Offsets.hpp` (noms identiques aux champs).
Les valeurs fournies sont celles du patch 42.40.

## Structure

    src/common/    utilitaires partages (logger, memoire, scanner AOB)
    src/injector/  le launcher .exe (CreateRemoteThread + LoadLibraryW)
    src/cheat/     la DLL
      SDK/         maths (FVector/FTransform), offsets, lecture des classes UE
      Features/    ESP, Aimbot, BulletTP
      Render/      hook Present/ResizeBuffers + helpers de dessin
      Menu/        panel ImGui
