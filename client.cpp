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
char nom;
int pid;

int main() {
    pid = getpid();
    shmid = shmget((key_t)50, 0, 0);
    if (shmid == -1) { perror ( "SHMGET" ); exit(1); }

    struct shmid_ds shmid_ds;

    if (shmctl(shmid, IPC_STAT, &shmid_ds) == -1) {
        perror("Erreur lors de shmctl");
        exit(1);
    }

    pid_t pid_client = shmid_ds.shm_cpid;

    texte = (char*) shmat ( shmid , NULL, 0 );

    std::cout << "--> Déclinez votre identité : ";
    std::cin >> nom;

    // Ignore the newline character left by std::cin >> nom;
    std::cin.ignore();
    while(1) {
        std::cout << "--> Tapez votre message : ";
        std::string message;
        std::getline(std::cin, message);

        std::strncpy(texte, message.c_str(), 155);
        kill(pid_client, SIGUSR1);
    }
}



