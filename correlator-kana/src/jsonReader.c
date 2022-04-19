#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "../include/JSON_parser.h"
#include "../include/jsonReader.h"
#include "../include/panic.h"

static int readCallback(void* ctx, int type, const JSON_value* value);

static JSON_Stack* newStack();
static JSON_TreeNode* newNode();

static void addNodeToObject(JSON_TreeNode **current);
static void addNode(JSON_TreeNode **current);

static void push(JSON_Stack* S, JSON_TreeNode* node);
static JSON_TreeNode* pop(JSON_Stack* S);

static void setRoot(JSON_Tree* jr);
static JSON_TreeNode* getStackTop(JSON_Stack* S);

static void printNode(JSON_TreeNode* node, int tab);
static void printTabs(int tab);

static JSON_TreeNode* findValue(JSON_TreeNode* node, char* name, int isArray);
static JSON_TreeNode* getValue(JSON_TreeNode* node, char* name);

// PUBLIC -----------------------------------

int JSON_read(JSON_Tree *jr, char *fname) {
    setRoot(jr);

    JSON_config config;
    init_JSON_config(&config);
    config.depth                  = 19;
    config.callback               = &readCallback;
    config.callback_ctx           = jr;
    config.allow_comments         = 1;
    config.handle_floats_manually = 0;

    JSON_parser parser = new_JSON_parser(&config);

    int result = 0;
    FILE* input = fopen(fname,"r");
    if(!input) return result;

    for (int count = 0; input; ++count) {
        int nextChar = fgetc(input);
        if (nextChar <= 0) break;
        if (!JSON_parser_char(parser, nextChar)) goto done;
    }

    if (!JSON_parser_done(parser)) goto done;

    result = 1;
    done:
    delete_JSON_parser(parser);
    fclose(input);
    free(jr->stack);
    return result;
}

char* JSON_getString(JSON_TreeNode* node, char* str) {
    node = getValue(node, str);
    if (node == NULL || node->type != STRING) {
        char* message = malloc(strlen(str) + 32);
        sprintf(message, "cannot read '%s' (string)", str);
        panic(message);
    }
    return node->value.chr;
}

long JSON_getInt(JSON_TreeNode* node, char* str) {
    node = getValue(node, str);
    if(node == NULL || node->type != INT) {
        char* message = malloc(strlen(str) + 32);
        sprintf(message, "cannot read '%s' (int)", str);
        panic(message);
    }
    return node->value.lng;
}

double JSON_getFloat(JSON_TreeNode* node, char* str) {
    node = getValue(node, str);
    if (node == NULL || node->type != FLOAT) {
        char* message = malloc(strlen(str) + 32);
        sprintf(message, "cannot read '%s' (float)", str);
        panic(message);
    }
    return node->value.dbl;
}

int JSON_hasKey(JSON_TreeNode* node, char* str) {
    if (getValue(node, str) == NULL) return 0;
    else return 1;
}

void JSON_printTree(JSON_TreeNode* root) {
    JSON_TreeNode* currentNode = root;
    JSON_Stack* stack = newStack();
    int tab = 0;
    while(1) {
        printNode(currentNode, tab);
        switch(currentNode->type) {
            case OBJ:
                push(stack, currentNode);
                printTabs(tab);
                printf("{\n");
                tab++;
                currentNode = currentNode->value.downNode;
                break;
            case ARRAY:
                push(stack, currentNode);
                printTabs(tab);
                printf("[\n");
                tab++;
                currentNode = currentNode->value.downNode;
                break;
            default:
                currentNode = currentNode->leftNode;
                break;
        }

        while (currentNode == NULL) {
            if(stack->top == -1) return;
            tab--;
            printTabs(tab);
            currentNode = pop(stack);
            if (currentNode->type == OBJ) {
                printf("}\n");
            } else if(currentNode->type == ARRAY) {
                printf("]\n");
            }
            currentNode = currentNode->leftNode;
        }
    }
    free(stack);
}

