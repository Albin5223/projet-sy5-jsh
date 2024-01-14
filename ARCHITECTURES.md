# Projet : programmation d'un interpréteur de commandes : Architecture


## Parse d'une commande 

Pour parser une commande 
 - Exemple : `cmd1 & cmd2 & cmd3`

Dans un premier temps, on crée un tableau avec les différents mots qui composent la commande (On suppose que chaque mot est séparé d'un espace).

Ensuite on sépare la ligne de commande en fonction de `&` et on remplit un nouveau tableau de commandes contenant cette structure :

```C
typedef struct {
    char **cmd;
    bool is_background;
} Command;
```
L'attribut `is_background` est défini en fonction de si le contenu se trouve avant un `&` alors background est à `true` sinon `false`.

On obtient alors un tableau de `Command` qui contient les différentes commandes à exécuter, et si elles doivent être exécutées en arrière plan ou non.

Ensuite on parcourt ce tableau et on sépare chaque commande en fonction de `|`

 - Exemple : on considère que `cmd0 | cmd1 > toto` contient un pipe 
 - Exemple : on considère que `cmd0 <( cmd1 | cmd2 )` ne contient pas de pipe, car le pipe est dans un subsitution de processus

On obtient alors un tableau où chaque élément est de la forme :
- Une commande interne
- Une commande externe
- Une commande avec des redirections
- Une commande avec une substituition de processus

## Exécution d'une commande

### Commande interne

Dans le module `internalCommand.c`, on retrouve les différentes implémentations des commandes internes.

### Commande externe

A l'aide de la fonction `execvp` on exécute la commande externe.

### Commande avec des redirections

- `cmd > toto` : création du fichier toto si il n'existe pas et écriture de la sortie standard dans le fichier toto
- `cmd >> toto` : création du fichier toto si il n'existe pas et écriture de la sortie standard à la fin du fichier toto
- `cmd < toto` : lecture du fichier toto et écriture dans l'entrée standard
- `cmd >| toto` : création du fichier toto si il n'existe pas et si il existe alors on l'écrase 
- `cmd 2> toto` : création du fichier toto si il n'existe pas et écriture de la sortie d'erreur dans le fichier toto
- `cmd 2>> toto` : création du fichier toto si il n'existe pas et écriture de la sortie d'erreur à la fin du fichier toto
- `cmd 2>| toto` : création du fichier toto si il n'existe pas et si il existe alors on l'écrase

Précisons que `cmd` peut être une commande interne ou externe.

### Commande avec une substituition de processus

Todo : expliquer la substituition de processus


## Exécution d'une commande pipe 

Pour excuter une commmande pipe, on compte le nombre de `|` dans la commande en faisant attention a ce qu'elle ne se situe pas dans une substitution de processus.

Puis ensuite on crée un tableau de pipe pour stocker les différents tubes nécessaires à l'exécution de la commande.

On parcours le tableau en faisant en sorte que la commande au rang `i` lise dans le tube `i-1` et écrive dans le tube `i`.
Bien sur si `i = 0` alors on ne lit pas dans un tube et si `i = nombre de pipe + 1` alors on n'écrit pas dans un tube.

Pour chaque commande, on peut faire une redirection de la sortie erreur en suivant le même principe que pour les redirections classiques.

La commande peut aussi être une commande interne, externe, ou être une substitution de processus.

Exemple : `cmd1 | cmd2 | cmd3`

On va avoir 2 tubes :
- `cmd1` : écrit dans le `tube 0`
- `cmd2` : lit dans le `tube 0` et écrit dans le `tube 1`
- `cmd3` : lit dans le `tube 1`

## Exécution d'une commande en arrière plan


Pour excécuter une commande en arrière plan, on crée un processus fils qui va exécuter la commande et le processus père *(le shell)*, va simplement ajouter son fils au tableau de jobs et va continuer son travail.

Exemple : `sleep 10 &`

On a à notre disposition un tableau de jobs, où se trouve tous les jobs en cours. 

```C
Job jobs[MAX_JOBS];
```

On va mettre cette commande `sleep 10 &` dans une structure `Job` et l'ajouter au tableau de jobs.  
    
```C
typedef struct {
    int pid;
    char cmd[MAX_SUB_CMD_LEN];
    int id;
    enum status status;
    int exit_code; // Ca correspond à la valeur de retour de la commande 
} Job;
```

Pour surveiller son fils, le processus père va utiliser la fonction `waitpid` avec l'option `WNOHANG` qui permet de ne pas bloquer le processus père si son fils n'a pas terminé.

On va ajouter aussi les options `WUNTRACED` et `WCONTINUED` pour gérer leur status *(cas où le processus se fait stopper, ou se fait reprendre)*.

A chaque tour de boucle du shell, on va parcourir le tableau de jobs et on va vérifier si un job a changé de status. Si c'est le cas, on va afficher un message en fonction du status, et le supprimer du tableau de jobs si il est terminé ou tué.


## Exécution de fg

Pour éxécuter `fg %i`, on va parcourir le tableau de jobs et on va chercher le job avec l'id `i`. Si il existe, on va utiliser la fonction `kill` pour envoyer un signal `SIGCONT` au processus fils.
Le père va attendre que son fils termine et va supprimer le job du tableau de jobs.

## Exécution de bg

Pour éxécuter `bg %i`, on va parcourir le tableau de jobs et on va chercher le job avec l'id `i`. Si il existe, on va utiliser la fonction `kill` pour envoyer un signal `SIGCONT` au processus fils.
Le père ne va **pas** attendre que son fils termine et va afficher un message pour indiquer que le job est en cours d'exécution.

## Exécution de jobs 

Sans l'option `-t`, on va parcourir le tableau de jobs et on va afficher les jobs en cours d'exécution avec leur pid et leur status (`running`, `stopped`, `done`, `killed`).

Avec l'option `-t`, on va se servir de `proc` : 
- Pour chaque `PID` du tableau job, on va chercher le `pid` de ses fils en faisant : `/proc/PID/task/PID/children` 
- Pour chaque fils on va récupérer ses infomations en faisant : `/proc/PIDChildren/stat` puis vérifier si lui-même a des enfants.
- Cette méthode est récursive.


## Gestion des signaux

Dans un premier temps, le **shell** ignore les signaux : 
- `SIGINT`
- `SIGTERM` 
- `SIGTTIN` 
- `SIGQUIT` 
- `SIGTTOU` 
- `SIGTSTP` 
- `SIGALRM`

Dès que le shell va créer un fils pour excécuter la commande, il va changer son comportement lorsqu'il reçoit un signal : 
- Il va le transmettre a son fils si il n'est **pas** en `background` puis lorsque le fils a fini, il rétablit son comportement initial.


## Mise en place du shell

Pour mettre en place le shell, on va utiliser la fonction `readline` pour lire la ligne de commande et la fonction `add_history` pour ajouter la ligne de commande à l'historique.

La fonction `readline` va afficher le prompt et va lire la ligne de commande.
On va ensuite parser la ligne de commande et l'exécuter.
Le shell va se répéter tant que l'utilisateur n'a pas entré la commande `exit` *(grâce a une boucle `while(1)`)*.


## Description des fichiers

- `jsh.c` : Contient le main du shell
- `utils.c` : Contient les fonctions utilitaires
- `internalCommand.c` : Contient les implémentations des commandes internes
- `jobs-command.c` : Contient les implémentations de la commande `jobs` et avec l'options `-t`
- `jobs.c` : Contient les implémentations des fonctions pour gérer les jobs et executer les commandes 
- `kill.c` : Contient les implémentations des fonctions pour gérer les signaux (envoie de signaux)
- `pipe.c` : Contient les implémentations des fonctions pour gérer les commandes avec des pipes
- `redirections.c` : Contient les implémentations des fonctions pour gérer les commandes avec des redirections
- `substitution_process.c` : Contient les implémentations des fonctions pour gérer les commandes avec des substitutions de processus

