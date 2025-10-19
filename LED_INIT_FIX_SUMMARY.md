# Résumé de la correction du problème d'initialisation des LEDs

## 🔴 Problèmes observés

1. **Au flash du firmware** : La LED 4 flashe très brièvement puis s'éteint
2. **Au démarrage à froid** : Aucune LED ne s'allume
3. **Workaround** : Il faut changer l'état du rocker pour que les LEDs se mettent à jour

## 🔍 Causes identifiées

### Cause 1 : Variables d'état identiques au démarrage
Les variables `g_cached*` et `g_last*` étaient initialisées avec les mêmes valeurs, empêchant la détection de changement.

### Cause 2 : Hardware instable au démarrage
- Pull-ups internes non stabilisés
- Rate-limiter de `IoState::update()` (polling 100Hz) 
- Rebonds mécaniques du rocker

## ✅ Solution triple implémentée

### 1. Initialisation différenciée des variables (dans `simple_leds.cpp`)
```cpp
// Variables actuelles
g_cachedRockerState = 0;
g_cachedColor = 0;
g_button24Low_cached = false;

// Variables "précédentes" avec valeurs INVALIDES
g_lastRockerState = 0xFF;        // Force la détection
g_lastPushedColor = 0xFFFFFFFF;  // Force la détection
g_button24Low_last = true;       // Force la détection
g_needFlush = true;              // Force le flush
```

### 2. Délai de stabilisation (dans `main.cpp`)
```cpp
simpleLedsInit();
IoState::init();

// ⏱️ Attente de 500ms pour stabilisation complète
delay(500);

// Lecture de l'état stable et mise à jour
IoState::RockerStatus initialState;
IoState::update(millis(), initialState);
simpleLedsSetRocker(initialState.pin4High, initialState.pin5High);
simpleLedsSetButton24(initialState.button24Low);
simpleLedsFrameFlush();
```

### 3. Ordre d'initialisation correct
LEDs → IoState → **delay(500)** → lecture + affichage

## 🎯 Résultat attendu

✅ Les LEDs s'allument **immédiatement et correctement** au démarrage
✅ Toutes les positions du rocker sont affichées (gauche, centre, droite)
✅ Fonctionne au flash ET à la mise sous tension à froid
✅ **Aucun impact sur les performances runtime** (le délai est uniquement dans `setup()`)

## 📝 Fichiers modifiés

- `src/main.cpp` : Ajout du `delay(500)` dans `setup()`
- `src/simple_leds.cpp` : Initialisation différenciée des variables `g_last*`
- Documentation : `LED_INIT_FIX.md`

## ⚡ Performance

Le délai de 500ms est présent **uniquement au démarrage** (fonction `setup()`).
La boucle principale (`loop()`) et les lectures ADC ne sont **pas affectées**.

## 🧪 Test

```bash
pio run -t upload
```

1. Mettre le rocker en position centrale
2. Débrancher et rebrancher le Teensy
3. ✅ La LED centrale (LED 3) doit s'allumer immédiatement

## 📅 Date

19 octobre 2025 - Solution finale avec délai de stabilisation
