
#include "../include/list.h"
#include <stdio.h>
#include <stdlib.h>

list list_init() {
    list head = malloc(sizeof(node **));
    *head = NULL;
    return head;
}

int list_push(list head, int value) {
    node *new_node = (node *)malloc(sizeof(node));
    if (new_node == NULL) return 0;
    new_node->value = value;
    new_node->next = *head;
    *head = new_node;
    return 1;
}

int list_remove(list head, int value) {
    if (*head == NULL) return 0;
    node *p = *head;
    //  Se il valore da eliminare è il primo
    if (p->value == value) {
        *head = p->next;
        free(p);
        return 1;
    }
    while (p->next != NULL) {
        if (p->next->value == value) {
            node *tmp = p->next;
            p->next = p->next->next;
            free(tmp);
            return 1;
        }
        p = p->next;
    }
    return 0;
}

void list_print(list head) {
    if (*head == NULL) {
        printf("[]\n");
    } else {
        node *p = *head;
        printf("[");
        while (p != NULL) {
            printf("%d, ", p->value);
            p = p->next;
        }
        printf("]\n");
    }
}