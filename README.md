# Progetto Lab. S.O. 2018/2019

## Componenti del gruppo
- Matteo Destro __192657__
- Riccardo Carretta __195545__
- Steve Azzolin __195365__

## Caratteristiche implementazione
Per la comunicazione tra processi abbiamo utilizzato __message queue__.

Per la comunicazione tra shell manuale e controller abbiamo optato per l'utilizzo dei __segnali__.

Quando un processo muore i dispositivi di controllo intercettano il segnale che il dispositivo morto invia al padre, così da liberare le strutture dati informative che il SO tiene in memoria.

Da comando manuale non è possibile eseguire __unlink__, dato che è un comando che rende disponibile al controllo un componente da parte della centralina. Quindi eseguito solo da terminal standard.

Il comando __link__ è possibile effetuarlo anche da __shell manuale__, in quanto rappresenta l'operazione di collegare un componente ad un altro (collegare una lampadina fisicamente ad un HUB).

Usare __./terminal__ per la shell normale e __./manual__ per quella manuale.



## Shell per override manuali
L'eseguibile __./manual__ deve essere lanciato passando come parametro l'id relativo alla centralina (stampato come prima linea all' avvio della stessa) così da consentire una comunicazione diretta per risolvere id in pid.
Una volta risolto la comunicazione tra __manual__ e device finale avviene con le stesse metodologie usate per la normale comunicazione tra processi.

Manual può eseguire qualsiasi comando eccetto i comandi di LIST, UNLINK, ADD ed EXPORT sia su dispositivi abilitati che non abilitati.


## Peculiarità
1. Quando la centralina (che ha sempre *id*=0) viene spenta tramite il comando __switch 0 off__ i timer si sospendono (perchè sono ipoteticamente gestiti direttamente da essa), mentre i fridge e alarm funzionano normalmente (rispettivamente chiusura automatica e "rilevazione intrusi"). Ciò può essere inteso come un *mini sistema di sicurezza* per la casa, così facendo in un ipotetico impianto domotico non c'è il richio di avere alimenti avariati o un allarme non funzionante in caso di guasto della centralina (o di mancanza della corrente ecc...).   
Da **terminal** non è possibile eseguire comandi __SWITCH__ e __SET__.

2. Gestione dello stato degli __HUB__ (Vedi *Aggiunte* per maggiori info).

3. Gestione diversi file sorgente con codice molto simile attraverso **#ifndef __MANUAL__**.

4. Gli eseguibili sono stati testati nei seguenti sistemi operativi :   
   - Ubuntu 16.04 (virtual machine)
   - Ubuntu BOH (pc laboratorio)
   - Xubuntu 18.04 (virtual machine)
   - Centos 6.1 


## Aggiunte
1. __HUB__ supporta dispositivi __eterogenei__:   
     Nel comando *INFO* vengono elencati tutte le label dei vari interuttori e info di ricapitolazione a seconda dei dispositivi collegati.  
     Nel caso non ci siano fridge/window collegati ad un __HUB__, il comando *switch <id> open on* restituisce **Command undeined for device <id>**  (TODO: Da verificare).    
     E' in grado di modificare lo stato di qualsiasi tipo di device collegato (che possono essere eterogenei). Inoltre con la label *all* è possibile modificare lo stato di **tutti** i device ad esso collegati, indipendenetemente dal tipo dello stesso.

2. __ALARM__:  
     Dispositivo che simula un allarme casalingo. Quando rileva una persona/movimento si accende (idealmente emettendo un suono, a livello implementativo viene solo segnalata l'accensione).    
     Per ragioni implementative l' allarme scatta con una certa probabilità *prob* settabile come registro (la probabilità viene testata ogni *10* secondi).

3. __IMPORT/EXPORT__:  
     All' interno del __terminale__ è disponibile un comando __export__ che consente di salvare la struttura della rete costruita fin ora. Per implementare questa funzionalità ci appoggiamo ad un file temporaneo salvato nella cartella *./bin* che viene sempre cancellato alla chiusura di ogni terminale.    
     Il nome di tale file temporaneo è *tmp_file<pid_creatore>* in modo da evitare conflitti con eventuali altri terminal avviati in concorrenza tra di loro.   
     Per importare una struttura esportata precedentemente va specificato il nome del file da importare come argomento dell' eseguibile __terminal__.   
     Avendo notato che nella fase di importazione di una struttura si verificano dei problemi per i comandi di **del, unlink e link** abbiamo deciso di utilizzare un semplice **usleep(X)** che consente al sistema di sincronizzarsi internamente con i messaggi/segnali inviati dei figli che sibiscono tali comandi.

4. __MULTI-TERMINAL__:   
     Il sistema supporta diversi __terminal__ indipendenti aperti in concorrenza.

5. __COMANDO UNLINK__:  
     E' supportato il comando __unlink__ che permette di scollegare un componente dalla centralina, disattivandolo. E' comunque possibile interagire con esso tramite la shell manuale. Per rispettare la struttura gerarchica del sistema, il componente viene effettivamente rimosso dall' albero dei figli della **centralina**, diventando figlio di **terminal**.



## Alcuni comand utili

1. add bulb/timer/alarm/fridge/window/hub
2. del <id_device>
3. unlink <id_device>
4. switch <id_bulb> light on/off
5. switch <id_window> open on/off
6. switch <hub> light on/off  (accende/spegne tutte le lampadine ad esso connesse)
7. switch <hub> open on/off  (apre/chiude tutte le finestre ad esso connesse)
8. switch <hub> all on/off  (accende/spegne o apre/chiude tutti i componenti ad esso connessi)
9. switch 0 general on/off  (accende/spegne la centralina)
10. set <id_fridge> delay 15 (setta il tempo di chiusura a 15s per il frigorifero)
11.
12.
13.
