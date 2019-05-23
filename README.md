# Progetto Lab. S.O. 2018/2019

## Componenti del gruppo:
- Matteo Destro __192657__
- Riccardo Carretta __195545__
- Steve Azzolin __195365__

## Caratteristiche implementazione:
Per la comunicazione tra processi abbiamo utilizzato __message queue__.

Per la comunicazione tra shell manuale e controller abbiamo optato per l'utilizzo dei __segnali__.

Quando un processo muore i dispositivi di controllo intercettano il segnale che il dispositivo morto invia al padre, così da liberare le strutture dati informative che il SO tiene in memoria.

Da comando manuale non è possibile eseguire __unlink__, dato che è un comando che rende disponibile al controllo un componente da parte della centralina. Quindi eseguito solo da terminal standard.

Il comando __link__ è possibile effetuarlo anche da __shell manuale__, in quanto rappresenta l'operazione di collegare un componente ad un altro (collegare una lampadina fisicamente ad un HUB).

Usare __./terminal__ per la shell normale e __./manual__ per quella manuale.



##Shell per override manuali
L'eseguibile __./manual__ deve essere lanciato passando come parametro l'id relativo alla centralina (stampato come prima linea all' avvio della stessa) così da consentire una comunicazione diretta per risolvere id in pid.
Una volta risolto la comunicazione tra __manual__ e device finale avviene con le stesse metodologie usate per la normale comunicazione tra processi.

Manual può eseguire qualsiasi comando eccetto il comando di LIST, sia su dispositivi abilitati che non abilitati.


##Peculiarità
1. Quando la centralina si spegne i timer si sospendono (perchè sono ipoteticamente gestiti direttamente da essa), mentre i fridge e alarm funzionano normalmente (rispettivamente chiusura automatica e "rilevazione intrusi"). Ciò può essere inteso come un *mini sistema di sicurezza* per la casa, così facendo in un ipotetico impianto domotico non c'è il richio di avere alimenti avariati o un allarme non funzionante in caso di guasto della centralina (o di mancanza della corrente ecc...)

2. 

##Aggiunte
1. __HUB__ supporta dispositivi eterogenei:
     _ Nel comando *INFO* vengono elencati tutte le label dei vari interuttori e info di ricapitolazione a seconda dei dispositivi collegati. 
     _ Nel caso non ci siano fridge/window collegati ad un __HUB__, il comando *switch <id> open on* restituisce **Command undeined for device <id>**  (TODO: Da verificare)
     _ E' in grado di modificare lo stato di qualsiasi tipo di device collegato (che possono essere eterogenei). Inoltre con la label *all* è possibile modificare lo stato di **tutti ** i device ad esso collegati, indipendenetemente dal tipo dello stesso.

2. __ALARM__:
     _ Dispositivo che simula un allarme casalingo. Quando rileva una persona/movimento si accende (idealmente emettendo un suono, a livello implementativo viene solo segnalata l'accensione).
       _ Per ragioni implementative l' allarme scatta con una certa probabilità *p* decisa a priori (la probabilità viene testata ogni *X* secondi)
