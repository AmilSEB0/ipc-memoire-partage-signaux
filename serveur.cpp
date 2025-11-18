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

struct Client {
    std::string nomPrenom;
    std::string dernierMessage;
};

char * texte;
int shmid ;
std::map<int, Client> mapClient;
char autorisation; /* réponse (o/n) à une question */

void afficherMenu() {
    std::cout << "\n--- Menu ---\n";
    std::cout << "1. Voir les derniers messages de tous les clients\n";
    std::cout << "2. Voir le dernier message d'un client spécifique\n";
    std::cout << "3. Faire quitter un client de la mémoire partagée\n";
    std::cout << "4. Détruire la mémoire partagé\n";
    std::cout << "Choisissez une option (1-4): ";
    fflush(stdout);
}

void afficherTousLesMessages() {
    if (mapClient.empty()) {
        std::cout << "Aucun client connecté.\n";
        return;
    }

    std::cout << "\nDerniers messages de tous les clients:\n";
    for (const auto& client : mapClient) {
        std::cout << client.second.nomPrenom << " a dit: " << client.second.dernierMessage << std::endl;
    }
}

void afficherMessageClient() {
    if (mapClient.empty()) {
        std::cout << "Aucun client connecté.\n";
        return;
    }

    for (const auto& client : mapClient) {
        std::cout << client.second.nomPrenom << std::endl;
    }

    std::string nom;
    std::cout << "Entrez le nom complet du client: ";

    // Ajouter cette ligne pour vider le tampon avant de lire une nouvelle ligne
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // Lire le nom complet du client
    std::getline(std::cin, nom);

    bool clientTrouve = false;
    for (const auto& client : mapClient) {
        if (client.second.nomPrenom == nom) {
            std::cout << "Le dernier message de " << nom << " est : " << client.second.dernierMessage << std::endl;
            clientTrouve = true;
            break;
        }
    }

    if (!clientTrouve) {
        std::cout << "Client non trouvé.\n";
    }
}

void faireQuitterClient() {
    if (mapClient.empty()) {
        std::cout << "Aucun client connecté.\n";
        return;
    }

    for (const auto& client : mapClient) {
        std::cout << client.second.nomPrenom << std::endl;
    }

    std::string nom;
    std::cout << "Entrez le nom complet du client à faire quitter: ";

    // Ajouter cette ligne pour vider le tampon avant de lire une nouvelle ligne
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // Lire le nom complet du client
    std::getline(std::cin, nom);

    bool clientTrouve = false;
    for (auto& client : mapClient) {
        if (client.second.nomPrenom == nom) {
            std::cout << "Envoi du signal SIGUSR1 à " << nom << " pour qu'il quitte la mémoire partagée.\n";
            kill(client.first, SIGUSR1);
            mapClient.erase(client.first);
            clientTrouve = true;
            break;
        }
    }

    if (!clientTrouve) {
        std::cout << "Client non trouvé.\n";
    }
}

void sigusr1_handler(int sig) {
    printf("SIGUSR1 handler\n");
    char pid[50];
    char message[1000];

    std::strncpy(pid, texte, 50);
    std::strncpy(message, texte + 50, 1000);

    int pid_client = std::stoi(pid);

    auto it = mapClient.find(pid_client);

    if (it != mapClient.end()) {
        if (strcmp(message, "quitter") == 0) {
            printf("--> Autorisez-vous %s à quitter la mémoire partagée (o/n) ? : ", it->second.nomPrenom.c_str());
            fflush(stdout);
            scanf(" %c", &autorisation);
            std::cin.ignore();  // Vide le tampon
            snprintf(texte, 50, "%d", getpid());
            if (autorisation == 'o') {
                std::strncpy(texte + 50, "Votre demande a été acceptée\n", 1000);
                kill(pid_client, SIGUSR1);
                mapClient.erase(pid_client);
                std::cout << "La map contient " << mapClient.size() << " éléments." << std::endl;
            } else {
                std::strncpy(texte + 50, "Votre demande a été refusée.\n", 1000);
                kill(pid_client, SIGUSR2);
            }
        } else {
            it->second.dernierMessage = message;
            std::cout << it->second.nomPrenom << " a dit: " << it->second.dernierMessage << std::endl;
        }
    } else {
        std::string NomPrenom = message;
        mapClient[pid_client] = Client{NomPrenom, ""};
        std::cout << "Bienvenue à: " << NomPrenom << std::endl;
    }
    afficherMenu();
    //std::cout << "PID du client : " << pid << std::endl;
    //std::cout << "Message : " << message << std::endl;
}

