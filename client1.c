#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <regex.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define MAX_QUESTION_SIZE 256
#define MAX_RESPONSE_SIZE 256
#define MESSAGE_LENGTH 256

extern int errno;


int client_socket;
char* nume;


int QUESTIONS_NR;
struct sockaddr_in server_address;
const char *admin = "ADMIN";

char* primeste_mesaj() {
    int read_res;
    char* mesaj = malloc(MESSAGE_LENGTH * sizeof(char));
    if (mesaj == NULL) {
        perror("Error allocating memory.\n");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    read_res = read(client_socket, mesaj, MESSAGE_LENGTH);
    if (read_res < 0) {
        perror("Error reading mesaj from server.\n");
        close(client_socket);
        free(mesaj);
        exit(EXIT_FAILURE);
    } else if (read_res == 0) {
        printf("Server has disconnected before sending the message.\n");
        close(client_socket);
        free(mesaj);
        exit(EXIT_FAILURE);
    }
    return mesaj;
}


void connect_client() {
    
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
        perror("Eroare la crearea socketului.\n");
    }

    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) 
    {
        perror("Eroare la connect.\n");
    }    
}

void sendAnswer(int question_id) 
{
    int answer;
    printf("Introduceti raspunsul pentru intrebarea %d: ", question_id);
    scanf("%d", &answer);
    if(answer == 0){
    	printf("You quit the game!\n");
    	close(client_socket);
    	exit(0);
    }
    
    if (write(client_socket, &answer, sizeof(int)) == -1) {
        perror("Error writing to server.\n");
    }
}

void receive_questions_from_server(int socket) {
    int question_id;
    char question[MAX_QUESTION_SIZE];
    int read_res;
    
    read_res = read(socket, &QUESTIONS_NR, sizeof(int));
    if (read_res < 0) {
        perror("Error reading QUESTIONS_NR from server.\n");
        close(socket);
        exit(EXIT_FAILURE);
    } else if (read_res == 0) {
        printf("Ai fost deconectat de la server\n");
        close(socket);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < QUESTIONS_NR; i++) { 
        read_res = read(socket, &question_id, sizeof(int));
        if (read_res < 0) {
            perror("Error reading question id from server.\n");
            close(socket);
            exit(EXIT_FAILURE);
        } else if (read_res == 0) {
            printf("Server has disconnected.\n");
            close(socket);
            exit(EXIT_FAILURE);
        }

        read_res = read(socket, question, MAX_QUESTION_SIZE);
        if (read_res < 0) {
            perror("Error reading question from server.\n");
            close(socket);
            exit(EXIT_FAILURE);
        } else if (read_res == 0) {
            // serverul s-a deconectat inainte sa trimita intrebarea
            printf("Server has disconnected before sending the question.\n");
            close(socket);
            exit(EXIT_FAILURE);
        }

        printf("Question %d: %s\n", question_id, question);
        sendAnswer(question_id); 
        char *feedback = primeste_mesaj();  
        printf("%s\n", feedback);
    
    } 
}


void interactAsAdmin() {
    //autentificare admin
        nume = (char*)malloc(20);
        memset(nume, 0, 20);
        printf("Nume: ");
        scanf("%s", nume);
        if (write(client_socket, nume, 20) == -1) {
            perror("Error writing name to the server.\n");
        }
        char* welcome_mesaj = primeste_mesaj();
        printf("%s\n", welcome_mesaj);
        free(welcome_mesaj);
        bool add_more = true;
        //adauga intrebari
        while (add_more) {
            printf("Doresti sa adaugi o intrebare noua? (1 - da, 0 - nu): ");
            int decision;
            scanf("%d", &decision);
            if (write(client_socket, &decision, sizeof(int)) == -1) {
                perror("Eroare la scrierea deciziei.\n");
                break;
            }
            if (decision == 0) {
                add_more = false;
            } else {
                char *new_question = (char*)malloc(MAX_QUESTION_SIZE);
                printf("Introduceti intrebarea de forma Q:A1|A2|A3|A4|CorrectA: ");
                scanf(" %s", new_question); // Citeste pana la new line
                if (write(client_socket, new_question, MAX_QUESTION_SIZE) == -1) {
                    perror("Eroare la scrierea intrebarii.\n");
                }
                free(new_question);
                char *feedback = primeste_mesaj();
                printf("%s\n", feedback);
            }
        }
        printf("Vei fi deconectat in scurt timp!\n");
        sleep(2);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Eroare, lipseste adresa sau portul.\n");
        return -1;
    }
    const char *addr = argv[1];
    int port = atoi(argv[2]);
    //setup
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(addr);
    server_address.sin_port = htons(port);
    
    connect_client();

    char *type = primeste_mesaj();
    if (strcmp(type, admin) == 0) {
        interactAsAdmin();
    }else {
    //autentificare player
        nume = (char*)malloc(20);
        memset(nume, 0, 20);
        printf("Nume: ");
        scanf("%s", nume);
        if (write(client_socket, nume, 20) == -1) {
            perror("Eroare la scrierea numelui.\n");
        }
        char* welcome_mesaj = primeste_mesaj();
        printf("%s\n\n", welcome_mesaj);
        free(welcome_mesaj);

        receive_questions_from_server(client_socket);
    
    //cine a castigat
    char* castigator = primeste_mesaj();
    printf("%s\n", castigator);
    free(castigator);

    sleep(2);
    //Game over
    printf("Jocul s-a terminat!\n");

    //deconectare
    close(client_socket);
    exit(EXIT_FAILURE);
    }
    free(type);
    free(nume);
            

    return 0;
}
