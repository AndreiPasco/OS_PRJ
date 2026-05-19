# AI Usage Documentation - Phases 1 & 2

## Phase 1: Filtering Logic & Code Structuring

**Cum am folosit AI-ul:** L-am întrebat cum pot să filtrez eficient datele dintr-un fișier binar (`reports.dat`) în funcție de ce introduce utilizatorul în terminal, fără să scriu zeci de condiții `if` complicate. Totodată, am folosit AI-ul pentru a mă ajuta să îmi reorganizez codul.

**Ce am învățat / De ce l-am folosit:**
 1. AI-ul mi-a sugerat să citesc fișierul secvențial cu `while(read(...))` și să folosesc funcții clasice din C precum `strcmp` (pentru a compara texte) și `atoi` (pentru a transforma inputul din terminal în număr și a-l compara cu severitatea). 

2. **Modularizarea:** Pentru că funcția `main` devenise imensă, AI-ul m-a ghidat cum să sparg logica în funcții separate (ex: `action_add`, `action_view`, etc.). Acest lucru a făcut codul mult mai curat, ușor de citit și pregătit pentru Faza 2.

## Phase 2: Process Signals & Forking

**Cum am folosit AI-ul:** Aici am folosit AI-ul ca pe un asistent de studiu pentru a înțelege exact cum comunică două programe separate (procese) și cum se folosesc corect semnalele în Linux.

**Ce am învățat / De ce l-am folosit:**
1. Nu știam exact sintaxa pentru `sigaction()` (deoarece cerința interzicea folosirea vechiului `signal()`). AI-ul mi-a dat un șablon curat, m-a ajutat să repar o eroare de compilare adăugând `#define _XOPEN_SOURCE 700` și m-a învățat un detaliu tehnic important: în funcțiile care prind semnale (handlere) e mult mai sigur să folosesc `write()` în loc de `printf()` pentru a afișa mesaje, evitând astfel posibile blocaje.

2. M-a ajutat să structurez logica de notificare a monitorului: m-a învățat să citesc întâi PID-ul din fișierul ascuns `.monitor_pid`, să îl transform în număr întreg și abia apoi să folosesc funcția `kill()` pentru a trimite semnalul `SIGUSR1`.

3. Pentru comanda de `remove_district`, AI-ul mi-a clarificat funcționarea lui `fork()`. Mi-a explicat cum procesul părinte trebuie să folosească `wait()` pentru a aștepta cuminte terminarea procesului copil, dar și cum să verific dacă comanda Linux `rm -rf` a avut succes folosind macro-ul `WEXITSTATUS`.