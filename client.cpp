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
bool signalServeur;
std::string nomPrenom;
bool clavierActiver = true;
bool traitementSignal = false;

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
    clavierActiver = false;
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);  // Sauvegarder l'état actuel du terminal
    new_termios = orig_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);  // Désactive l'entrée canonique et l'écho
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

void restore_input(void) {
    if (clavierActiver == false) {
        // Réinitialisation du terminal en mode canonique
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
        // Vider le buffer d'entrée (forcé par tcflush)
        tcflush(STDIN_FILENO, TCIFLUSH);
        clavierActiver = true;
    }
}

void ecrireMessage(void){
    traitementSignal = false;
    signalServeur = false;
    snprintf(texte, 50, "%d", pid);
    std::cout << "--> Tapez votre message : ";
    fflush(stdout); // Forcer l'affichage
    std::string message;
    std::getline(std::cin, message);
    message = trim(message);
    std::strncpy(texte + 50, message.c_str(), 1000);
    kill(pid_serveur, SIGUSR1);
    if (message == "quitter") {
        printf("\nEn attente de la réponse du serveur\n");
        // On est obligé de remettre le pid car le serveur reçoit le pid de la personne qui a écrit
        snprintf(texte, 50, "%d", pid);
        disable_input();
        pause();
    }
}

void sigusr1_handler(int sig) {
    char message[1000];

    std::strncpy(message, texte + 50, 1000);
    printf("%s", message);
    fflush(stdout);

    int ret = shmdt(texte);
    if (ret == -1) { perror("SHMDT"); exit(3); }

    if(clavierActiver == false) {
        // Réinitialisation du terminal en mode canonique
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);

        // Vider le buffer d'entrée (forcé par tcflush)
        tcflush(STDIN_FILENO, TCIFLUSH);
    }

    //printf("Je sors de la mémoire partage\n");
    exit(0);
}

void sigusr2_handler(int sig) {
    char message[1000];

    std::strncpy(message, texte + 50, 1000);
    printf("%s", message);

    if(clavierActiver == false) {
        // Réinitialisation du terminal en mode canonique
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);

        // Vider le buffer d'entrée (forcé par tcflush)
        tcflush(STDIN_FILENO, TCIFLUSH);
    }
}

void sigtstp_handler(int sig) {
    if (traitementSignal) {
        return;
    }
    traitementSignal = true;
    snprintf(texte, 50, "%d", pid);
    std::strncpy(texte + 50, "SIGTSTP", 1000);
    kill(pid_serveur, SIGUSR2);
    while(signalServeur == false) {
        pause();
        char message[1000];
        std::strncpy(message, texte + 50, 1000);
        printf("%s", message);
    }

    // Restaurer l'entrée après le signal
    restore_input();
}

void sigint_handler(int sig) {
    if (traitementSignal) {
        return;
    }
    traitementSignal = true;
    printf("\nPluie de moquerie\n");
    snprintf(texte, 50, "%d", pid);
    std::strncpy(texte + 50, "SIGINT", 1000);
    kill(pid_serveur, SIGUSR2);
    disable_input();
    while(signalServeur == false) {
        pause();
        char message[1000];
        std::strncpy(message, texte + 50, 1000);
        printf("%s", message);
    }

    // Restaurer l'entrée après le signal
    restore_input();
}

void sigquit_handler(int sig) {
    if (traitementSignal) {
        return;
    }
    traitementSignal = true;
    snprintf(texte, 50, "%d", pid);
    std::strncpy(texte + 50, "SIGQUIT", 1000);
    kill(pid_serveur, SIGUSR2);
    while(signalServeur == false) {
        pause();
        char message[1000];
        std::strncpy(message, texte + 50, 1000);
        printf("%s", message);
    }

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

    // Restaurer l'entrée après le signal
    restore_input();
    printf("\n");
}

void sigterm_handler(int sig) {
    if (traitementSignal) {
        return;
    }
    traitementSignal = true;
    snprintf(texte, 50, "%d", pid);
    std::strncpy(texte + 50, "SIGTERM", 1000);
    kill(pid_serveur, SIGUSR2);
    while(signalServeur == false) {
        pause();
        char message[1000];
        std::strncpy(message, texte + 50, 1000);
        printf("%s", message);
    }

    disable_input();
    std::cin.clear();  // Réinitialiser le flux d'entrée
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    // Le programme attend ici jusqu'à ce qu'il soit terminé ou qu'une action supplémentaire soit prise
    while (1) {
        // Bloquer l'exécution ici (le terminal est désactivé)
        pause();
    }
}

void sigcont_handler(int sig) {
    char pid[50];
    char message[1000];

    std::strncpy(pid, texte, 50);
    std::strncpy(message, texte + 50, 1000);

    // Conversion de pid en pid_t
    pid_t pid_message = static_cast<pid_t>(std::stoi(pid));  // Conversion de la chaîne en entier

    if (pid_message == pid_serveur) {
        std::string message_str(message); // convertir le tableau char en std::string
        if(message_str.substr(0, 10) == "pid_client"){
            if(traitementSignal){
                kill(pid_serveur, SIGCONT);
                return;
            }
            int pid_client_a_punir;
            char message_serveur[1000];

            sscanf(message, "pid_client:%dmessage:%[^\n]", &pid_client_a_punir, message_serveur);

            printf("\n");
            printf("%s", message_serveur);
            printf("\n");
            std::cout << "--> Tapez votre message méchant : ";
            fflush(stdout); // Forcer l'affichage
            std::string message_punition;
            std::getline(std::cin, message_punition);

            message_punition = trim(message_punition);
            snprintf(texte, 50, "%d", getpid());
            std::string msg = "De la part de " + nomPrenom + " : " + message_punition + "\n";
            std::strncpy(texte + 50, msg.c_str(), 1000);
            kill(pid_client_a_punir, SIGCONT);
            sleep(0.5);
            kill(pid_serveur, SIGCONT);
            ecrireMessage();
        } else {
            signalServeur = true;
        }
    }
}

int main() {
    pid = getpid();
	printf("pid_client = %d\n", pid);
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
    signal(SIGCONT, sigcont_handler);

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
    nomPrenom = prenom + " " + nom;

    std::strncpy(texte + 50, nomPrenom.c_str(), 1000);

    kill(pid_serveur, SIGUSR1);

    // Ignore the newline character left by std::cin >> nom;
    //std::cin.ignore();
    while(1) {
        ecrireMessage();
    }
}