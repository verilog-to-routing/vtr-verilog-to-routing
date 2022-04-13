#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "read_edif.h"

#define BUFFER_MAX 512

void reverse(struct SNode* head) {
    // Initialize current, previous and
    // next pointers
    SNode* current = head;
    SNode *prev = NULL, *next = NULL;

    while (current != NULL) {
        // Store next
        next = current->next;

        // Reverse current node's pointer
        current->next = prev;

        // Move pointers one position ahead.
        prev = current;
        current = next;
    }
    head = prev;
}

//reverse (head);
void print_linklist(struct SNode* head) {
    struct SNode* current = head;
    while (current != NULL) {
        if (current->type == 0) { printf("\nThe list starts here with the value %d", current->list_counter); }

        if (current->type == 1) { printf("\nThe string is  %s", current->value); }
        if (current->type == 2) { printf("\nThe symbol is  %s", current->value); }
        if (current->type == 3) { printf("\nThe integer is  %c", *current->value); }
        if (current->type == 4) { printf("\nThe float is  %f", current->value); }
        if (current->type == 5) { printf("\nThe list ends here with the value %d", current->list_counter); }
        current = current->next;
    }
}
int is_float(char* str) {
    // printf ("\n entering is_float function to check %s", str );
    char* ptr = NULL;
    strtod(str, &ptr);

    return !*ptr;
}

int is_integer(char* str) {
    //printf ("\n entering is_integer function to check %s", str );
    char* ptr = NULL;
    strtol(str, &ptr, 10);
    return !*ptr;
}

int is_lst_term(int c) {
    //printf ("\n entering is_lst function to check %c", c );
    return c == EOF || isspace(c) || c == '(' || c == ')';
}

int is_str_term(int c) {
    return c == EOF || c == '"';
}

char* read_value(FILE* fp, int* c, int (*is_term)(int)) {
    int len = 0;
    char buffer[BUFFER_MAX + 1];

    while (!is_term(*c = fgetc(fp)) && len < BUFFER_MAX) {
        buffer[len] = *c;
        len++;
    }
    buffer[len] = '\0';

    char* str = (char*)malloc((len + 1) * sizeof(char));
    return strcpy(str, buffer);
}
static int my_number;

struct SNode* snode_parse(FILE* fp) {
    // Using a linked list, nodes are appended to the list tail until we
    // reach a list terminator at which point we return the list head.
    struct SNode *tail, *head = NULL;
    int c;

    while ((c = fgetc(fp)) != EOF) {
        struct SNode* node = NULL;

        if (c == ')') {
            node = new SNode;
            node->type = LIST_END;
            my_number = my_number - 1;
            node->list_counter = my_number;

        } else if (c == '(') {
            // Begin list recursion
            node = new SNode;
            node->type = LIST_START;
            my_number = my_number + 1;
            node->list_counter = my_number;
            //node->list = snode_parse(fp);
        } else if (c == '"') {
            // Read a string
            node = new SNode;
            node->type = STRING;
            node->value = read_value(fp, &c, &is_str_term);
        } else if (!isspace(c)) {
            // Read a float, integer, or symbol
            ungetc(c, fp);
            node = new SNode;
            node->value = read_value(fp, &c, &is_lst_term);
            // Put the terminator back
            ungetc(c, fp);
            if (is_integer(node->value)) {
                node->type = INTEGER;
            } else if (is_float(node->value)) {
                node->type = FLOAT;
            } else {
                node->type = SYMBOL;
            }
        }

        if (node != NULL) {
            // Terminate the node
            node->next = NULL;

            if (head == NULL) {
                // Initialize the list head
                head = tail = node;
            } else {
                // Append the node to the list tail
                tail = tail->next = node;
            }
        }
    }
    return head;
}
// Recursively free memory allocated by a node
void snode_free(struct SNode* node) {
    while (node != NULL) {
        struct SNode* tmp = node;
        if (node->type == LIST_START) {
            snode_free(node->list);
            node->list_counter = NULL;
        }
        if (node->type == LIST_END) {
            snode_free(node->list);
            node->list_counter = NULL;
        } else {
            // Free current value
            free(node->value);
            node->value = NULL;
        }
        node = node->next;
        // Free current node
        free(tmp);
        tmp = NULL;
    }
}
