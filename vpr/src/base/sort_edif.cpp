
#include <assert.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <tuple>
#include <iostream>
#include <map>
#include <bits/stdc++.h>
#include "read_edif.cpp"
#include "sort_edif.h"

class usefull_data {
  public:
    std::vector<std::pair<char*, char*>> ports_vec;
    std::vector<std::tuple<char*, char*, char*, char*>> con_vec;
    std::vector<std::pair<char*, std::vector<char*>>> nets_vec;
    //std::map<char*, std::vector<std::tuple<  char* , char*, char*>>> nets;

    std::string top_cell;
    std::vector<Cell> cell_vec;

    char* find_top_(struct SNode* head) {
        char* top;
        struct SNode* current = head;
        struct Cell* n_cell = NULL;
        while (current != NULL) {
            if (current->type == 2) {
                std::string cmp_string = "design";
                std::string input_string = current->value;
                if (input_string == cmp_string) {
                    current = current->next;
                    top = current->value;
                    printf("\ntop_cell name %s", top);
                }
            }
            current = current->next;
        }
        return top;
    }

    struct Cell* read_thelinklist(struct SNode* head, char* cell_name) {
        int list_depth = 0;
        std::vector<Cell> cells_vector;

        printf("\nlooking for %s", cell_name);

        struct SNode* current = head;
        struct Cell* n_cell = NULL;
        n_cell = new Cell;
        while (current != NULL) {
            if (current->type == 0 || current->type == 5) {
                list_depth = current->list_counter;
            } else if (current->type == 2) {
                std::string cmp_string = "cell";
                std::string cmp_string1 = "null";
                std::string input_string = current->value;
                if (input_string == cmp_string)
                // The cell starts here when we encounter the word "cell"
                {
                    current = current->next;
                    if (current->type == 0 || current->type == 5) {
                        //list_depth=current->list_counter;
                        current = current->next;
                        cmp_string = "rename";
                        input_string = current->value;
                        if (input_string == cmp_string) {
                            current = current->next;
                            n_cell->cell_name = current->value;
                        }
                    }
                    // A struct with the name cell is created
                    else {
                        n_cell->cell_name = current->value;
                    }
                    printf("\n   The cell is created with the name of %s and we are looking for %s", n_cell->cell_name, cell_name);
                    if (*(n_cell->cell_name) == *cell_name) {
                        printf("\n   The cell is created with the name of %s", n_cell->cell_name);
                        // Now we will iterate in the cell untill we reach the end of the cell
                        int cell_iteration = list_depth - 1;
                        //   Entering the cell it will iterate untill the cell ends
                        while (list_depth > cell_iteration) {
                            current = current->next;
                            if (current->type == 0 || current->type == 5) {
                                list_depth = current->list_counter;
                            } else if (current->type == 2) {
                                // now getting the interface of the cell the method is similar to a cell
                                input_string = current->value;
                                cmp_string = "interface";
                                if (input_string == cmp_string) {
                                    int interface_iteration = list_depth - 1;
                                    while (list_depth > interface_iteration) {
                                        current = current->next;
                                        if (current->type == 0 || current->type == 5) {
                                            list_depth = current->list_counter;
                                        }
                                        if (current->type == 2) {
                                            // creating my ports structure
                                            cmp_string = "port";
                                            input_string = current->value;
                                            if (input_string == cmp_string) {
                                                current = current->next;
                                                Port* n_port = NULL;
                                                n_port = new Port;

                                                if (current->type == 0 || current->type == 5) {
                                                    list_depth = current->list_counter;
                                                    current = current->next;
                                                    cmp_string = "array";
                                                    cmp_string = "rename";
                                                    input_string = current->value;
                                                    if ((input_string == cmp_string) | (input_string == cmp_string1)) {
                                                        current = current->next;
                                                        n_cell->ports = n_port;
                                                        n_port->port_name = current->value;
                                                        current = current->next;
                                                        current = current->next;
                                                        current = current->next;
                                                        current = current->next;
                                                        current = current->next;
                                                        n_port->port_dirt = current->value;
                                                    }
                                                } else {
                                                    n_cell->ports = n_port;
                                                    n_port->port_name = current->value;

                                                    current = current->next;
                                                    current = current->next;
                                                    current = current->next;
                                                    n_port->port_dirt = current->value;
                                                }
                                                ports_vec.push_back(std::make_pair(n_port->port_name, n_port->port_dirt));
                                                printf("\n The port is : %s \n Direction is : %s", n_port->port_name, n_port->port_dirt);
                                            }
                                        }
                                    }
                                }
                                //   second section of contents
                                cmp_string = "contents";
                                if (input_string == cmp_string) {
                                    int content_iteration = list_depth - 1;
                                    while (list_depth > content_iteration) {
                                        current = current->next;
                                        if (current->type == 0 || current->type == 5) {
                                            list_depth = current->list_counter;
                                        }
                                        if (current->type == 2) {
                                            cmp_string = "instance";
                                            input_string = current->value;
                                            if (input_string == cmp_string) {
                                                Instan* n_instan = NULL;
                                                n_instan = new Instan;
                                                current = current->next;

                                                if (current->type == 0 || current->type == 5) {
                                                    current = current->next;
                                                    cmp_string = "rename";
                                                    input_string = current->value;
                                                    if (input_string == cmp_string) {
                                                        current = current->next;
                                                        n_instan->inst_name = current->value;
                                                    }
                                                } else {
                                                    n_instan->inst_name = current->value;
                                                }
                                                printf("\n  The instance is created with the name of %s", n_instan->inst_name);
                                                int instance_iteration = list_depth - 1;
                                                //   Entering the cell it will iterate untill the cell ends
                                                while (list_depth > instance_iteration) {
                                                    current = current->next;
                                                    if (current->type == 0 || current->type == 5) {
                                                        list_depth = current->list_counter;
                                                    }

                                                    if (current->type == 2) {
                                                        cmp_string = "cellRef";
                                                        input_string = current->value;

                                                        if (input_string == cmp_string) {
                                                            current = current->next;
                                                            n_instan->inst_ref = current->value;
                                                        }
                                                        cmp_string = "property";
                                                        input_string = current->value;
                                                        if (input_string == cmp_string) {
                                                            current = current->next;
                                                            cmp_string = "LUT";
                                                            input_string = current->value;
                                                            if (input_string == cmp_string) {
                                                                current = current->next;
                                                                current = current->next;
                                                                current = current->next;
                                                                n_instan->property_lut = current->value;
                                                                printf("\nThe lut property is %s", n_instan->property_lut);
                                                            }
                                                            cmp_string = "WIDTH";
                                                            if (input_string == cmp_string) {
                                                                current = current->next;
                                                                current = current->next;
                                                                current = current->next;
                                                                n_instan->property_width = current->value;
                                                                printf("\nThe width property is %s", n_instan->property_width);
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                            // creating my nets structure
                                            cmp_string = "net";
                                            //input_string= current->value;
                                            if (input_string == cmp_string) {
                                                struct Nets* n_net;
                                                n_net = new Nets;
                                                n_cell->nets = n_net;
                                                current = current->next;
                                                if (current->type == 0 || current->type == 5) {
                                                    //list_depth=current->list_counter;
                                                    current = current->next;
                                                    cmp_string = "rename";
                                                    input_string = current->value;
                                                    if (input_string == cmp_string) {
                                                        current = current->next;
                                                        n_net->net_name = current->value;
                                                    }
                                                } else {
                                                    n_net->net_name = current->value;
                                                }

                                                char* net_name_repeat = n_net->net_name;
                                                printf("\n The net is  :  %s", n_net->net_name);
                                                int net_joining_iteration = list_depth - 1;
                                                while (list_depth > net_joining_iteration) {
                                                    current = current->next;
                                                    if (current->type == 0 || current->type == 5) {
                                                        list_depth = current->list_counter;
                                                    }
                                                    if (current->type == 2) {
                                                        cmp_string = "portRef";
                                                        input_string = current->value;
                                                        if (input_string == cmp_string) {
                                                            current = current->next;
                                                            if (current->type == 0 || current->type == 5) {
                                                                current = current->next;
                                                                cmp_string = "member";
                                                                input_string = current->value;
                                                                if (input_string == cmp_string) {
                                                                    current = current->next;
                                                                    n_net->port_ref = current->value;
                                                                    printf("\n The port ref is  :  %s", n_net->port_ref);
                                                                    current = current->next;
                                                                    n_net->member_num = current->value;
                                                                    printf("\n The member is  :  %s", n_net->member_num);
                                                                }
                                                            } else {
                                                                n_net->port_ref = current->value;
                                                                printf("\n The port refis  :  %s", n_net->port_ref);
                                                                char* No_mem = "0";
                                                                n_net->member_num = No_mem;
                                                                printf("\n The member is  :  %s", n_net->member_num);
                                                            }
                                                            int instance_ref_iteration = list_depth - 1;
                                                            //bool instance_ref = false;
                                                            n_net->instance_ref = n_cell->cell_name;
                                                            while (list_depth > instance_ref_iteration) {
                                                                current = current->next;
                                                                if (current->type == 0 || current->type == 5) {
                                                                    list_depth = current->list_counter;
                                                                }
                                                                if (current->type == 2) {
                                                                    cmp_string = "instanceRef";
                                                                    input_string = current->value;
                                                                    if (input_string == cmp_string) {
                                                                        current = current->next;
                                                                        //instance_ref= true;
                                                                        n_net->instance_ref = current->value;
                                                                        printf("\ninstance ref is  %s", n_net->instance_ref);
                                                                    }
                                                                }
                                                            }
                                                            con_vec.push_back(std::make_tuple(net_name_repeat, n_net->port_ref, n_net->member_num, n_net->instance_ref));
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            current = current->next;
        }
        return n_cell;
    }
};
