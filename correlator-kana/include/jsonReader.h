#ifndef KANA_JSONREADER_H
#define KANA_JSONREADER_H

#include "JSON_parser.h"

/* possible values of the type */
#define RD_ST_ARY	1
#define RD_ST_OBJ   2
#define RD_ST_KEY   3
#define RD_ST_VAL   4
#define RD_0    	0 /* nenoteikts */

typedef struct JSON_TreeNode_struct JSON_TreeNode;
typedef struct JSON_Stack_struct JSON_Stack;

typedef enum {
    EMPTY,
    OBJ,
    INT,
    FLOAT,
    STRING,
    ARRAY
} JSON_Type;

typedef union {
    char* chr;
    double dbl;
    long int lng;
    JSON_TreeNode* downNode;
} JSON_NodeValue;

struct JSON_TreeNode_struct {
    char* name;
    JSON_Type type;
    JSON_TreeNode* leftNode;
    JSON_NodeValue value;
};

struct JSON_Stack_struct{
    JSON_TreeNode* nodes[256];
    int top;
};

typedef struct {
    JSON_Stack* stack;
    JSON_TreeNode* current;
    JSON_TreeNode* tree;
} JSON_Tree;

int JSON_read(JSON_Tree* jr, char* fname);

char* JSON_getString(JSON_TreeNode* node, char* str);
long JSON_getInt(JSON_TreeNode* node, char* str);
double JSON_getFloat(JSON_TreeNode* node, char* str);
int JSON_hasKey(JSON_TreeNode* node, char* str);

void JSON_printTree(JSON_TreeNode* root);
void JSON_freeTree(JSON_TreeNode* root);

#endif //KANA_JSONREADER_H
