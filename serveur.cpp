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

char * texte;
int shmid ;

void sigusr1_handler(int sig) {
    printf("SIGUSR1 handler\n");
    // Lire le PID (premiers 50 caractères) et le message (500 caractères suivants)
    char pid[50];
    char message[1000];

    std::strncpy(pid, texte, 50); // Les 50 premiers caractères pour le PID
    std::strncpy(message, texte + 50, 1000); // Les 1000 caractères suivants pour le message

    // Afficher le PID et le message
    std::cout << "PID du client : " << pid << std::endl;
    std::cout << "Message : " << message << std::endl;
}

void sigint_handler(int sig) {
    int ret = shmdt(texte);
    if (ret == -1) { perror("SHMDT"); exit(3); }

    ret = shmctl(shmid, IPC_RMID, NULL);
    if (ret == -1) { perror("SHMCTL IPC_RMID"); exit(4); }

    printf("Serveur arrêté.\n");
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
