#include <cstdlib>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <cstring>
#include <map>
#include <limits>

// Structure représentant un client (nom + dernier message envoyé)
struct Client {
    std::string nomPrenom;      // Nom complet (Prénom + Nom)
    std::string dernierMessage; // Dernier message envoyé au serveur
};


// Pointeur vers la mémoire partagée (zone de communication)
char * texte;

// Identifiant du segment de mémoire partagée
int shmid ;

// Map qui associe chaque PID client → infos du client
std::map<int, Client> mapClient;

// Stocke une réponse du serveur dans certains dialogues
std::string autorisation;

// Ensemble de signaux (pour bloquer/débloquer SIGUSR1)
sigset_t set;


// Bloque SIGUSR1 afin d'éviter d'être interrompu pendant une opération importante
void bloquer_SIGUSR1() {
    sigemptyset(&set);                // Initialise un ensemble de signaux vide
    sigaddset(&set, SIGUSR1);         // Ajoute SIGUSR1 à cet ensemble
    sigprocmask(SIG_BLOCK, &set, NULL);  // Bloque SIGUSR1 pour ce processus
}

// Débloque SIGUSR1 (permet à nouveau au serveur de le recevoir)
void debloquer_SIGUSR1() {
    sigprocmask(SIG_UNBLOCK, &set, NULL); // Débloque SIGUSR1
}


// Fonction d’affichage du menu du serveur
void afficherMenu() {
    std::cout << "\n--- Menu ---\n";                                         // Titre du menu
    std::cout << "1. Voir les derniers messages de tous les clients\n";      // Option 1
    std::cout << "2. Voir le dernier message d'un client spécifique\n";      // Option 2
    std::cout << "3. Faire quitter un client de la mémoire partagée\n";      // Option 3
    std::cout << "4. Détruire la mémoire partagé\n";                        // Option 4
    std::cout << "Choisissez une option (1-4): ";                             // Invite de choix
    fflush(stdout);                                                           // Force l'affichage
}


// Affiche le dernier message de tous les clients
void afficherTousLesMessages() {
    if (mapClient.empty()) {                              // Vérifie si des clients existent
        std::cout << "Aucun client connecté.\n";
        return;
    }

    std::cout << "\nDerniers messages de tous les clients:\n";

    for (const auto& client : mapClient) {                // Parcourt tous les clients
        std::cout << client.second.nomPrenom              // Affiche le nom du client
                  << " a dit: "
                  << client.second.dernierMessage         // Affiche son dernier message
                  << std::endl;
    }
}


// Affiche le dernier message d’un client choisi par son nom
void afficherMessageClient() {
    if (mapClient.empty()) {                              // Aucun client ?
        std::cout << "Aucun client connecté.\n";
        return;
    }

    // Affiche la liste des noms disponibles
    for (const auto& client : mapClient) {
        std::cout << client.second.nomPrenom << std::endl;
    }

    std::string nom;
    std::cout << "Entrez le nom complet du client: ";

    // Vide le tampon avant lecture
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // Lecture du nom complet
    std::getline(std::cin, nom);

    bool clientTrouve = false;

    // Recherche le client correspondant
    for (const auto& client : mapClient) {
        if (client.second.nomPrenom == nom) {             // Nom trouvé ?
            std::cout << "Le dernier message de "
                      << nom << " est : "
                      << client.second.dernierMessage
                      << std::endl;
            clientTrouve = true;
            break;
        }
    }

    if (!clientTrouve) { // Nom introuvable
        std::cout << "Client non trouvé.\n";
    }
}


// Fonction permettant au serveur de forcer un client à quitter
void faireQuitterClient() {
    if (mapClient.empty()) {
        std::cout << "Aucun client connecté.\n";
        return;
    }

    // Affiche les clients présents
    for (const auto& client : mapClient) {
        std::cout << client.second.nomPrenom << std::endl;
    }

    std::string nom;
    std::cout << "Entrez le nom complet du client à faire quitter: ";

    // Vide le tampon pour éviter des lectures invalides
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // Lecture du nom du client
    std::getline(std::cin, nom);

    bool clientTrouve = false;

    // Recherche du client
    for (auto& client : mapClient) {
        if (client.second.nomPrenom == nom) { // Correspondance trouvée

            std::cout << "Envoi du signal à " << nom
                      << " pour qu'il quitte la mémoire partagée.\n";

            kill(client.first, SIGUSR1); // SIGUSR1 → le client se termine

            mapClient.erase(client.first); // Retire du tableau des clients
            clientTrouve = true;
            break;
        }
    }

    if (!clientTrouve) { // Client non trouvé
        std::cout << "Client non trouvé.\n";
    }
}


