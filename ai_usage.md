# AI Usage Documentation - Phase 1

## Prompt
"Generează logica în C pentru filtrarea rapoartelor dintr-un fișier binar `reports.dat`. Vreau să pot filtra dinamic după 'category', 'severity' sau 'inspector', pe baza argumentelor din linia de comandă, și să afișez doar rapoartele care se potrivesc."

## How AI Tools Were Used
Am folosit Inteligența Artificială ca asistent pentru a scrie blocul logic asociat comenzii `--filter`. AI-ul a sugerat citirea secvențială a fișierului binar și folosirea funcțiilor `strcmp()` și `atoi()` pentru a compara dinamic câmpul și valoarea cerută din terminal cu datele structurii `Report`. 

## Rationale
Am ales să folosesc AI pentru această sarcină deoarece logica de comparare dinamică pe mai multe câmpuri ar fi necesitat un cod repetitiv. AI-ul a ajutat la structurarea codului într-un mod curat și ușor de citit.
