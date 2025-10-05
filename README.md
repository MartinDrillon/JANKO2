# Clavicore 0.1

Le Clavicore est un contrôleur midi basé sur la disposition Janko.  
Il fonctionne grâce à des interrupteurs MX magnétiques standards et un Teensy 4.1.  
Le jeu est dynamique.

---

## JANKO LAYOUT

Contrairement à un piano standard, l’écart physique entre les touches correspond à l’écart musical entre les notes.  
Il y a un ton de différence sur les lignes et un demi-ton sur les colonnes. C’est tout. Toutes les gammes partagent le même doigté pour des accords identiques et sont jouées de la même manière.  

https://en.wikipedia.org/wiki/Jank%C3%B3_keyboard

---

## FONCTIONNEMENT

Le clavier est basé sur des switchs magnétiques.  
Un capteur hall génère un voltage relatif à la position de la touche.  
Le microcontrôleur identifie le voltage correspondant à sa position haute et basse.  
Il suffit ensuite de mesurer le temps passé entre l’état "touche au repos" et l’état "touche pressée" pour attribuer une vélocité au signal midi, donc pour avoir un jeu dynamique.

La difficulté est de suivre simultanément et avec précision la position des 120 touches.  
Avec quelques multiplexeurs, buffers et autres, j’arrive à environ 13 000 échantillons de position par seconde et par touche.  
Au delà, les ADC du Teensy ne suivent plus.

Le programme a été vibe codé.  
Je crois que c’est moche, mais ça fonctionne.  
Des ajustements doivent encore être faits au niveau de la calibration des touches et de la courbe de vélocité.

---

## DÉMONSTRATION

Lien que je vais ajouter

---

## CREDIT

Pour la preuve qu’il est possible de calculer efficacement la dynamique de jeu avec des capteurs halls :  
https://github.com/aleathwick/midi_hammer

Pour la preuve qu’on peut jouer avec des touches MX classiques :  
https://github.com/KOOPInstruments/melodicade_mx