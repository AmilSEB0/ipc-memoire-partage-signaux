# IPC — Mémoire partagée et signaux Unix

Projet **C++** de communication inter-processus permettant à plusieurs clients de se connecter à un serveur et d'échanger des messages via une **mémoire partagée System V**. La communication et la synchronisation entre processus sont assurées par des **signaux Unix**.

Le projet a été réalisé dans le cadre d'un cours sur les processus et les mécanismes d'IPC (*Inter-Process Communication*). L'objectif initial était de développer un serveur créant un espace de mémoire partagée auquel plusieurs clients peuvent se connecter, puis utiliser cet espace pour communiquer et échanger des messages.

J'ai ensuite étendu le projet avec plusieurs fonctionnalités supplémentaires autour de la gestion des clients et des signaux Unix.

## Objectif

Le serveur crée une **mémoire partagée** accessible par plusieurs processus clients.

Chaque client peut :

- se connecter au serveur
- renseigner son nom et son prénom
- envoyer des messages
- recevoir les réponses du serveur
- demander à quitter la mémoire partagée
- interagir avec les autres clients à travers le serveur

Le serveur conserve pour chaque client :

- son PID
- son nom et prénom
- son dernier message envoyé

Il dispose également d'un menu permettant de consulter les clients et leurs messages ou de gérer leur déconnexion.

## Architecture

Le projet est composé de deux programmes :

```text
                    ┌──────────────────┐
                    │     SERVEUR      │
                    │                  │
                    │  Gestion clients │
                    │  Menu            │
                    │  Messages        │
                    └────────┬─────────┘
                             │
                  Mémoire partagée System V
                             │
          ┌──────────────────┼──────────────────┐
          │                  │                  │
          ▼                  ▼                  ▼
    ┌───────────┐      ┌───────────┐      ┌───────────┐
    │  Client 1 │      │  Client 2 │      │  Client 3 │
    └───────────┘      └───────────┘      └───────────┘
```
La mémoire partagée sert de zone commune pour transmettre les informations entre les processus.

Les **signaux Unix** permettent quant à eux de notifier les différents processus lorsqu'une action doit être traitée.

## Communication
La mémoire partagée est organisée en deux zones principales :
```text
┌──────────────────────┬──────────────────────────────────┐
│      50 octets       │           1000 octets            │
│         PID          │             Message              │
└──────────────────────┴──────────────────────────────────┘
```
Le PID de l'émetteur est écrit dans la première partie et les données ou le message dans la seconde.

Les processus utilisent ensuite différents signaux pour déclencher les traitements correspondants.

## Principaux signaux utilisés
| Signal | Utilisation |
| :--- | :--- |
| `SIGUSR1` | Communication principale client ↔ serveur |
| `SIGUSR2` | Réponses et notifications particulières |
| `SIGINT` | Gestion de Ctrl+C |
| `SIGTSTP` | Gestion de Ctrl+Z |
| `SIGQUIT` | Gestion de Ctrl+\ |
| `SIGTERM` | Gestion d'une tentative d'arrêt |
| `SIGCONT` | Synchronisation et interactions entre clients |

## Gestion des clients
Le serveur utilise une `std::map` pour associer chaque PID à un objet `Client` :
```cpp
std::map<int, Client> mapClient;
```
Chaque client est représenté par :
```markdown
struct Client {
    std::string nomPrenom;
    std::string dernierMessage;
};
```

Cela permet notamment au serveur de :

- détecter les nouvelles connexions
- identifier l'émetteur d'un message grâce à son PID
- conserver le dernier message de chaque client
- afficher la liste des utilisateurs connectés
- retrouver le dernier message d'un utilisateur
- gérer la déconnexion d'un client

## Fonctionnalités du serveur

Le serveur possède un menu interactif permettant de :
1. consulter les derniers messages de tous les clients
2. consulter le dernier message d'un client particulier
3. demander à un client de quitter la mémoire partagée
4. détruire la mémoire partagée et arrêter le serveur

Lorsqu'un nouveau client se connecte, le serveur reçoit son identité et l'ajoute automatiquement à la liste des clients.

Lorsqu'un client envoie un message, le serveur met à jour son dernier message.

## Fonctionnalités supplémentaires
Le sujet demandait principalement la mise en place de la communication entre un serveur et plusieurs clients.

J'ai volontairement ajouté plusieurs mécanismes supplémentaires afin d'explorer davantage les signaux Unix et la synchronisation entre processus.

### Gestion des tentatives de sortie

J'ai également ajouté un système de gestion humoristique des tentatives de sortie du programme à l'aide des signaux Unix.

Les raccourcis clavier sont interceptés par les clients et transmis au serveur, qui décide ensuite de la réaction à appliquer.

