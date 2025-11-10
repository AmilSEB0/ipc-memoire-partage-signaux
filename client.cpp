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

int shmid;
char * texte;
int pid;
char rejoindreServeur; /* réponse (o/n) à une question */

// Fonction pour supprimer les espaces au début et à la fin d'une chaîne
std::string trim(const std::string &str) {
    size_t start = 0;
    size_t end = str.size();

    // Trouver la première position non-espaces
    while (start < end && std::isspace(str[start])) {
        start++;
    }

    // Trouver la dernière position non-espaces
    while (end > start && std::isspace(str[end - 1])) {
        end--;
    }

    // Retourner la sous-chaîne "trimée"
    return str.substr(start, end - start);
}

int main() {
    pid = getpid();
    shmid = shmget((key_t)50, 0, 0);
    if (shmid == -1) { perror ( "SHMGET" ); exit(1); }

    struct shmid_ds shmid_ds;

    if (shmctl(shmid, IPC_STAT, &shmid_ds) == -1) {
        perror("Erreur lors de shmctl");
        exit(1);
    }

    pid_t pid_serveur = shmid_ds.shm_cpid;

    printf("pid_serveur = %d\n", pid_serveur);

    texte = (char*) shmat ( shmid , NULL, 0 );

    printf("--> Voulez-vous rejoindre la mémoire partagé (o/n) ? : ");
    fflush(stdout);
    scanf("%c", &rejoindreServeur);
    std::cin.ignore();
    if(rejoindreServeur == 'n') {
        exit(0);
    }

    std::cout << "--> Déclinez votre identité :\n";

    std::cout << "Nom : ";
    std::string nom;
    std::getline(std::cin, nom);

    std::cout << "Prénom : ";
    std::string prenom;
    std::getline(std::cin, prenom);

    // Trim les espaces superflus
    nom = trim(nom);
    prenom = trim(prenom);

    // Combiner nom et prénom
    std::string nomPrenom = nom + " " + prenom;

    std::strncpy(texte + 50, nomPrenom.c_str(), 1000);

    kill(pid_serveur, SIGUSR1);

    // Ignore the newline character left by std::cin >> nom;
    //std::cin.ignore();
    while(1) {
        snprintf(texte, 50, "%d", pid);
        std::cout << "--> Tapez votre message : ";
        std::string message;
        std::getline(std::cin, message);

        message = trim(message);

        std::strncpy(texte + 50, message.c_str(), 1000);
        kill(pid_serveur, SIGUSR1);
    }
}