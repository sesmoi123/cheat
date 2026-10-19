// A ajouter en tete de fichier :
#include "Input/Input.hpp"

// Puis remplacer chaque test de touche :
//
//   if (GetAsyncKeyState(config::g.aim_key) & 0x8000)
// par
//   if (input::down(config::g.aim_key, config::g.aim_pad_button))
//
// et pour le mode "front montant" :
//   if (input::pressed(config::g.aim_key, config::g.aim_pad_button))
//
// Le joueur manette vise donc en tenant LT (0x10000), sans configuration.
