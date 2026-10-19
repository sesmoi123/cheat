## Utilisation

1. Fortnite en **DirectX 11** (Parametres > Rendu > Mode de rendu).
2. Attends d'etre **dans le lobby**.
3. Double-clic sur `RetracOGFN.exe` : UAC, puis injection.
4. `Inser` ou **LB + RB** ouvre / ferme le menu.

## Manette

Le cheat supporte nativement la manette, en parallele du clavier. Aucun
outil externe n'est necessaire.

- Navigation du menu : D-pad ou stick gauche, validation **A**, retour **B**.
- Aimbot : maintien de **LT**.
- Bullet TP : **Y**.
- Ouvrir / fermer le menu : **LB + RB**.

Tous les boutons sont rebindables dans l'onglet **Manette**. Le jeu lit la
manette en meme temps que le cheat : choisis de preference un combo rarement
utilise en jeu pour le menu.

| Symptome | Cause | Solution |
| --- | --- | --- |
| menu ne s'ouvre pas a la manette | manette non detectee | branche la manette avant de lancer Fortnite, verifie `Manette : connectee` dans l'onglet Manette |
| `LoadLibraryW a renvoye NULL` | dependance manquante dans le dossier du jeu | reinstalle le Redistributable MSVC 2015-2022 x64, ou copie `vcruntime140.dll` et `msvcp140.dll` dans `%LOCALAPPDATA%\RetracOGFN` |
| rien a l'ecran en jeu | rendu DirectX 12 | passe en DirectX 11 |
| ESP decale / vide | offsets perimes | corrige `%LOCALAPPDATA%\RetracOGFN\offsets.json` |
