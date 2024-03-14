#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sqlite3.h>
#include <fcntl.h>
#include "database.c"

#define PORT 5006
#define CLIENTI 5
#define MAX_CLIENTS 100
#define MSG1 "Nu exista castigatori!\0"
#define WINNER_MSG "Locul intai merge catre: "
#define FINAL "Jocul s-a terminat!"
#define REGULI " Jocul va incepe in scurt timp. Pentru fiecare intrebare vei avea un timp de 10 secunde in care sa raspunzi, altfel vei fi eliminat. Raspunsurile trebuie sa fie de forma: 1|2|3|4. Succes!"

const char *ADMIN = "Admin";
const char *PLAYER = "Player";
int QUESTIONS_NR;

extern int errno;

//structuri clienti
typedef struct {
    int socket;
    struct sockaddr_in address;
    char* nume;
    int scor;
    int stop;  //cand se deconecteaza player-ul stop ia valoarea 1
} thread_client;

typedef struct {
    int socket;
    struct sockaddr_in address;
    char* nume;
} thread_admin;

//variabile globale
int server_socket;  
int opt = 1; //pt SO_REUSEADDR
struct sockaddr_in server_address;
int incepe_joc;  //sincronizeaza inceputul rundei
int count_clients; //nr de clienti conectati
int count_admins;  //nr de admini conectati
thread_client* clients[MAX_CLIENTS];  //clientii conectati in runda
int admin_is_done;  //cand se deconecteaza adminul se inchide si serverul


//calculeaza scorul fiecarui jucator
int calcScor(thread_client *client, int punctaj)
{	
	client->scor += punctaj;
	return client->scor;
}

//functie pentru trimiterea de mesaje catre clienti
void trimite_mesaj(char *text, int socket){
    char *mesaj = (char*)malloc(256);
    strcpy(mesaj, text);

    if (write(socket, mesaj, strlen(mesaj) + 1) == -1) {
        perror("Error writing mesaj to client.\n");
    }
    free(mesaj);
}

//afiseaza castigatorul/castigatorii
char* cine_a_castigat()
{	
	char* castigator = (char*)malloc(30 + 20 * count_clients);
	int max = 0;
    int exista_castigator = 0;

	for(int i = 0; i < count_clients; i++)
	{
        if(!clients[i]->stop){
            if(clients[i]->scor > max)
            {
                exista_castigator = 1;
                max = clients[i]->scor;
                strcpy(castigator, WINNER_MSG);
                strcat(castigator, clients[i]->nume);
            }
            else if(clients[i]->scor == max && max > 0)
            {
                strcat(castigator, ", ");
                strcat(castigator, clients[i]->nume);
                if(i == (count_clients - 1)){  //ultimul client din lista
                    strcat(castigator, "!");
                }
            }
        }
    }
	
    if(exista_castigator){

	    return castigator;
    } else{

        free(castigator);
        return NULL;
    }

}

//serverul asteapta raspunsurile de la clienti
void receiveAnswer(thread_client *client, int question_id) {
    fd_set read_fds;
    struct timeval timeout;
    int result, answer;

    FD_ZERO(&read_fds);
    FD_SET(client->socket, &read_fds);

    timeout.tv_sec = 10;  
    timeout.tv_usec = 0;
    result = select(client->socket + 1, &read_fds, NULL, NULL, &timeout);
    //eroare la select
    if (result < 0) {
        perror("Error in select.\n");
        pthread_exit(NULL);
    //nu a raspuns la timp
    } else if (result == 0) {
        printf("%s did not respond in time for question %d. Disconnecting...\n", client->nume, question_id);
        close(client->socket);
        client->stop = 1;
        pthread_exit(NULL);

    } else {
        int read_res = read(client->socket, &answer, sizeof(int));
        if (read_res <= 0) {
            //clientul s-a deconectat
            printf("%s was disconnected or an error occurred!\n", client->nume);
            close(client->socket);
            client->stop = 1;
            pthread_exit(NULL);
        } else {
            //clientul a raspuns la timp
            int rightAnswer = getRightAnswer(question_id); 
            if(answer != 1 && answer != 2 && answer != 3 && answer != 4){
                printf("%s sent an invalid answer for question %d\n", client->nume, question_id);
                char *feedback = "Raspuns invalid!";
                trimite_mesaj(feedback, client->socket);
            }else if (answer == rightAnswer) {
                calcScor(client, 5);  
                printf("%s answered correctly for question %d\n", client->nume, question_id);
                char *feedback = "Raspuns corect!";
                trimite_mesaj(feedback, client->socket);
            } else {
                printf("%s answered incorrectly for question %d\n", client->nume, question_id);
                char *feedback = "Raspuns gresit!";
                trimite_mesaj(feedback, client->socket);
            }
        }
    }
}

