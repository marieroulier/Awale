# Awale

## Compilation
Côté serveur :
    - make server
    - ./bin/server

Côté client :
    - make client
    - ./bin/client <adresse IP serveur> <pseudo client>


## Fonctionnalités implémentées
    - list : lister tous les clients disponibles pour une partie
    - bio : ajouter/modifier sa bio
    - consult : consulter la bio d'un client
    - friends : ajouter/modifier/enlever des amis de sa liste d'amis
    - observe : observer une partie de jeu
    - challenge : défier un autre client pour une partie
    - accept : accepter un challenge
    - refuse : refuser un challenge
    - quit : quitter le serveur

Toute autre entrée sera considérée comme un chat et sera diffusée à tous les clients, y compris ceux en partie.
