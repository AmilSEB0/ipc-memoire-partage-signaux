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

struct Client {
    std::string nomPrenom;
    std::string dernierMessage;
};

char * texte;
int shmid ;
std::map<int, Client> mapClient;
char autorisation; /* réponse (o/n) à une question */

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
            snprintf(texte, 50, "%d", getpid());
            if (autorisation == 'o') {
                std::strncpy(texte + 50, "Votre demande a été acceptée\n", 1000);
                kill(pid_client, SIGUSR1);
                mapClient.erase(pid_client);
                std::cout << "La map contient " << mapClient.size() << " éléments." << std::endl;
            } else {
                std::strncpy(texte + 50, "Votre demande a été refusée. Tapez sur la touche 'Entrée' pour pouvoir continuer à écrire.\n", 1000);
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

    //std::cout << "PID du client : " << pid << std::endl;
    //std::cout << "Message : " << message << std::endl;
}

void sigint_handler(int sig) {
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
    signal(SIGINT, sigint_handler);
    while (1) {
        pause();
    }
}