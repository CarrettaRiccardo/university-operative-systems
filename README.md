# Progetto Lab. S.O. 2018/2019

## Componenti del gruppo
- Matteo Destro __192657__
- Riccardo Carretta __195545__
- Steve Azzolin __195365__

## Caratteristiche implementazione
Per la comunicazione tra processi abbiamo utilizzato una singola __message queue__.

Per la comunicazione tra shell manuale e controller abbiamo optato per l'utilizzo dei __segnali__.

Quando un processo muore i dispositivi di controllo intercettano il segnale che il dispositivo morto invia al padre, così da liberare le strutture dati informative che il SO tiene in memoria.

Usare __./terminal__ per la shell normale e __./manual__ per quella manuale.

Il comando `link` può essere eseguito anche da __shell manuale__, in quanto rappresenta l'operazione di collegare un componente ad un altro (collegare una lampadina fisicamente ad un HUB).

Da comando manuale non è possibile eseguire `unlink`, dato che è un comando che rimuove un componente dal controller (centralina). Quindi può essere eseguito solo da terminal standard.




## Shell per override manuali
L'eseguibile __./manual__ deve essere lanciato passando come parametro l'id relativo alla centralina (stampato come prima linea all' avvio della stessa o visualizzabile con il comando `help`) così da consentire una comunicazione diretta per risolvere *id* in *pid*.
Una volta che l'*id* è stato risolto, la comunicazione tra __manual__ e device finale avviene direttamente con le stesse metodologie usate per la normale comunicazione tra processi.

Manual può eseguire qualsiasi comando eccetto i comandi `list`, `unlink`, `add` ed `export` sia su dispositivi abilitati che non abilitati.



## Peculiarità
- Il controller (ossia la centralina, che ha sempre *id*=0) può essere  spenta tramite il comando __switch 0 off__.
Quando ciò avviene, da **terminal** non è possibile eseguire i comandi `switch` e `set`, mentre tramite **manual** sono ancora utilizzabili.   
  - I timer si sospendono (perchè sono ipoteticamente gestiti direttamente da essa), mentre i fridge e alarm funzionano normalmente (rispettivamente chiusura automatica e "rilevazione intrusi"). Ciò può essere inteso come un *mini sistema di sicurezza* per la casa, così facendo in un ipotetico impianto domotico non c'è il richio di avere alimenti avariati o un allarme non funzionante in caso di guasto della centralina (o di mancanza della corrente ecc...).   


- Gestione degli __HUB__ (Vedi *Aggiunte* per maggiori info).

- Gli eseguibili sono stati testati nei seguenti sistemi operativi :   
   - Ubuntu 16.04 (virtual machine)
   - Ubuntu pc laboratorio
   - Xubuntu 18.04 (virtual machine)
   - Centos 6.1 


## Aggiunte
1. __Supporto dispositivi eterogenei in HUB e TIMER__:   
     Lo stato, gli interruttori e i registri di un hub o timer sono un mirroring di quelli dei figli, sia nel caso i figli siano dello stesso tipo che nel caso siano di tipo diverso.  
     Gli hub supportano tutte le operazioni eseguibili sui figli, compresi `switch` e `set`. Ad esempio uno switch su un interruttore specifico agirà sugli interruttori corrispondenti di tutti i figli, comportamento analogo peri registri.   
     Nel caso un particolare tipo di dispositivo non sia presente nel sottoalbero dei figli dell'hub, i corrispondenti interruttori e registri non saranno visualizzabili o modificabili.    
     Se un hub ha come figli (diretti o non) più dispositivi dello stesso tipo, il valore mostrato dai registri sarà il massimo tra i valori dei figli.  
     E' stato aggiunto un interruttore `all` con il quale è possibile agire sullo stato di **tutti** i device collegati all'hub, indipendentemente dal loro tipo.


2. __Nuovo dispositivo ALARM__:  
     Dispositivo che simula un allarme casalingo. Quando rileva una persona/movimento si accende (idealmente emettendo un suono, a livello implementativo viene solo segnalata l'accensione).    
     Per ragioni implementative l'allarme scatta con una certa probabilità *prob* settabile come registro (la probabilità viene testata ogni *10* secondi).
     - _stato_: ​ ringing/off
     - _interruttori_: enable (on/off per accendere/spegnere)
     - _registro_: ​ ​ time​ = tempo di utilizzo (di accensione)

3. __Gestione IMPORT/EXPORT configurazione__:  
     All'interno del __terminal__ è disponibile un comando `export` che consente di salvare la struttura della rete costruita fin ora. Per implementare questa funzionalità ci appoggiamo ad un file temporaneo salvato nella cartella *./bin* che viene sempre cancellato alla chiusura di ogni terminale.     
     Il nome di tale file temporaneo è *tmp_file<pid_creatore>* in modo da evitare conflitti con eventuali altri terminal avviati in concorrenza tra di loro.   
     Per importare una struttura esportata precedentemente va specificato il percorso relativo del file da importare come primo argomento dell'eseguibile __terminal__.    
     E' presente nella cartella "project" un file denominato "sample_net1.txt" importabile con una rete di dispositivi già pronta.
     

4. __MULTI-TERMINAL__:   
     Il sistema supporta diversi __terminal__ indipendenti aperti in concorrenza.

5. __Comando UNLINK__:  
     E' supportato il comando `unlink` che permette di scollegare un componente dal controller (centralina). E' comunque possibile interagire con esso tramite la shell manuale. Per rispettare la struttura gerarchica del sistema, il componente viene effettivamente rimosso dall' albero dei processi figli del **controller**, diventando processo figlio di **terminal**.



## Alcuni esempi di comandi
- `add bulb/timer/alarm/fridge/window/hub`
- `del <id_device>`
- `unlink <id_device>`
- `switch <id_bulb> light on/off`
- `switch <id_window> open on/off`
- `switch <id_hub> light on/off`  (accende/spegne tutte le lampadine ad esso connesse)
- `switch <id_hub> open on/off ` (apre/chiude tutte le finestre ad esso connesse)
- `switch <id_hub> all on/off`  (accende/spegne o apre/chiude tutti i componenti ad esso connessi)
- `switch 0 general on/off`  (accende/spegne la centralina)
- `set <id_fridge> delay 15` (setta il tempo di chiusura a 15s per il frigorifero)
- `set <id_alarm> prob 20` (setta la probabilità di accensione dell'alarm a 20%)