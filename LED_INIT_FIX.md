# Fix : LEDs s'allument immédiatement au démarrage

## Problème résolu

Avant ce correctif, les LEDs ne s'allumaient pas immédiatement après :
- Mise sous tension du clavier
- Flash du firmware
- Reset/reboot du Teensy

Il fallait effectuer une action (changer le rocker, presser un bouton) pour que les LEDs s'allument la première fois.

## Cause du problème

Dans `simpleLedsFrameFlush()`, la fonction vérifie si l'état a changé avant de mettre à jour les LEDs :

```cpp
if (!g_needFlush && 
    g_cachedColor == g_lastPushedColor && 
    g_cachedRockerState == g_lastRockerState && 
    g_button24Low_cached == g_button24Low_last) {
    return; // Pas de changement, rien à faire
}
```

À l'initialisation, tous les états "cached" et "last" étaient initialisés à la même valeur (0 ou false), donc aucun changement n'était détecté et les LEDs restaient éteintes.

## Solution implémentée

### 1. Forcer la mise à jour initiale dans `simple_leds.cpp`

Initialiser les états "last" avec des valeurs **différentes** des états "cached" pour forcer le premier flush :

```cpp
// Initialize state variables - set "last" states to invalid values
g_cachedColor = 0;
g_lastPushedColor = 0xFFFFFFFF; // Force different from cached
g_cachedRockerState = 0;
g_lastRockerState = 0xFF;        // Force different from cached
g_button24Low_cached = false;
g_button24Low_last = true;        // Force different from cached
g_needFlush = true;               // Mark flush needed
```

Ainsi, quand `simpleLedsFrameFlush()` est appelé la première fois, il détecte un "changement" et met à jour les LEDs.

### 2. Appliquer l'état initial du rocker dans `main.cpp`

Après l'initialisation de `IoState`, lire immédiatement l'état et l'appliquer aux LEDs :

```cpp
// Apply initial rocker state to LEDs immediately after init
{
    IoState::RockerStatus initialState;
    IoState::update(millis(), initialState); // Read initial state
    simpleLedsSetRocker(initialState.pin4High, initialState.pin5High);
    simpleLedsSetButton24(initialState.button24Low);
    simpleLedsFrameFlush(); // Apply changes immediately
}
```

## Impact sur les performances

✅ **AUCUN impact sur les performances en runtime** :
- Ces changements n'affectent **que** l'initialisation (une seule fois au démarrage)
- Aucun code supplémentaire dans la boucle principale (`loop()`)
- Le scan ADC et le traitement MIDI restent absolument identiques
- Après l'initialisation, le comportement est exactement le même qu'avant

## Résultat

Maintenant, dès la mise sous tension ou après un flash :
1. ✅ La LED verte unique (pin 12) s'allume immédiatement
2. ✅ Le strip de 5 LEDs (pin 10) affiche l'état correct :
   - LED 0 : Bouton 24 (blanc si pressé)
   - LED 1 : Pin 3 (bleu si HIGH)
   - LEDs 2-4 : Indicateurs rocker (mauve selon position)
3. ✅ La luminosité configurée (`kLedBrightness`) est appliquée correctement
4. ✅ Toutes les LEDs répondent instantanément sans nécessiter d'action préalable

## Fichiers modifiés

- `src/simple_leds.cpp` : Initialisation des états "last" avec valeurs forcées différentes
- `src/main.cpp` : Application de l'état initial du rocker après `IoState::init()`

## Date de modification

19 octobre 2025