void sendQuestionsToClient(thread_client *client) {

    if(!client->stop){

        //trimitem nr total de intrebari
        printf("%d\n", QUESTIONS_NR);
        if (write(client->socket, &QUESTIONS_NR, sizeof(int)) == -1) {
            perror("Error writing QUESTIONS_NR to client.\n");
        }
    }else{
        close(client->socket);
        pthread_exit(NULL);
    }
    for (int question_id = 1; question_id <= QUESTIONS_NR; ++question_id) {

        if(!client->stop){        
            char *question = getQuestion(question_id);  
            if(question == NULL || strlen(question) == 0) {
                printf("Question %d not found.\n", question_id);
                continue; 
            }

            printf("Sending Question %d: %s\n", question_id, question);
    
            if(!client->stop){
                if (write(client->socket, &question_id, sizeof(int)) == -1) {
                    perror("Error writing question ID to client.\n");
                }

                if (write(client->socket, question, strlen(question) + 1) == -1) {
                    perror("Error writing question to client.\n");
                }
            }
        
                            
            free(question); 

            time_t start, end;  
            double elapsed; 
            time(&start);

            if(client->stop){
                close(client->socket);
                pthread_exit(NULL);
                break;
            }
            receiveAnswer(client, question_id);
            time(&end);  
            elapsed = difftime(start+10, end);  
            sleep(elapsed);
    
        }else{
            close(client->socket);
            pthread_exit(NULL);
            break;
        }       
    
    }
}

void *threadRoutine(void *arg) {

    //setup player
    thread_client *th_arg = (thread_client *)arg;
    th_arg->scor = 0;
    pthread_detach(pthread_self());
    clients[count_clients - 1] = th_arg;

    //trimit semnal la client
    char *type = "PLAYER";
    trimite_mesaj(type, th_arg->socket);

    //autentificare 
    th_arg->nume = (char*)malloc(20);
    memset(th_arg->nume, 0, 20);

    if (read(th_arg->socket, th_arg->nume, 20) == -1) {
        perror("Failed to read from client.\n");
    }
    printf("%s joined the game!\n", th_arg->nume);
    char* welcome = (char*)malloc(256);
    strcpy(welcome, "Bine ai venit, ");
    strcat(welcome, th_arg->nume);
    strcat(welcome, "!");
    strcat(welcome, REGULI);
    trimite_mesaj(welcome, th_arg->socket);

    //asteapta sa inceapa runda
    while (!incepe_joc) {
               
        sleep(1);
    }    

    //se trimit intrebari, raspunsuri
    sendQuestionsToClient(th_arg); 

    //se anunta castigatorul
    char *message = cine_a_castigat();  
    if(message == NULL) {   //nu exista castigator
        trimite_mesaj(MSG1, th_arg->socket);
    }else{                  //exista 1 sau mai multi castigatori
        trimite_mesaj(message, th_arg->socket);             
        free(message); 
    }

    //Game over 

    // Inchide conexiunea 
    close(th_arg->socket);
    free(th_arg);
    pthread_exit(NULL);
} 
    