void destruction_memoire_partage_handler(int sig) {
    for (auto& client : mapClient) {
        std::cout << "PID du client : " << client.first << std::endl;
        kill(client.first, SIGUSR1);
    }

    mapClient.clear();

    struct shmid_ds shmid_ds;
    int ret = shmctl(shmid, IPC_STAT, &shmid_ds);
    if (ret == -1) {
        perror("Erreur lors de shmctl");
        exit(1);
    }

    std::cout << "Nombre de processus attachés : " << shmid_ds.shm_nattch << std::endl;

    ret = shmdt(texte);
    if (ret == -1) {
        perror("Erreur lors du détachement de la mémoire partagée (shmdt)");
        exit(3);
    }

    ret = shmctl(shmid, IPC_RMID, NULL);
    if (ret == -1) {
        perror("Erreur lors de la suppression du segment de mémoire partagée (shmctl IPC_RMID)");
        exit(4);
    }

    printf("Serveur arrêté proprement et mémoire partagée supprimée.\n");
    exit(0);
}

void sigusr2_handler(int sig) {
    char pid[50];
    char message[1000];

    std::strncpy(pid, texte, 50);
    std::strncpy(message, texte + 50, 1000);

    int pid_client = std::stoi(pid);
    snprintf(texte, 50, "%d", getpid());
    if (strcmp(message, "SIGTSTP") == 0) {
        std::strncpy(texte + 50, "\nTentative d’évasion échouée. Retour à ton clavier !\n", 1000);
    } else if (strcmp(message, "SIGQUIT") == 0) {
        std::strncpy(texte + 50, "\nFuir n’était pas une option. 5 minutes de prison numérique, interdit de toucher à la mémoire partagée. Profite de ta pause forcée pour réfléchir à tes choix !\n", 1000);
    } else if (strcmp(message, "SIGTERM") == 0) {
        std::strncpy(texte + 50, "\nVous avez tenté de quitter la confrérie sans autorisation. Vous subirez le pire des châtiments : votre droit à la parole.\n", 1000);
    }
    kill(pid_client, SIGCONT);
}

int main() {
    // Tentative de création de la mémoire partagée
    shmid = shmget((key_t)50, 1050, IPC_CREAT | IPC_EXCL | S_IWUSR | S_IRUSR);

    // Vérification des erreurs
    if (shmid == -1) {
        if (errno == EEXIST) {
            std::cerr << "Erreur : La mémoire partagée existe déjà." << std::endl;
        } else {
            std::cerr << "Erreur : échec de la création de la mémoire partagée. Code d'erreur : " << errno << std::endl;
        }
        return 1;
    }

    texte = (char*) shmat ( shmid , NULL, 0 );

    printf("PID: %d\n", getpid());
    signal(SIGUSR1, sigusr1_handler);
    signal(SIGUSR2, sigusr2_handler);
    signal(SIGINT, destruction_memoire_partage_handler);
    signal(SIGQUIT, destruction_memoire_partage_handler); // fait la même chose que SIGINT
    signal(SIGTSTP, destruction_memoire_partage_handler); // fait la même chose que SIGINT
    signal(SIGTERM, destruction_memoire_partage_handler); // fait la même chose que SIGINT

    while (1) {
        afficherMenu();

        int choix;
        std::cin >> choix;

        if (std::cin.fail()) {
            std::cin.clear();  // Effacer les erreurs de saisie
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  // Ignorer le reste de la ligne
            std::cout << "Entrée invalide. Essayez encore." << std::endl;
            continue;  // Reprendre la boucle
        }

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