// Handler exécuté quand un client envoie SIGUSR1
void sigusr1_handler(int sig) {
    printf("SIGUSR1 handler\n");

    char pid[50]; // Zone pour lire le PID client depuis la mémoire partagée
    char message[1000]; // Zone pour lire le message envoyé

    std::strncpy(pid, texte, 50); // Copie le PID stocké par le client
    std::strncpy(message, texte + 50, 1000);  // Copie le message du client

    int pid_client = std::stoi(pid); // Convertit PID en integer

    auto it = mapClient.find(pid_client); // Recherche client dans la map

    // Cas 1 → Client existe déjà
    if (it != mapClient.end()) {

        // Cas particulier → Client veut quitter la mémoire partagée
        if (strcmp(message,
        "Cher serveur, J'ai passé un moment agréable dans cette mémoire partagée, Entre tes lignes de code et tes accès bien ordonnés. Mais je crois qu'il est temps pour moi de m’éclipser, De libérer cet espace que j'ai occupé, Pour laisser place à d'autres processus, d'autres voyageurs. Ne t'inquiète pas, je reviendrai un jour, Peut-être quand tu m'inviteras à nouveau. Mais pour l’instant, je quitte ce monde binaire, Et je me retire en paix, léger, serein. Merci pour tout, serveur, et à bientôt… Bisous.") == 0) {

            // Empêche interruption du serveur lors du dialogue
            bloquer_SIGUSR1();

            std::cout << "--> Autorisez-vous "
                      << it->second.nomPrenom
                      << " à quitter la mémoire partagée (o/n) ?: ";
            
            std::cin >> autorisation;          // Lecture de l’autorisation
            std::cin.ignore();                 // Vide le tampon

            // On écrit notre PID dans la mémoire partagée
            snprintf(texte, 50, "%d", getpid());

            // Si le serveur accepte avec une phrase très particulière
            if (autorisation ==
            "Cher client, Je vois que tu veux quitter ma belle mémoire partagée, Mon cœur se déchire en apprenant cette nouvelle, Blablabla, Au revoir, cher voyageur. Bisous Bisous") {

                std::strncpy(texte + 50,
                  "Votre demande a été acceptée\n", 1000);

                kill(pid_client, SIGUSR1);   // Réponse → client se termine

                mapClient.erase(pid_client); // Supprime le client de la map

                std::cout << "La map contient "
                          << mapClient.size()
                          << " éléments."
                          << std::endl;
            } 
            else {
                // Refus
                std::strncpy(texte + 50,
                  "Votre demande a été refusée.\n", 1000);
                kill(pid_client, SIGUSR2);   // Client est averti
            }

            // Réautorise SIGUSR1
            debloquer_SIGUSR1();
        }

        // Cas classique → simple message
        else {
            it->second.dernierMessage = message;          // Mise à jour du dernier message
            std::cout << it->second.nomPrenom
                      << " a dit: "
                      << it->second.dernierMessage
                      << std::endl;
        }
    }

    // Cas 2 → Nouveau client (non présent dans la map)
    else {
        std::string NomPrenom = message;                 // Récupère nom complet
        mapClient[pid_client] = Client{NomPrenom, ""};   // Ajout dans la map
        std::cout << "Bienvenue à: " << NomPrenom << std::endl;
    }

    // Réaffiche le menu après chaque message
    afficherMenu();
}


// Handler exécuté lors de la destruction de la mémoire partagée
void destruction_memoire_partage_handler(int sig) {

    // Préviens tous les clients qu’ils doivent quitter
    for (auto& client : mapClient) {
        std::cout << "PID du client : " << client.first << std::endl;
        kill(client.first, SIGUSR1); // Termine les clients
    }

    mapClient.clear(); // Vide la map

    struct shmid_ds shmid_ds;

    // Récupère les informations du segment partagé
    int ret = shmctl(shmid, IPC_STAT, &shmid_ds);
    if (ret == -1) {
        perror("Erreur lors de shmctl");
        exit(1);
    }

    std::cout << "Nombre de processus attachés : "
              << shmid_ds.shm_nattch << std::endl;

    // Détache la mémoire partagée
    ret = shmdt(texte);
    if (ret == -1) {
        perror("Erreur lors du détachement (shmdt)");
        exit(3);
    }

    // Supprime la mémoire partagée au niveau système
    ret = shmctl(shmid, IPC_RMID, NULL);
    if (ret == -1) {
        perror("Erreur lors de la suppression (shmctl IPC_RMID)");
        exit(4);
    }

    printf("Serveur arrêté proprement et mémoire partagée supprimée.\n");
    exit(0);
}


