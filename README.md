# Progetto Lab. S.O. 2018/2019

## Componenti del gruppo:
- Matteo Destro __192657__
- Riccardo Carretta __195545__
- Steve Azzolin __195365__

## Caratteristiche implementazione:
Per la comunicazione tra processi abbiamo utilizzato nella maggior parte dei casi le __message queue__.

Per la comunicazione tra shell manuale e controller abbiamo optato per l'utilizzo dei __segnali__.

Da comando manuale non è possibile esegyire unlink, dato che è un a comando che rende disponibile al controllo un componetnte alla centralina. Quindi eseguito solo da terminal standard.

Mentra il link è possibile effeturalo da manual, in quanto rappresenta l'operazione di collegare un componente ad un altro (collegare una lampadina fisicamente ad un HUB)

Usare __./terminal__ per la shell normale e __./manual__ per quella manuale

##Aggiunte
- Hub con figli eterogenei
- Possibilità di fare link anche su dispositivi appena aggiunti o disabilitati (TODO)