void *threadRoutine_admin(void *arg) {

    //setup admin
    thread_admin *th_adm = (thread_admin *)arg;

    //trimit semnal clientului

    char *type = "ADMIN";
    trimite_mesaj(type, th_adm->socket);

    //autentificare admin 
    th_adm->nume = (char*)malloc(20);
    memset(th_adm->nume, 0, 20);
    if (read(th_adm->socket, th_adm->nume, 20) == -1) {
        perror("Failed to read from client.\n");
    }
    printf("%s s-a logat ca admin!\n", th_adm->nume);
    char* welcome = (char*)malloc(100);
    strcpy(welcome, "Bine ai venit, admin ");
    strcat(welcome, th_adm->nume);
    strcat(welcome, "!");
    trimite_mesaj(welcome, th_adm->socket);

    bool add_more = true;
    while (add_more) {
    //alege daca mai adauga o intrebare
        int decision;
        if (read(th_adm->socket, &decision, sizeof(int)) == -1) {
            perror("Failed to read decision.\n");
        }
        if(decision == 0){
            add_more = 0;
            break;
        }
        char *new_question = (char*)malloc(MAX_QUESTION_SIZE);
        if (read(th_adm->socket, new_question, MAX_QUESTION_SIZE) == -1) {
            perror("Failed to read decision.\n");
        }
        if(add_question(new_question) == 0){  
            char *feedback = "Intrebare adaugata cu succes!";
            printf("%s\n", feedback);
            trimite_mesaj(feedback, th_adm->socket);
            free(new_question);
        }else{
            char *feedback = "Eroare la adaugarea unei noi intrebari in baza de date!";
            printf("%s\n", feedback);
            trimite_mesaj(feedback, th_adm->socket);
            free(new_question);

        }
    }
    admin_is_done = 1;
    close(th_adm->socket);
    pthread_exit(NULL);

}


void connect_player() 
{
    thread_client *thrd;
    int sock;
    thrd = (thread_client *)malloc(sizeof(thread_client));
    if (thrd == NULL) 
    {
        perror("Eroare la alocarea memoriei.\n");
        exit(EXIT_FAILURE);
    }
    int size = sizeof(thrd->address);
    sock = accept(server_socket, (struct sockaddr *)&thrd->address, &size);
    thrd->socket = sock;
    if (thrd->socket < 0) 
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {

            return;
        }        
        perror("Eroare la accept.\n");
        exit(EXIT_FAILURE);
    }
    else if (incepe_joc) {
        close(thrd->socket);
        exit(EXIT_FAILURE);
        return;
    }    
    count_clients++;
    
    pthread_t thread;
    if (pthread_create(&thread, NULL, threadRoutine, thrd) != 0) 
    {
        perror("Eroare la crearea thread-ului\n");
        exit(EXIT_FAILURE);
    }
}

void connect_admin() 
{
    thread_admin *thrda;
    int sock;
    thrda = (thread_admin *)malloc(sizeof(thread_admin));
    if (thrda == NULL) 
    {
        perror("Eroare la alocarea memoriei.\n");
        exit(EXIT_FAILURE);
    }
    int size = sizeof(thrda->address);
    sock = accept(server_socket, (struct sockaddr *)&thrda->address, &size);
    thrda->socket = sock;
    if (thrda->socket < 0) 
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return;
        }        
        perror("Eroare la accept.\n");
        exit(EXIT_FAILURE);
    }   
    count_admins = 1;
    pthread_t threada;
    if (pthread_create(&threada, NULL, threadRoutine_admin, thrda) != 0) 
    {
        perror("Error creating thread\n");
        exit(EXIT_FAILURE);
    }
}


int main() {

    create_DB();
    
//server setup
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;
    
//creez socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error creating socket.\n");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Error binding socket.\n");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, CLIENTI) == -1) {
        perror("Error listening on socket.\n");
        exit(EXIT_FAILURE);
    }

    int current_flags = fcntl(server_socket, F_GETFL);
	fcntl(server_socket, F_SETFL, current_flags | O_NONBLOCK);  //socket nonblocking

    
    time_t start, end;  
    double elapsed; 
    time(&start);
    int conectare = 1;

    while (conectare) 

    {
        connect_player();
    

        time(&end);  
        elapsed = difftime(end, start);  
        if (elapsed >= 20.0) {  
            conectare = 0;
            incepe_joc = 1;
        }
    }    

        QUESTIONS_NR = getQuestionsNr();
        for(int q = 1; q <= QUESTIONS_NR; q++){
        sleep(10);
    }
    sleep(10);

    printf("Game over!\n");  

    sleep(1);
    time_t start2, end2;  
    double elapsed2; 
    time(&start2);
    int conectare_admins = 1;

    printf("Se asteapta conexiune de la admin..\n");
    while (count_admins < 1 && conectare_admins == 1) 
    {
        connect_admin();

        time(&end2);  
        elapsed2 = difftime(end2, start2);  
        if (elapsed2 >= 20.0) {  
            conectare_admins = 0;
        }
    
    }  

    if(count_admins == 1){

        while(!admin_is_done){
            sleep(1);
        }

        printf("Adminul s-a deconectat!\n");
    }

    sleep(3);

    printf("Serverul se inchide.\n");


    
    return 0;

}