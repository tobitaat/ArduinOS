# Opleverset
    > Tobias de Bildt
    > TINBES-03
    > 04-03-2023
Samen met de hulp van vrienden waarbij het ook niet lukten om de test scripts te versturen naar de Arduino is het gelukt om een de test scripts te sturen. Deze scripts en de aangepaste converter is te vinden onder de folder ./converter

## Opdracht Doelen
1. Het OS leest opdrachten die op de command line worden gegeven
    > Er kunne opdrachten gestuurd worden naar de Arduino via de CLI en deze worden uitgevoerd
2. Vanuit de command line kan data in bestanden worden opgeslagen, een lijst getoond van opgeslagen bestanden, de inhoud van bestanden getoond, bestanden gewist en de hoeveelheid beschikbare vrije ruimte getoond
    >
    > `store <filename> <filesize>` slaat een bestand op.
    >
    > `retrieve <filename>` haalt een bestand zijn data op.
    >
    > `erase <filename>` verwijderd een bestand.
    > 
    > `files` toont een lijst met bestanden.
    >
    > `freespace` laat zien hoeveel ruimte er nog over is.
    >
3. Er kunnen variabelen in het geheugen opgeslagen worden van de typen CHAR, INT, FLOAT en STRING, weer teruggehaald uit het geheugen, en van waarde veranderd
    >
    > Dit kan getest worden door het script test_vars te runnen op de Arduino
    >
4. Een bestand in het bestandssysteem kan als proces gestart, gepauzeerd en gestopt worden
    >
    > `run <filename>` Start een proces op.
    >
    > `suspend <filename>` pauzeert een proces.
    >
    > `kill <filename>` stopt een proces.
    > 
5. Er wordt van verschillende processen bijgehouden wat de toestand is (running, suspended, terminated), de waarde van de program counter, en welke variabelen bij dat proces horen
    >
    > De staat van alle draaiende processen wordt bijgehouden in de processtable en kan worden opgehaald met het commando `list`
    >
6. Het OS kan instructies in een bytecode-programma uitvoeren
    >
    > Er kunnen testscripts in de ./converter folder uitgevoerd worden door deze converteren en dan te runnen (`run <filename>`)
    >
7. Het OS kan meerdere processen tegelijk uitvoeren (afwisselend een instructie van elk proces)
    >
    > Deze functionaliteit is aan te tonen door twee scripts tegelijkertijd op te starten, bijvoorbeeld test_delay
    >
9. Vanuit een bytecode-programma kunnen andere processen opgestart (geforkt) worden en kan gewacht worden op het voltooien van een ander proces
    >
    > Het script test_fork laat zien dat er programma's opgestart kunnen worden vanuit een ander programma.
    >