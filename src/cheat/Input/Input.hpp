#pragma once

#include <cstdint>

namespace input {

// Vrai si la touche clavier OU le bouton manette est enfonce.
bool down(int virtualKey, uint32_t padMask);

// Vrai sur le front montant uniquement.
bool pressed(int virtualKey, uint32_t padMask);

} // namespace input
