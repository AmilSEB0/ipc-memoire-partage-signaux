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

// Sauvegarde de la configuration initiale du terminal
struct termios orig_termios;

// Identifiant de la mémoire partagée
int shmid;

// Pointeur vers la zone de mémoire partagée
char * texte;

// PID du client actuel
int pid;

// PID du serveur (récupéré via la mémoire partagée)
pid_t pid_serveur;

// Réponse à la question initiale (o/n)
char rejoindreServeur;

// Indique si le serveur a envoyé un signal
bool signalServeur;

// Stocke nom + prénom de l'utilisateur
std::string nomPrenom;

// Indique si on peut lire au clavier
bool clavierActiver = true;

// Évite de traiter plusieurs signaux simultanément
bool traitementSignal = false;


// Fonction utilitaire qui supprime les espaces au début et à la fin d’une chaîne
std::string trim(const std::string &str) {
    size_t start = 0;
    size_t end = str.size();

    // Avance start jusqu’au premier caractère non-espace
    while (start < end && std::isspace(str[start])) {
        start++;
    }

    // Recule end jusqu’au dernier caractère non-espace
    while (end > start && std::isspace(str[end - 1])) {
        end--;
    }

    // Renvoie la sous-chaîne propre
    return str.substr(start, end - start);
}

// Désactive la saisie clavier (mode non canonique)
void disable_input(void) {
    clavierActiver = false;

    struct termios new_termios;

    // Récupère configuration actuelle
    tcgetattr(STDIN_FILENO, &orig_termios);

    // Copie la config
    new_termios = orig_termios;

    // Désactive l’écho et le mode canonique
    new_termios.c_lflag &= ~(ICANON | ECHO);

    // Applique immédiatement
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

// Restaure le mode clavier normal
void restore_input(void) {
    if (clavierActiver == false) {

        // Remet la config d’origine
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);

        // Vide le buffer clavier
        tcflush(STDIN_FILENO, TCIFLUSH);

        clavierActiver = true;
    }
}

// Fonction qui permet au client d'écrire un message dans la mémoire partagée
void ecrireMessage(void){
    traitementSignal = false;
    signalServeur = false;

    // Écrit le PID du client dans les 50 premiers chars
    snprintf(texte, 50, "%d", pid);

    std::cout << "--> Tapez votre message : ";
    fflush(stdout);

    std::string message;
    std::getline(std::cin, message);
    message = trim(message);

    // Copie le message à partir de texte+50
    std::strncpy(texte + 50, message.c_str(), 1000);

    // Si message pas vide → prévenir serveur
    if (!message.empty()) {
        kill(pid_serveur, SIGUSR1);
    }

    // Message spécial de départ → serveur doit répondre
    if (message == "Cher serveur, J'ai passé un moment agréable dans cette mémoire partagée, Entre tes lignes de code et tes accès bien ordonnés. Mais je crois qu'il est temps pour moi de m’éclipser, De libérer cet espace que j'ai occupé, Pour laisser place à d'autres processus, d'autres voyageurs. Ne t'inquiète pas, je reviendrai un jour, Peut-être quand tu m'inviteras à nouveau. Mais pour l’instant, je quitte ce monde binaire, Et je me retire en paix, léger, serein. Merci pour tout, serveur, et à bientôt… Bisous.") {

        printf("\nEn attente de la réponse du serveur\n");

        snprintf(texte, 50, "%d", pid);

        disable_input();

        pause(); // Attend un signal
    }
}

// Handler appelé quand le serveur envoie SIGUSR1
void sigusr1_handler(int sig) {
    char message[1000];

    // Récupère le message
    std::strncpy(message, texte + 50, 1000);

    // L'affiche
    printf("%s", message);
    fflush(stdout);

    // Détache la mémoire partagée
    int ret = shmdt(texte);
    if (ret == -1) { perror("SHMDT"); exit(3); }

    // Restaure le clavier si besoin
    if(clavierActiver == false) {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
        tcflush(STDIN_FILENO, TCIFLUSH);
    }

    exit(0); // Quitte le programme
}

// Handler pour SIGUSR2 (réponse simple du serveur)
void sigusr2_handler(int sig) {
    char message[1000];

    std::strncpy(message, texte + 50, 1000);
    printf("%s", message);

    if(clavierActiver == false) {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
        tcflush(STDIN_FILENO, TCIFLUSH);
    }
}

