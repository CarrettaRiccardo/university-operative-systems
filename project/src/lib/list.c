#include "../include/list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

list_t listInit(int (*equal)(const void *, const void *)) {
    list_t l = malloc(sizeof(struct list *));
    l->head = NULL;
    l->equal = equal;
    return l;
}

int cmpInt(const void *a, const void *b) {
    return *(const int *)a == *(const int *)b;
}

int cmpMsg(const void *a, const void *b) {
    return ((const message_t *)a)->sender == ((const message_t *)b)->sender;
}

list_t listIntInit() {
    listInit(cmpInt);
}

list_t listMsgInit() {
    listInit(cmpMsg);
}

void listDestroy(list_t l) {
    node_t *p = l->head;
    while (p != NULL) {
        node_t *tmp = p->next;
        free(p->value);
        free(p);
        p = tmp;
    }
    free(l);
}

int listPushBack(list_t l, void *value, size_t size) {
    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    if (new_node == NULL) return 0;
    new_node->value = malloc(size);
    memcpy(new_node->value, value, size);
    if (l->head == NULL) {
        new_node->next = l->head;
        l->head = new_node;
    } else {
        node_t *p = l->head;
        while (p->next != NULL) p = p->next;
        new_node->next = NULL;
        p->next = new_node;
    }
    return 1;
}

int listPushFront(list_t l, void *value, size_t size) {
  node_t *new_node = (node_t *)malloc(sizeof(node_t));
  if (new_node == NULL) return 0;
  new_node->value = malloc(size);
  memcpy(new_node->value, value, size);
  new_node->next = l->head;
  l->head = new_node;
  return 1;
}

int listRemove(list_t l, void *value) {
    if (l->head == NULL) return 0;
    node_t *p = l->head;
    //  Se il valore da eliminare è il primo
    if (l->equal(p->value, value)) {
        l->head = p->next;
        free(p->value);
        free(p);
        return 1;
    }
    while (p->next != NULL) {
        if (l->equal(p->next->value, value)) {
            node_t *tmp = p->next;
            p->next = p->next->next;
            free(tmp->value);
            free(tmp);
            return 1;
        }
        p = p->next;
    }
    return 0;
}

int listContains(list_t l, void *value) {
    node_t *p = l->head;
    while (p != NULL) {
        if (l->equal(p->value, value)) return 1;
        p = p->next;
    }
    return 0;
}

int listCount(list_t l) {
    int count = 0;
    node_t *p = l->head;
    while (p != NULL) {
        count++;
        p = p->next;
    }
    return count;
}

void* listLast(list_t l){
  node_t *p = l->head;
  if(p == NULL)  return NULL;
  while (p->next != NULL) {
      p = p->next;
  }
  return p->value;
}

void listIntPrint(list_t l) {
    if (l->head == NULL) {
        printf("[]\n");
    } else {
        node_t *p = l->head;
        printf("[");
        while (p != NULL) {
            printf("%d, ", *(int *)p->value);
            p = p->next;
        }
        printf("]\n");
    }
}

int listEmpty(list_t l) {
    if (l->head == NULL) {
        return 1;
    } else {
        return 0;
    }
}