- **`Ctrl+C` (`SIGINT`)** : le client ne quitte pas immédiatement. Le serveur déclenche une "punition collective" : il demande aux autres clients connectés de rédiger chacun un message de moquerie destiné au client ayant tenté de quitter le programme. Les messages sont ensuite transmis au client concerné avant qu'un dernier message du serveur ne lui soit envoyé.

  Par exemple :

  ```text
  Client A appuie sur Ctrl+C

  Serveur :
  "Huez Client A"

  Client B :
  "Tu pensais vraiment pouvoir partir comme ça ?"

  Client C :
  "Ctrl+C ne te sauvera pas cette fois !"

  Serveur :
  "De la part du chef : J'espère que cela te servira de leçon"
  ```

- **`Ctrl+Z` (`SIGTSTP`)** : au lieu de suspendre le processus, le signal est intercepté et envoyé au serveur. Celui-ci renvoie un message indiquant que la tentative de suspension a échoué.

- **`Ctrl+\` (`SIGQUIT`)** : le signal est également intercepté. Le serveur informe le client qu'une "punition" lui est appliquée, puis le client lance un compte à rebours de plusieurs minutes avant de retrouver son fonctionnement normal.

- **`SIGTERM`** : le client informe le serveur qu'une tentative d'arrêt a eu lieu. Le serveur renvoie un message, puis le client reste volontairement bloqué afin d'empêcher sa fermeture normale.

Ces fonctionnalités sont volontairement humoristiques et ne constituent pas le cœur du projet. Elles ont été ajoutées pour aller au-delà du sujet initial et expérimenter davantage avec les **signaux Unix, la synchronisation entre processus et la communication entre plusieurs clients via le serveur**.

### Déconnexion contrôlée
Un client peut également envoyer une demande particulière au serveur afin de quitter la mémoire partagée.

Le serveur peut alors accepter ou refuser la demande avant de retirer le client de sa liste.

### Interaction entre clients
Lorsqu'un client déclenche certaines actions, le serveur peut demander aux autres clients d'interagir avec lui.

Cela permet d'expérimenter un scénario où le serveur orchestre plusieurs processus indépendants en utilisant les signaux et la mémoire partagée.

## Technologies utilisées
- C++
- Linux / Unix
- System V IPC
- Mémoire partagée : `shmget`, `shmat`, `shmdt`, `shmctl`
- Signaux Unix : `signal` `kill`, `pause`, `sigprocmask`
- Gestion du terminal : `termios`
- Processus et PID
- STL : `std::map`, `std::string`

## Lancement
Avant de lancer les programmes, vous devez compiler les fichiers sources :
```bash
g++ serveur.cpp -o serveur
g++ client.cpp -o client
```

Le serveur doit être lancé en premier afin de créer la mémoire partagée.
```text
./serveur
```
Puis un ou plusieurs clients peuvent être lancés dans différents terminaux :
```text
./client
```
Chaque client récupère automatiquement le PID du serveur à partir des informations du segment de mémoire partagée.

Plusieurs clients peuvent être exécutés simultanément pour tester la communication.

## Exemple
```text
--- Menu ---
1. Voir les derniers messages de tous les clients
2. Voir le dernier message d'un client spécifique
3. Faire quitter un client de la mémoire partagée
4. Détruire la mémoire partagé

Derniers messages de tous les clients:

Alice Martin a dit: Bonjour tout le monde !
Jean Dupont a dit: Salut Alice !
Bob Durand a dit: Comment allez-vous ?
```
Le serveur agit comme point central de gestion. La mémoire partagée sert de zone commune pour échanger les données entre processus, tandis que les signaux Unix permettent de notifier les processus et de synchroniser les différentes actions.

## Ce que ce projet m'a permis d'expérimenter
Au-delà de l'objectif initial du devoir, ce projet m'a permis de travailler concrètement avec les mécanismes bas niveau de Linux liés aux processus et à la communication inter-processus.

J'ai notamment pu expérimenter :

- la création et la gestion d'une mémoire partagée
- la communication entre plusieurs processus indépendants
- l'utilisation et le traitement des signaux Unix
- la synchronisation entre processus
- l'identification des processus par leur PID
- la gestion dynamique de plusieurs clients
- la manipulation du terminal avec `termios`
- la gestion de différents scénarios de communication et de synchronisation

Le projet a commencé comme une implémentation relativement simple de **serveur + clients + mémoire partagée**, puis a progressivement évolué vers un système plus complet afin d'explorer les possibilités offertes par les IPC et les signaux Unix.

> **Note :** certaines fonctionnalités supplémentaires sont volontairement humoristiques. Elles ont été ajoutées après la réalisation de la fonctionnalité principale afin d'expérimenter davantage les mécanismes étudiés pendant le cours.

Auteur : Amil Sebo