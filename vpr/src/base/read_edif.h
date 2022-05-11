enum SNodeType {
    LIST_START,
    STRING,
    SYMBOL,
    INTEGER,
    FLOAT,
    LIST_END
};
enum Celltype {
    Cell_start,
    Ports,
    Net
};
struct SNode {
    struct SNode* next;
    enum SNodeType type;
    int list_counter;
    union {
        struct SNode* list;
        char* value;
    };
};
int is_float(char* str);
int is_integer(char* str);
int is_lst_term(int c);
char* read_value(FILE* fp, int* c, int (*is_term)(int));
int is_str_term(int c);
struct SNode* snode_parse(FILE* fp);

void snode_free(struct SNode* node);
