#include "../include/list.h"
#include <stdio.h>
#include <stdlib.h>

list_t listInit() {
    list_t head = malloc(sizeof(node_t **));
    *head = NULL;
    return head;
}

void listDestroy(list_t head) {
    node_t *p = *head;
    while (p != NULL) {
        node_t *tmp = p->next;
        free(p);
        p = tmp;
    }
    free(head);
}

int listPush(list_t head, int value) {
    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    if (new_node == NULL) return 0;
    new_node->value = value;
    new_node->next = *head;
    *head = new_node;
    return 1;
}

int listRemove(list_t head, int value) {
    if (*head == NULL) return 0;
    node_t *p = *head;
    //  Se il valore da eliminare è il primo
    if (p->value == value) {
        *head = p->next;
        free(p);
        return 1;
    }
    while (p->next != NULL) {
        if (p->next->value == value) {
            node_t *tmp = p->next;
            p->next = p->next->next;
            free(tmp);
            return 1;
        }
        p = p->next;
    }
    return 0;
}

int listContains(list_t head, int value) {
    node_t *p = *head;
    while (p != NULL) {
        if (p->value == value) return 1;
        p = p->next;
    }
    return 0;
}

int listCount(list_t head) {
    int count = 0;
    node_t *p = *head;
    while (p != NULL) {
        count++;
        p = p->next;
    }
    return count;
}

void listPrint(list_t head) {
    if (*head == NULL) {
        printf("[]\n");
    } else {
        node_t *p = *head;
        printf("[");
        while (p != NULL) {
            printf("%ld, ", p->value);
            p = p->next;
        }
        printf("]\n");
    }
}

int listEmpty(list_t l) {
    if (*l == NULL) {
        return 1;
    } else {
        return 0;
    }
}