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

struct SNode* snode_parse(FILE* fp);
void snode_free(struct SNode* node);
