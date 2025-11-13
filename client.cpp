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
#include <termios.h>
#include <limits>

struct termios orig_termios;
int shmid;
char * texte;
int pid;
pid_t pid_serveur;
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

void disable_input(void) {
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);  // Sauvegarder l'état actuel du terminal
    new_termios = orig_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);  // Désactive l'entrée canonique et l'écho
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

void ecrireMessage(void){
    snprintf(texte, 50, "%d", pid);
    std::cout << "--> Tapez votre message : ";
    fflush(stdout); // Forcer l'affichage
    std::string message;
    std::getline(std::cin, message);

    message = trim(message);

    std::strncpy(texte + 50, message.c_str(), 1000);

    kill(pid_serveur, SIGUSR1);

    if (message == "quitter") {
        disable_input();
        pause();
    }
}

void sigusr1_handler(int sig) {
    char message[1000];

    std::strncpy(message, texte + 50, 1000);
    printf("%s", message);

    int ret = shmdt(texte);
    if (ret == -1) { perror("SHMDT"); exit(3); }

    // Réinitialisation du terminal en mode canonique
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);

    // Vider le buffer d'entrée (forcé par tcflush)
    tcflush(STDIN_FILENO, TCIFLUSH);

    //printf("Je sors de la mémoire partage\n");
    exit(0);
}

void sigusr2_handler(int sig) {
    char message[1000];

    std::strncpy(message, texte + 50, 1000);
    printf("%s", message);

    // Réinitialisation du terminal en mode canonique
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);

    // Vider le buffer d'entrée (forcé par tcflush)
    tcflush(STDIN_FILENO, TCIFLUSH);
}

void sigtstp_handler(int sig) {
    printf("\nTentative d’évasion échouée. Retour à ton clavier !\n");
}

void sigint_handler(int sig) {
    printf("SIGINT");
}

void sigquit_handler(int sig) {
    // Afficher le message d'avertissement
    printf("\nFuir n’était pas une option. 5 minutes de prison numérique, interdit de toucher à la mémoire partagée. Profite de ta pause forcée pour réfléchir à tes choix !\n");

    // Désactiver l'entrée
    disable_input();

    int remaining_time = 300; // Temps restant en secondes (5 minutes)

    // Afficher le temps restant toutes les secondes pendant 5 minutes
    while (remaining_time > 0) {
        int minutes = remaining_time / 60;  // Calculer le nombre de minutes
        int seconds = remaining_time % 60;  // Calculer le nombre de secondes restantes

        // Afficher le temps restant sous forme "minutes:secondes"
        printf("\rTemps restant : %02d:%02d", minutes, seconds); // \r efface la ligne précédente
        fflush(stdout); // Forcer l'affichage

        sleep(1);  // Attendre 1 seconde
        remaining_time--; // Réduire le temps restant
    }

    // Réinitialisation du terminal en mode canonique
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);

    // Vider le buffer d'entrée (forcé par tcflush)
    tcflush(STDIN_FILENO, TCIFLUSH);
    printf("\n");
    ecrireMessage();
}

void sigterm_handler(int sig) {
    printf("\nVous avez tenté de quitter la confrérie sans autorisation. Vous subirez le pire des châtiments : votre droit à la parole.\n");
    disable_input();
    std::cin.clear();  // Réinitialiser le flux d'entrée
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    // Le programme attend ici jusqu'à ce qu'il soit terminé ou qu'une action supplémentaire soit prise
    while (1) {
        // Bloquer l'exécution ici (le terminal est désactivé)
        pause();
    }
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

    pid_serveur = shmid_ds.shm_cpid;

    printf("pid_serveur = %d\n", pid_serveur);

    texte = (char*) shmat ( shmid , NULL, 0 );

    signal(SIGUSR1, sigusr1_handler);
    signal(SIGUSR2, sigusr2_handler);
    signal(SIGINT, sigint_handler);
    signal(SIGQUIT, sigquit_handler);
    signal(SIGTSTP, sigtstp_handler);
    signal(SIGTERM, sigterm_handler);

    printf("--> Voulez-vous rejoindre la mémoire partagé (o/n) ? : ");
    fflush(stdout);
    scanf("%c", &rejoindreServeur);
    std::cin.ignore();
    if(rejoindreServeur == 'n') {
        printf("Refus non accepté. Le système a décidé pour toi : bienvenue dans la mémoire partagée !\n");
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

    // Combiner prénom et nom
    std::string nomPrenom = prenom + " " + nom;

    std::strncpy(texte + 50, nomPrenom.c_str(), 1000);

    kill(pid_serveur, SIGUSR1);

    // Ignore the newline character left by std::cin >> nom;
    //std::cin.ignore();
    while(1) {
        ecrireMessage();
    }
}