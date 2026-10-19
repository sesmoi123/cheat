# Retrac OGFN

Cheat Fortnite (OGFN) livre en **un seul fichier `RetracOGFN.exe`**.

- Le launcher attend `FortniteClient-Win64-Shipping.exe`, puis injecte la DLL du cheat dedans.
- La DLL (hook DXGI Present, overlay ImGui, ESP / Aimbot / Bullet TP) est **embarquee dans l'exe**
  et extraite automatiquement dans `%LOCALAPPDATA%\RetracOGFN` au lancement.
- Aucune dependance : pas de Visual C++ Redistributable (runtime MSVC statique),
  pas de fichier annexe a cote de l'exe.

## Recuperer le .exe sans rien installer

Trois facons, aucune ne demande d'outil sur le PC :

1. **Releases** : onglet Releases du depot, telecharge `RetracOGFN.exe`.
2. **Actions** : onglet Actions, dernier run vert, artefact `RetracOGFN`.
3. **Script Node** (zero dependance npm) :

       node tools/deploy.mjs --token=ghp_xxxxxxxx

   Le script cree le depot si besoin, pousse les sources, declenche le build cloud,
   attend la fin et depose `dist/RetracOGFN.exe`.

## Utilisation

1. Lance Fortnite en **DirectX 11** (Parametres > Rendu > Mode de rendu > DirectX 11).
2. Attends d'etre **dans le lobby**.
3. Double-clic sur `RetracOGFN.exe` : Windows demande l'UAC (droits admin, obligatoire
   car EasyAntiCheat protege le process Fortnite), puis l'injection se fait.
4. En jeu, touche `Inser` pour ouvrir / fermer le menu.

L'exe demande lui-meme l'elevation via son manifest, et se relance en administrateur
si besoin. En cas d'echec d'injection, il retente l'ouverture du handle pendant 15 s.

## Fonctionnalites

**ESP**

- Boite, skeleton (bones), snapline, pseudo, distance, barre de vie, etiquette DBNO
- Filtre equipe / distance max / joueur local
- Couleurs configurables

**Aimbot**

- FOV, smooth, choix du os (tete / poitrine / bassin)
- Mode memoire (ecrit `ControlRotation`) ou mode souris (`mouse_event`)
- Vis-check optionnel, cercle de FOV affiche a l'ecran
- Touche configurable

**Bullet TP**

- Ecrit la position de spawn des projectiles dans l'arme locale
- Touche dediee + mode "toujours actif"

**Divers**

- Menu ImGui stylise (touche `Inser`), watermark
- Config sauvegardee dans `%LOCALAPPDATA%\RetracOGFN\config.json`
- Offsets surchargeables sans recompiler

## Mise a jour des offsets

Les offsets vivent dans `src/cheat/SDK/Offsets.hpp` et sont surchargeables par un
`offsets.json` place a cote de la DLL, c'est-a-dire dans `%LOCALAPPDATA%\RetracOGFN` :

    {
      "GWorld": 45300712,
      "World_OwningGameInstance": 568,
      "Character_Mesh": 752,
      "Pawn_PlayerState": 656,
      "Pawn_CurrentWeapon": 2464,
      "PlayerState_PlayerName": 776
    }

Toutes les cles sont dans `Offsets.hpp` (memes noms que les champs).
Les valeurs par defaut correspondent au patch 42.40.

Un `offsets.json` d'exemple est ecrit automatiquement au premier lancement.

## Compiler en local (optionnel)

Prerequis : Visual Studio 2022 (C++), CMake >= 3.20.

    cmake -S . -B build -A x64
    cmake --build build --config Release --parallel

Le binaire sort dans `build/bin/RetracOGFN.exe`.

## Structure

    tools/deploy.mjs   script Node de deploiement + recuperation du .exe
    src/common/        utilitaires partages (logger, memoire, scanner AOB)
    src/injector/      le launcher .exe (ressource DLL + injection LoadLibraryW)
    src/cheat/         la DLL
      SDK/             maths (FVector/FTransform), offsets, lecture des classes UE
      Features/        ESP, Aimbot, BulletTP
      Render/          hook Present/ResizeBuffers + helpers de dessin
      Menu/            panel ImGui

## Depannage

| Symptome | Cause | Solution |
|---|---|---|
| `OpenProcess a echoue (code 5)` | process non eleve | accepter l'UAC, ou clic droit > Executer en tant qu'administrateur |
| `le process a disparu (code 87)` | jeu ferme pendant l'injection | relancer Fortnite puis l'exe |
| rien ne s'affiche en jeu | rendu DirectX 12 | passer en DirectX 11 dans les options |
| pas de menu | injection trop tot | injecter une fois dans le lobby |

## Notes

- Reste en DirectX 11 : le hook DXGI ne matche pas le rendu DirectX 12.
- La DLL extraite peut etre verrouillee si le jeu tourne encore : le launcher
  bascule alors sur un nom de fichier horodate, rien a faire de special.
