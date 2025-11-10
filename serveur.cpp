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

void sigusr1_handler(int sig) {
    printf("SIGUSR1 handler\n");
    // Lire le PID (premiers 50 caractères) et le message (500 caractères suivants)
    char pid[50];
    char message[1000];

    std::strncpy(pid, texte, 50); // Les 50 premiers caractères pour le PID
    std::strncpy(message, texte + 50, 1000); // Les 1000 caractères suivants pour le message

    int pid_client = std::stoi(pid);

    auto it = mapClient.find(pid_client);

    if (it != mapClient.end()) {
        it->second.dernierMessage = message;
        std::cout << it->second.nomPrenom << " a dit: " << it->second.dernierMessage << std::endl;
    } else {
        std::string NomPrenom = message;
        mapClient[pid_client] = Client{NomPrenom, ""};
        std::cout << "Bienvenue à: " << NomPrenom << std::endl;
    }

    //std::cout << "PID du client : " << pid << std::endl;
    //std::cout << "Message : " << message << std::endl;
}

void sigint_handler(int sig) {
    // Détachement de la mémoire partagée
    int ret = shmdt(texte);
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
    shmid = shmget((key_t) 50, 256, IPC_CREAT | S_IWUSR | S_IRUSR);
    if ( shmid == -1) { perror ( "SHMGET" ); exit(1); }

    texte = (char*) shmat ( shmid , NULL, 0 );

    printf("PID: %d\n", getpid());
    signal(SIGUSR1, sigusr1_handler);
    signal(SIGINT, sigint_handler);
    while (1) {
        pause();
    }
}