void JSON_freeTree(JSON_TreeNode* root) {
    JSON_Stack* stack = newStack();
    int i=0;

    JSON_TreeNode* currentNode = root;
    while(1) {
        if(currentNode->leftNode != NULL) {
            push(stack, currentNode->leftNode);
        }
        if (currentNode->type == OBJ || currentNode->type == ARRAY) {
            push(stack, currentNode->value.downNode);
            free(currentNode);
            if(stack->top == -1) return;
            currentNode = pop(stack);
        } else {
            free(currentNode);
            if(stack->top == -1) return;
            currentNode = pop(stack);
        }
        i++;
    }
    free(stack);
}

// STATIC -----------------------------------

static int readCallback(void* ctx, int type, const JSON_value* value) {
    JSON_Tree* tree = ctx;
    JSON_Stack* stack = tree->stack;

    switch (type) {
        case JSON_T_OBJECT_BEGIN:
            if (tree->current->type != EMPTY) {
                addNode(&(tree->current));
            }
            push(stack, tree->current);
            tree->current->type = OBJ;
            addNodeToObject(&(tree->current));
            break;

        case JSON_T_OBJECT_END:
            tree->current = pop(stack);
            break;

        case JSON_T_KEY:
            if (tree->current->type != EMPTY) {
                addNode(&(tree->current));
            }
            tree->current->name = strdup(value->vu.str.value);
            break;

        case JSON_T_ARRAY_BEGIN:
            if(tree->current->type != EMPTY) {
                addNode(&(tree->current));
            }
            push(stack, tree->current);
            tree->current->type = ARRAY;
            addNodeToObject(&(tree->current));
            break;

        case JSON_T_ARRAY_END:
            tree->current = pop(stack);
            break;

        case JSON_T_STRING:
            if (tree->current->type != EMPTY
                && getStackTop(stack)->type == ARRAY) {
                addNode(&(tree->current));
                char* cval = strdup(value->vu.str.value);
                tree->current->value.chr = cval;
                tree->current->type = STRING;
            } else if (tree->current->type == EMPTY) {
                char* cval = strdup(value->vu.str.value);
                tree->current->value.chr = cval;
                tree->current->type = STRING;
            }
            break;

        case JSON_T_FLOAT:
            if(tree->current->type != EMPTY
               && getStackTop(stack)->type == ARRAY) {
                addNode(&(tree->current));
                double dval = value->vu.float_value;
                tree->current->value.dbl = dval;
                tree->current->type = FLOAT;
            } else if (tree->current->type == EMPTY) {
                double dval = value->vu.float_value;
                tree->current->value.dbl = dval;
                tree->current->type = FLOAT;
            }
            break;

        case JSON_T_INTEGER:
            if(tree->current->type != EMPTY
               && getStackTop(stack)->type == ARRAY) {
                addNode(&(tree->current));
                long lval = value->vu.integer_value;
                tree->current->value.lng = lval;
                tree->current->type = INT;
            } else if (tree->current->type == EMPTY) {
                long lval = value->vu.integer_value;
                tree->current->value.lng = lval;
                tree->current->type = INT;
            }
            break;

        case JSON_T_TRUE:
            if(tree->current->type != EMPTY
               && getStackTop(stack)->type == ARRAY) {
                addNode(&(tree->current));
                tree->current->value.chr = "true";
                tree->current->type = STRING;
            } else if (tree->current->type == EMPTY) {
                tree->current->value.chr = "true";
                tree->current->type = STRING;
            }
            break;

        case JSON_T_FALSE:
            if (tree->current->type != EMPTY
                && getStackTop(stack)->type == ARRAY) {
                addNode(&(tree->current));
                tree->current->value.chr = "false";
                tree->current->type = STRING;
            } else if(tree->current->type == EMPTY) {
                tree->current->value.chr = "false";
                tree->current->type = STRING;
            }
            break;

        case JSON_T_NULL:
            if (tree->current->type != EMPTY
                && getStackTop(stack)->type == ARRAY) {
                addNode(&(tree->current));
                tree->current->value.chr = "null";
                tree->current->type = STRING;
            } else if (tree->current->type == EMPTY) {
                tree->current->value.chr = "null";
                tree->current->type = STRING ;
            }
            break;
    }

    return 1;
}

