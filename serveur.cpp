#include <cstdlib>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

char * texte;
int shmid ;

void sigusr1_handler(int sig) {
    printf("%s\n", texte);
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
