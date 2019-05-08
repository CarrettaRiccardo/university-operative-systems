#ifndef _UTILS_
#define _UTILS_

/*  Ritorna 1 se la stringa è un numero, 0 altrimenti   */
int isInt(char *str);

/*  Ritorna il path relativo della cartella del pat  passato come parametro  */
char *extractBaseDir(char *path);

#endif
