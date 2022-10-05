

struct Nets {
    char* net_name;
    char* port_ref;
    char* instance_ref;
    char* member_num;
};

struct Instan {
    char* inst_name;
    char* inst_ref;
    char* property_lut;
    char* property_width;
};
struct Port {
    int num;
    char* port_name;
    char* port_dirt;
};

struct Cell {
    char* cell_name;
    struct Port* ports;
    struct Nets* nets;
};