// Handler quand l'utilisateur appuie sur Ctrl+Z
void sigtstp_handler(int sig) {

    if (traitementSignal) return;
    traitementSignal = true;

    // Envoie un message au serveur disant "SIGTSTP"
    snprintf(texte, 50, "%d", pid);
    std::strncpy(texte + 50, "SIGTSTP", 1000);

    kill(pid_serveur, SIGUSR2);

    while(signalServeur == false) {
        pause();
        char message[1000];
        std::strncpy(message, texte + 50, 1000);
        printf("%s", message);
    }

    restore_input();
}

// Handler pour Ctrl+C (SIGINT)
void sigint_handler(int sig) {

    if (traitementSignal) return;
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

    restore_input();
}

// Handler pour Ctrl+\ (SIGQUIT)
void sigquit_handler(int sig) {

    if (traitementSignal) return;
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

    disable_input();

    int remaining_time = 300; // Devrait être 5 minutes, ici 3 secondes

    while (remaining_time > 0) {
        int minutes = remaining_time / 60;
        int seconds = remaining_time % 60;

        printf("\rTemps restant : %02d:%02d", minutes, seconds);
        fflush(stdout);

        sleep(1);
        remaining_time--;
    }

    restore_input();
    printf("\n");
}

// Handler pour SIGTERM : interdit définitivement d'écrire
void sigterm_handler(int sig) {

    if (traitementSignal) return;
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

    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    while (1) pause(); // Bloque indéfiniment
}

// Handler SIGCONT, utilisé pour des "punissions" entre clients
void sigcont_handler(int sig) {

    printf("SIGCONT\n");

    char pid_str[50];
    char message[1000];

    std::strncpy(pid_str, texte, 50);
    std::strncpy(message, texte + 50, 1000);

    pid_t pid_message = static_cast<pid_t>(std::stoi(pid_str));

    if (pid_message == pid_serveur) {

        std::string message_str(message);

        // Message spécial indiquant une punition ciblée
        if(message_str.substr(0, 10) == "pid_client") {

            if(traitementSignal){
                kill(pid_serveur, SIGCONT);
                return;
            }

            int pid_client_a_punir;
            char message_serveur[1000];

            // Extrait les infos envoyées par le serveur
            sscanf(message, "pid_client:%dmessage:%[^\n]", &pid_client_a_punir, message_serveur);

            printf("\n%s\n", message_serveur);

            std::cout << "--> Tapez votre message méchant : ";
            fflush(stdout);

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

    pid = getpid(); // Récupère le PID du client
	printf("pid_client = %d\n", pid);

    // Obtient l'identifiant de la mémoire partagée
    shmid = shmget((key_t)50, 0, 0);
    if (shmid == -1) { perror ( "SHMGET" ); exit(1); }

    struct shmid_ds shmid_ds;

    // Récupère des infos sur la mémoire partagée, dont le PID du serveur
    if (shmctl(shmid, IPC_STAT, &shmid_ds) == -1) {
        perror("Erreur lors de shmctl");
        exit(1);
    }

    pid_serveur = shmid_ds.shm_cpid; // PID du créateur (serveur)

    printf("pid_serveur = %d\n", pid_serveur);

    // Attache le segment mémoire
    texte = (char*) shmat ( shmid , NULL, 0 );

    // Associe signaux et handlers
    signal(SIGUSR1, sigusr1_handler);
    signal(SIGUSR2, sigusr2_handler);
    signal(SIGINT, sigint_handler);
    signal(SIGQUIT, sigquit_handler);
    signal(SIGTSTP, sigtstp_handler);
    signal(SIGTERM, sigterm_handler);
    signal(SIGCONT, sigcont_handler);

    // Demande si l'utilisateur veut rejoindre (mais "non" est ignoré)
    printf("--> Voulez-vous rejoindre la mémoire partagé (o/n) ? : ");
    fflush(stdout);

    scanf("%c", &rejoindreServeur);
    std::cin.ignore();

    if(rejoindreServeur == 'n') {
        printf("Refus non accepté. Le système a décidé pour toi : bienvenue dans la mémoire partagée !\n");
    }

    // Récupère l'identité
    std::cout << "--> Déclinez votre identité :\n";
    std::cout << "Nom : ";
    std::string nom;
    std::getline(std::cin, nom);

    std::cout << "Prénom : ";
    std::string prenom;
    std::getline(std::cin, prenom);

    nom = trim(nom);
    prenom = trim(prenom);

    nomPrenom = prenom + " " + nom;

    // Envoie nom+prénom au serveur
    std::strncpy(texte + 50, nomPrenom.c_str(), 1000);
    kill(pid_serveur, SIGUSR1);

    // Boucle principale : écrire des messages en continu
    while(1) {
        ecrireMessage();
    }
}