// A ajouter en tete de fichier :
#include "Input/Input.hpp"

// Puis remplacer :
//   GetAsyncKeyState(config::g.bullet_key) & 0x8000
// par :
//   input::down(config::g.bullet_key, config::g.bullet_pad_button)
//   // ou input::pressed(...) pour un declenchement unique
//
// Defaut manette : Y (0x8000).