static void setRoot(JSON_Tree* jr) {
    //ROOT node:
    jr->current = jr->tree = newNode();
    jr->tree->name = "root";
    jr->tree->leftNode = NULL;
    jr->tree->type = EMPTY;
    //STACK:
    jr->stack = newStack();
}

static void addNode(JSON_TreeNode** current) {
    if (current[0]->type == EMPTY)
        panic("addNode: tree empty");
    current[0]->leftNode = newNode();
    current[0] = current[0]->leftNode;
}

static void addNodeToObject(JSON_TreeNode** current) {
    if (current[0]->type != OBJ && current[0]->type != ARRAY)
        panic("addNodeToObject: not an array or object");
    current[0]->value.downNode = newNode();
    current[0] = current[0]->value.downNode;
}

static JSON_TreeNode* newNode() {
    JSON_TreeNode* node = malloc(sizeof(node[0]));
    //if (node == NULL) ALLOC_ERROR;
    node->name = NULL;
    node->leftNode = NULL;
    node->type = EMPTY;
    return node;
}

static void push(JSON_Stack* stack, JSON_TreeNode* node) {
    (stack->top)++;
    stack->nodes[stack->top] = node;
}

static JSON_TreeNode* pop(JSON_Stack* stack) {
    int index = stack->top;
    (stack->top)--;
    if(stack->top < -1)
        panic("pop: stack empty");
    return stack->nodes[index];
}

static JSON_TreeNode* getStackTop(JSON_Stack* stack) {
    return stack->nodes[stack->top];
}

static JSON_Stack* newStack(){
    JSON_Stack* stack = malloc(sizeof(JSON_Stack));
    //if (stack == NULL) ALLOC_ERROR;
    stack->top = -1;
    return stack;
}

static void printTabs(int tab) {
    for (int i = 0; i < tab; ++i) {
        printf("\t");
    }
}

static void printNode(JSON_TreeNode* node, int tab) {
    printTabs(tab);
    if (node->name != NULL) {
        printf("%s: ", node->name);
    }

    switch (node->type) {
        case STRING:
            printf("%s\n", node->value.chr);
            break;
        case FLOAT:
            printf("%.10Le\n", (long double)node->value.dbl);
            break;
        case INT:
            printf("%ld\n", node->value.lng);
            break;
        case OBJ:
            printf("\n");
            break;
        case ARRAY:
            printf("\n");
            break;
        case EMPTY:
            printf("\n");
            break;
    }
}

static JSON_TreeNode* getValue(JSON_TreeNode* node, char* str) {
    char* name = malloc(sizeof(char) * 255);
    name[0] = '\0';

    int i = 0;
    int isArray = 0;
    while (1) {
        int len = strlen(name);
        name[len + 1] = name[len];

        switch(str[i]) {
            case ':':
                node = findValue(node, name, isArray);
                if (node == NULL) return NULL;
                if(node->type == ARRAY) isArray = 1;
                else isArray = 0;

                if(node->type == ARRAY || node->type == OBJ) {
                    node = node->value.downNode;
                } else {
                    return NULL;
                }
                name[0] = '\0';
                break;

            case '\0':
                node = findValue(node, name, isArray);
                free(name);
                return node;
        }
        name[len] = str[i];
        ++i;
    }
}

static JSON_TreeNode* findValue(JSON_TreeNode* node, char* name, int isArray){
    if(isArray == 1) {
        int index = atoi(name);
        for(int i = 0; i < index; ++i) {
            if(node->leftNode == NULL) {
                return NULL;
            } else {
                node = node->leftNode;
            }
        }
        return node;
    }

    while(node != NULL && node->name != NULL) {
        if(strcmp(node->name, name) == 0) {
            return node;
        } else {
            node = node->leftNode;
        }
    }

    return NULL;
}

