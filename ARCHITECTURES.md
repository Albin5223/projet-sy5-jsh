# Projet : programmation d'un interpréteur de commandes : Architecture


## Parse d'une commande 

Pour parser une commande Exemple : `cmd0 | cmd1 > toto & cmd2 <( cmd3 | cmd4 ) | cmd5`

Dans un premier temps, on crée un tableau avec les différents mots qui composent la commande (On suppose que chaque mot est séparé d'un espace).

Ensuite on sépare la commande en fonction de `&` : 
en suivant 