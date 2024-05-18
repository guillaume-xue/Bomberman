# Architecture du Projet - Bomberman en Réseau (bomberman)

# Module Principal (main.c)
    Gère le flux principal du programme.
    Initialise le serveur Bomberman, écoute les connexions des clients et gère les parties en cours.
    Gère les interactions avec les clients et les transferts de données.

# Module de Gestion des Parties (game_manager.c)
    Crée et gère les différentes parties de Bomberman.
    Associe les joueurs aux parties en attente ou crée de nouvelles parties si nécessaire.
    Multidiffuse l'état du jeu aux joueurs participants.

# Module de Gestion des Clients (client_manager.c)
    Gère les connexions des clients au serveur.
    Traite les demandes d'intégration à une partie et les actions des joueurs pendant le jeu.
    Gère les échanges de messages de chat entre les joueurs.

# Module de Gestion de la Grille (grid_manager.c)
    Crée et maintient l'état de la grille de jeu.
    Applique les règles du jeu et met à jour l'état de la grille en fonction des actions des joueurs.

# Module de Gestion des Actions (action_manager.c)
    Interprète les actions des joueurs reçues du client.
    Vérifie la validité des actions et met à jour l'état du jeu en conséquence.

# Module de Gestion du Chat (chat_manager.c)
    Gère les messages de chat entre les joueurs pendant la partie.
    Transmet les messages de chat aux joueurs concernés.

# Module de Gestion des Paramètres (config_manager.c)
    Charge et gère les paramètres configurables du serveur (taille de la grille, fréquences de multidiffusion et de traitement des requêtes, etc.).
    Permet de modifier les paramètres pendant l'exécution du serveur.

# Module de Communication Réseau (network_communication.c)
    Gère les échanges de données entre le serveur et les clients via TCP et UDP.
    Assure la réception et l'envoi des messages conformément au protocole défini.

# Fichiers d'En-tête (*.h)
    Déclare les prototypes des fonctions pour chaque module.
    Facilite la séparation de l'interface et de l'implémentation.

# Makefile (Makefile)
    Facilite la compilation et la gestion des dépendances du projet.
    Permet de compiler le serveur Bomberman en un exécutable fonctionnel.