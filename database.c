#include <stdio.h>
#include <sqlite3.h>
#include<stdlib.h>
#include<string.h>

#define MAX_QUESTION_SIZE 256


sqlite3 *db;

// functie pentru crearea bazei de date & tabelelor
void create_DB() 
{
    sqlite3 *db;
    char *errMsg = 0;
    
    int rc = sqlite3_open("quizz.db", &db);
    
    if (rc) 
    {
        fprintf(stderr, "Nu se poate deschide sau crea baza de date: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    } 
    else 
    {
        printf("Baza de date a fost deschisă/creată cu succes.\n");
    }
    
    //cream tabela Questions
    char *sql_questions = "CREATE TABLE IF NOT EXISTS Questions ("
                          "id INTEGER PRIMARY KEY, "
                          "Question TEXT, "
                          "Answer1 TEXT, "
                          "Answer2 TEXT, "
                          "Answer3 TEXT, "
                          "Answer4 TEXT);"
                          "INSERT INTO Questions (id, Question, Answer1, Answer2, Answer3, Answer4) "
                          "VALUES (1, 'Q1', 'Q11', 'Q12', 'Q13', 'Q14') "
                          "ON CONFLICT(id) DO NOTHING;"
                          "INSERT INTO Questions (id, Question, Answer1, Answer2, Answer3, Answer4) "
                          "VALUES (2, 'Q2', 'Q21', 'Q22', 'Q23', 'Q24') "
                          "ON CONFLICT(id) DO NOTHING;";
    
    rc = sqlite3_exec(db, sql_questions, 0, 0, &errMsg);
    
    if (rc != SQLITE_OK) 
    {
        fprintf(stderr, "Eroare la crearea tabelei 'Questions': %s\n", errMsg);
        sqlite3_free(errMsg);
    } 
    else 
    {
        printf("Tabela 'Questions' a fost creata cu succes.\n");
    }

    //cream tabela Questions
    
    char *sql_answers = "CREATE TABLE IF NOT EXISTS Answers ("
                        "id INTEGER PRIMARY KEY, "
                        "CorrectAnswer INTEGER);"
                        "INSERT INTO Answers (id, CorrectAnswer) VALUES (1, 2) "
                        "ON CONFLICT(id) DO NOTHING;"
                        "INSERT INTO Answers (id, CorrectAnswer) VALUES (2, 4) "
                        "ON CONFLICT(id) DO NOTHING;";
                       
    rc = sqlite3_exec(db, sql_answers, 0, 0, &errMsg);
    
    if (rc != SQLITE_OK) 
    {
          fprintf(stderr, "Eroare la crearea tabelei 'Answers': %s\n", errMsg);
        sqlite3_free(errMsg);
    } 
    else 
    {
        printf("Tabela 'Answers' a fost creata cu succes.\n");
    }
        
    sqlite3_close(db);
}
       


//extrage cate o intrebare din bd
char *getQuestion(int id) 
{
    char *err_msg = 0;
    sqlite3_stmt *stmt;

    int rc = sqlite3_open("quizz.db", &db);

    if (rc != SQLITE_OK) 
    {
        printf("Database opening error\n");
        sqlite3_close(db);
        return NULL;
    }

    char *sql_comm = "SELECT Question, Answer1, Answer2, Answer3, Answer4 FROM Questions WHERE id = ?";
    rc = sqlite3_prepare_v2(db, sql_comm, -1, &stmt, 0);

    if (rc == SQLITE_OK) 
    {
        sqlite3_bind_int(stmt, 1, id);
    } else 
    {
        printf("\n Error fetching data from database.\n");
        sqlite3_close(db);
        return NULL;
    }

    int step = sqlite3_step(stmt);

    char *question = NULL;

    if (step == SQLITE_ROW) 
    {
        // aloc memorie pt intrebare
        question = (char *)malloc(MAX_QUESTION_SIZE);
        if (question == NULL) {
            fprintf(stderr, "Eroare la alocarea memoriei pentru întrebare.\n");
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return NULL;
        }

        // copiez intrebarea in sirul alocat
        snprintf(question, MAX_QUESTION_SIZE, "%s: %s | %s | %s | %s", sqlite3_column_text(stmt, 0),
                     sqlite3_column_text(stmt, 1),
                     sqlite3_column_text(stmt, 2),
                     sqlite3_column_text(stmt, 3),
                     sqlite3_column_text(stmt, 4));
    } else 
    {
        printf("Nu a fost gasita intrebare pentru ID-ul dat.\n");
    }

    // inchid bd
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return question;
}

//extrage raspunsul corect al unei intrebari 
int getRightAnswer(int id) 
{
    char *err_msg = 0;
    sqlite3_stmt *stmt;
    int result = 0;  

    int rc = sqlite3_open("quizz.db", &db);

    if (rc != SQLITE_OK) 
    {
        printf("Eroare la deschiderea bazei de date.\n");
        sqlite3_close(db);
        result = -1;  
    }

    char *sql_comm = "SELECT CorrectAnswer FROM Answers WHERE id = ?";
    rc = sqlite3_prepare_v2(db, sql_comm, -1, &stmt, 0);

    if (rc == SQLITE_OK) 
    {
        sqlite3_bind_int(stmt, 1, id);
    } else 
    {
        printf("Eroare la pregatirea interogarii.\n");
        sqlite3_close(db);
        result = -1;  
    }

    int step = sqlite3_step(stmt);

    if (step == SQLITE_ROW) 
    {
        result = atoi((const char*)sqlite3_column_text(stmt, 0));
    } else 
    {
        printf("Nu s-a gasit raspuns pentru ID-ul dat.\n");
        result = -2;  
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return result;  
}

//nr total de intrebari
int getQuestionsNr() {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int row_count = -1;
    int rc;

    rc = sqlite3_open("quizz.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM questions", -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        row_count = sqlite3_column_int(stmt, 0);
    } else {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return row_count;
}

//adauga intrebari in bd
int add_question(const char *Q) {

    char *dupeQ = strdup(Q); 
    char **result = malloc(6 * sizeof(char *)); //avem 6 subsiruri 
    int i = 0;

    // impartim dupa ":"
    char *token = strtok(dupeQ, ":");
    result[i++] = strdup(token);

    // impartim dupa "|"
    token = strtok(NULL, "|");
    while (token != NULL && i < 6) {
        result[i++] = strdup(token);
        token = strtok(NULL, "|");
    }

    free(dupeQ);

    char *err_msg = 0;
    int rc;
    sqlite3_stmt *stmt;

    // deschide bd
    rc = sqlite3_open("quizz.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    char *sql_questions = "INSERT INTO Questions (Question, Answer1, Answer2, Answer3, Answer4) VALUES (?, ?, ?, ?, ?);";

    rc = sqlite3_prepare_v2(db, sql_questions, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }


    sqlite3_bind_text(stmt, 1, result[0], -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, result[1], -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, result[2], -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, result[3], -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, result[4], -1, SQLITE_STATIC);
 
    // executa stmt pt Questions
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return -1;
    }
    sqlite3_finalize(stmt);

    // ultimul ID inserat
    int new_questionid = sqlite3_last_insert_rowid(db);

    char *sql_answers = "INSERT INTO Answers (id, CorrectAnswer) VALUES (?, ?);";

    rc = sqlite3_prepare_v2(db, sql_answers, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    //convertesc result[5] adica right answer in int
    int correctAnswer = atoi(result[5]);

    sqlite3_bind_int(stmt, 1, new_questionid);
    sqlite3_bind_int(stmt, 2, correctAnswer);

    // executa stmt pt Answers
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return -1;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 0; // Success
}



	
	
	
	
	
	
	
	
	
	


