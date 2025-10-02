# Tema 2 - Task Planner with Java Threads

In aceasta tema am implementat un sistem de planificare a task-urilor,
folosind Java Threads. Sistemul are 2 componente principale - dispatcher si noduri
de executie - a caror functionalitate am implementat-o in urmatoarele 2 clase:

-- 1. Clasa MyDispatcher --
(extinde clasa abstracta Dispatcher)
Dispatcher-ul are rolul de a primi task-uri de la thread-urile care le citesc din fisiere
(clasa TaskGenerator) si de a le asigna unui nod/host, in functie de un algoritm de
planificare (cu care a fost initializat dispatcher-ul).
Aceasta functionalitate este implementata in metoda 'addTask(..)', sincronizata pentru a
nu exista probleme de concurenta la primirea task-urilor din thread-urile de citire.
Astfel, doar un thread poate adauga un task in dispatcher (prin apelul 'addTask(..)')
la un moment dat.

-- metoda addTask(..) --
Algoritmul Round Robin
Fiecare host primeste, pe rand, cate un task, in ordinea in care acestea sunt primite
in dispatcher. Folosesc un counter 'rrCounter' pentru a determina al carui host este
"randul" sa primeasca un task.

Algoritmul Shortest Queue
Host-ul cu cel mai mic queue de taskuri in asteptare (+1 pt task-ul care ruleaza in acel
moment) va primi task-ul curent. Dimensiunea queue-ului este calculata prin apelul
metodei 'getQueueSize()' a fiecarui host.

Algoritmul SITA
Host 0 va primi taskurile 'SHORT', Host 1 - taskurile 'MEDIUM' si Host 2 - cele 'LONG'.

Algoritmul Least Work Left
Asemenea alg. SQ, cu mentiunea ca se va calcula cantitatea de "munca" ramasa de executat
in cadrul fiecarui nod (exprimata in secunde), cu ajutorul metodei 'getWorkLeft()'.


-- 2. Clasa MyHost --
(extinde clasa abstracta Host, care la randul sau extinde clasa Thread)
Host-ul (sau nodul de executie) are rolul de a primi task-uri de la dispatcher si de a le
executa. Task-urile primite cand nodul este idle isi incep executia in momentul in care
sunt primite, pe cand nodurile care "se strang" in queue vor fi executate in functie de
prioritate. De asemenea, task-urile care sunt preemptable vor fi oprite din executie daca
au fost adaugate in queue task-uri mai prioritare, urmand sa isi reia executia imediat
dupa terminarea acestor task-uri.

-- metoda run() --
Implementeaza logica de baza a thread-ului: primirea unui task si executia acestuia.
Metoda run() este sincronizata prin 'queueLock' cu metoda addTask(), apelata de
dispatcher; astfel, cat timp coada de taskuri este goala, thread-ul va astepta,
iar cand se adauga un task in aceasta, thread-ul este notificat; ca urmare, acesta
va extrage un task din coasa si il va executa (apel metoda 'executeTask(..)').

-- metoda executeTask(..) --
Implementeaza executia propriu-zisa a task-ului, pentru durata de timp corespunzatoare.
Executia este simulata prin apelul 'Thread.sleep(100)', nefiind un task propriu-zis de
implementat. Tot aici se verifica daca este necesara si posibila intreruperea task-ului
curent pentru a se executa alte task-uri mai prioritare.

-- metoda addTask(..) --
Implementeaza adaugarea task-ului primit in queue-ul de task-uri al host-ului.
Verific 3 cazuri:
- queue e gol => adaug task-ul
- queue nu e gol & task-ul curent (pe care trebuie sa il adaug) nu a fost pre-emptat =>
adaug task-ul pe pozitia corespunzatoare prioritatii sale, dupa task-urile de
aceeasi prioritate (care au sosit in queue inaintea task-ului curent)
- queue nu e gol & task-ul curent a fost pre-emptat => adaug pe pozitia corespunzatoare
prioritatii, dar inainte de task-urile cu aceeasi prio ca task-ul curent (care au sosit
inainte ca task-ul curent sa fie pre-emptat, adica in timpul executiei sale)

-- metoda getQueueSize() --
Returneaza dimensiunea queue-ului + 1 daca am un task running in acel moment.

-- metoda getWorkLeft() --
Insumeaza milisecundele necesare executarii tuturor task-urilor ramase (queue tasks +
running task) si aproximeaza rezultatul prin transformarea acestuia in secunde.

-- metoda shutdown() --
Seteaza variabila booleana 'isRunning' pe false, ceea ce va opri metoda run()
a thread-ului; de asemenea notifica metodele, care se pot afla in asteptare de task-uri,
prin intermediul 'queueLock'.