// Handler pour les punitions envoyées par les clients
void sigusr2_handler(int sig) {

    char pid[50];
    char message[1000];

    std::strncpy(pid, texte, 50); // PID du client émetteur
    std::strncpy(message, texte + 50, 1000); // Message du client

    int pid_client = std::stoi(pid);

    // On écrit notre PID dans la mémoire partagée
    snprintf(texte, 50, "%d", getpid());

    // Le client tente Ctrl+Z
    if (strcmp(message, "SIGTSTP") == 0) {
        std::strncpy(texte + 50,
          "\nTentative d’évasion échouée. Retour à ton clavier !\n",
        1000);
    }

    // Le client tente Ctrl+\ (SIGQUIT)
    else if (strcmp(message, "SIGQUIT") == 0) {
        std::strncpy(texte + 50,
        "\nFuir n’était pas une option. 5 minutes de prison numérique...\n",
        1000);
    }

    // Le client tente SIGTERM
    else if (strcmp(message, "SIGTERM") == 0) {
        std::strncpy(texte + 50,
        "\nVous avez tenté de quitter la confrérie sans autorisation...\n",
        1000);
    }

    // Le client fait Ctrl+C : tout le monde doit le huer
    else if (strcmp(message, "SIGINT") == 0) {

        auto it = mapClient.find(pid_client);

        std::string messagePunition;

        if (it != mapClient.end()) {
            messagePunition =
            "pid_client:" + std::to_string(pid_client) +
            "message: Huez " + it->second.nomPrenom;
        } else {
            printf("cette personne n'existe pas");
        }

        // Empêche interruptions pendant la tournée des clients
        bloquer_SIGUSR1();

        // On demande à tous les autres clients d’envoyer un message méchant
        for (auto it = mapClient.begin(); it != mapClient.end(); ++it) {

            if (it->first != pid_client){

                printf("\nEn attente que %s rédige son message\n",
                       it->second.nomPrenom.c_str());

                snprintf(texte, 50, "%d", getpid());
                std::strncpy(texte + 50, messagePunition.c_str(), 1000);

                kill(it->first, SIGCONT); // Débloque le client puni
                pause(); // Attend sa réponse
            }
        }

        snprintf(texte, 50, "%d", getpid());

        std::strncpy(texte + 50,
          "De la part du chef : J'espère que cela te servira de leçon\n",
        1000);

        debloquer_SIGUSR1();

        afficherMenu();
    }

    kill(pid_client, SIGCONT);                 // Réveille le client puni
}


// Handler pour SIGCONT (ne fait qu'afficher un message)
void sigcont_handler(int sig){
    printf("SIGCONT\n");
}


// Fonction principale du serveur
int main() {

    // Tente de créer un segment de mémoire partagée de 1050 octets
    shmid = shmget((key_t)50, 1050,
    IPC_CREAT | IPC_EXCL | S_IWUSR | S_IRUSR);

    // Si la mémoire existe déjà → erreur
    if (shmid == -1) {

        if (errno == EEXIST) {
            std::cerr << "Erreur : La mémoire partagée existe déjà." 
                      << std::endl;
        }
        else {
            std::cerr << "Erreur : échec de création. Code "
                      << errno << std::endl;
        }

        return 1;
    }

    // Attache la mémoire partagée au processus serveur
    texte = (char*) shmat(shmid , NULL, 0);

    // Affiche le PID du serveur
    printf("PID: %d\n", getpid());

    // Lie les signaux à leurs handlers
    signal(SIGUSR1, sigusr1_handler);
    signal(SIGUSR2, sigusr2_handler);
    signal(SIGINT, destruction_memoire_partage_handler);
    signal(SIGQUIT, destruction_memoire_partage_handler); // fait la même chose que SIGINT
    signal(SIGTSTP, destruction_memoire_partage_handler); // fait la même chose que SIGINT
    signal(SIGTERM, destruction_memoire_partage_handler); // fait la même chose que SIGINT
    signal(SIGCONT, sigcont_handler);


    while (1) { // Boucle principale du serveur
        afficherMenu(); // Affiche le menu

        int choix;
        std::cin >> choix; // Lecture du choix utilisateur

        // Vérifie si la saisie est invalide
        if (std::cin.fail()) {

            std::cin.clear(); //  Effacer les erreurs de saisie
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Entrée invalide. Essayez encore." << std::endl;

            continue; // Retour au menu
        }

        // Exécute l’option choisie
        switch (choix) {

            case 1:
                afficherTousLesMessages();
                break;

            case 2:
                afficherMessageClient();
                break;

            case 3:
                faireQuitterClient();
                break;

            case 4:
                std::cout << "Arrêt du serveur...\n";
                destruction_memoire_partage_handler(0);
                break;

            default:
                printf("\n");
                std::cout << "Option invalide. Essayez encore.\n";
                break;
        }
    }
}