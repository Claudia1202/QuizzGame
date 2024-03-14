# QuizzGame

   Proiectul QuizzGame reprezinta un joc implementat sub forma unei aplicatii
formate dintr-un server multithreading la care se pot conecta oricati clienti.
Serverul va creea cate un thread pentru fiecare client care se conecteaza la acesta,
pentru a facilita desfasurarea simultana si sincronizata a jocului pentru acestia.
Comunicarea intre server si client se face prin socket-uri, toata logica este realizata
in server, clientul doar raspunde la intrebari.

Serverul este conectat la o baza de date SQLite care stocheaza intrebarile si
variantele de raspuns, precum si raspunsurile corecte pentru fiecare intrebare.

Serverul pune la dispozitie n secunde in care se vor conecta oricati clienti, si
apoi da start rundei. Clientii conectati vor primi intrebarile in acelasi timp, vor
avea 10 secunde la dispozitie sa raspunda la fiecare intrebare. Daca unul dintre 
clienti nu raspunde la o intrebare in intervalul de 10 secunde, acesta este eliminat 
din competitie de catre server. La final, serverul va compara scorurile si va anunta
castigatorul/castigatorii.

 
