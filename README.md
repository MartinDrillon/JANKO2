# Clavicore 0.1

Le Clavicore est un contrôleur midi basé sur la disposition Janko. Le jeu est dynamique. 
Il fonctionne grâce à des interrupteurs MX magnétiques standards et un Teensy 4.1.  


---

## JANKO LAYOUT

Contrairement à un piano standard, l’écart physique entre les touches correspond à l’écart musical entre les notes. Il y a un ton de différence sur les lignes et un demi-ton sur les colonnes. C’est tout. Toutes les gammes partagent le même doigté pour des accords identiques et sont jouées de la même manière.  

https://en.wikipedia.org/wiki/Jank%C3%B3_keyboard



## FONCTIONNEMENT

Un capteur hall génère le voltage relatif à la position de l’aimant dans la switch. Le microcontrôleur identifie le voltage correspondant à la position haute et basse de chaque touche. On mesure ensuite le temps passé pour passer de l’état "touche au repos" à l’état "touche pressée" pour connaitre la vitesse de frappe, et donc attribuer une vélocité au signal midi.

La difficulté est de suivre simultanément et avec précision la position des 120 touches. Avec quelques multiplexeurs, buffers et autres, j’arrive à environ 13 000 échantillons de position par seconde et par touche. Au delà, les ADC du Teensy ne suivent plus. 

Le programme a été vibe codé. Je crois que c’est moche, mais ça fonctionne. Des ajustements doivent encore être faits au niveau de la calibration des touchens, de la courbe de vélocité, etc.


## DÉMONSTRATION

Lien que je vais ajouter

## LA SUITE

* Faire un vrais boitier
* Utiliser des keycaps profilées pour faciliter le jeu avec le pouce : https://github.com/heyitscassio/PseudoMakeMeKeyCapProfiles
* Ajouter une cinquième lignes de touche ? 
* Intégrer quelques instruments directement dans le Teensy, si possible ?
* Remplacer le Teensy par un STM32 !  


---

### CREDIT

Pour la preuve qu’il est possible de calculer efficacement la dynamique de jeu avec des capteurs halls :  
https://github.com/aleathwick/midi_hammer

Pour la preuve qu’on peut jouer avec des touches MX classiques :  
https://github.com/KOOPInstruments/melodicade_mx