// A ajouter dans namespace injector { ... }

// Vrai des qu'une fenetre visible du processus existe (jeu pret a injecter).
// Template : accepte aussi bien un PID (entier) qu'un HANDLE, sans cast cote appelant.
bool game_window_ready_impl(uintptr_t processOrHandle);

template <typename T = uintptr_t>
bool game_window_ready(T processOrHandle = 0) {
    return game_window_ready_impl(static_cast<uintptr_t>(processOrHandle));
}
