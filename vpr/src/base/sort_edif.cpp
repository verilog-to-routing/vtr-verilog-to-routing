
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
    std::vector<std::tuple<std::string, std::string, std::string>> ports_vec;
    std::vector<std::tuple<std::string, std::string, std::string, std::string>> instance_vec;
    std::vector<std::tuple<std::string, std::string, std::string>> con_vec;

    std::map<std::string, std::vector<std::tuple<std::string, std::string, std::string>>> ports;

    std::map<std::string, std::vector<std::tuple<std::string, std::string, std::string>>> nets;

    std::string top_cell;
    std::vector<Cell> cell_vec;

    std::string find_top_(struct SNode* head) {
        std::string top;
        const char* t1;
        struct SNode* current = head;
        //struct Cell* n_cell = NULL;
        while (current != NULL) {
            if (current->type == 2) {
                std::string cmp_string = "design";
                std::string input_string = current->value;
                if (input_string == cmp_string) {
                    current = current->next;
                    t1 = current->value;
                    //top (&t1);
                    top.assign(t1);
                    //printf ("\ntop_cell name %s",top.c_str() );
                }
            }
            current = current->next;
        }
        return top;
    }

    void map_cell_ports(struct SNode* head) {
        int list_depth = 0;
        struct SNode* current = head;
        std::string cell_name;
        std::string port_name;
        std::string port_dirt;
        std::string port_size;
        while (current != NULL) {
            if (current->type == 0 || current->type == 5) {
                list_depth = current->list_counter;
            }
            ////printf ("the main while list depth is %d ",list_depth);
            if (current->type == 2) {
                std::string cmp_string = "cell";
                std::string cmp_string1 = "null";
                std::string input_string = current->value;
                if (input_string == cmp_string)
                //   entered a cell
                {
                    current = current->next;
                    if (current->type == 0 || current->type == 5) {
                        //list_depth=current->list_counter;
                        current = current->next;
                        cmp_string = "rename";
                        input_string = current->value;
                        if (input_string == cmp_string) {
                            current = current->next;
                            cell_name = current->value;
                        }
                    }
                    // A struct with the name cell is created
                    else {
                        cell_name = current->value;
                    }
                    //printf ("\n   The cell is found with the name of %s",cell_name.c_str());
                    int cell_iteration = list_depth - 1;
                    while (list_depth > cell_iteration) {
                        current = current->next;
                        if (current->type == 0 || current->type == 5) {
                            list_depth = current->list_counter;
                        } else if (current->type == 2) {
                            // creating my ports structure
                            cmp_string = "port";
                            input_string = current->value;
                            if (input_string == cmp_string) {
                                port_size = "1";
                                current = current->next;
                                if (current->type == 0 || current->type == 5) {
                                    //list_depth=current->list_counter;
                                    current = current->next;
                                    cmp_string = "array";
                                    cmp_string1 = "rename";
                                    input_string = current->value;

                                    if ((input_string == cmp_string1)) {
                                        current = current->next;
                                        port_name = current->value;
                                        current = current->next;
                                        current = current->next;
                                        current = current->next;
                                        current = current->next;
                                        current = current->next;
                                        port_dirt = current->value;
                                    }
                                    if (input_string == cmp_string) {
                                        current = current->next;
                                        port_name = current->value;
                                        current = current->next;
                                        port_size = current->value;
                                        current = current->next;
                                        current = current->next;
                                        current = current->next;
                                        current = current->next;
                                        port_dirt = current->value;
                                    }
                                } else {
                                    port_name = current->value;
                                    current = current->next;
                                    current = current->next;
                                    current = current->next;
                                    port_dirt = current->value;
                                }
                                ports_vec.push_back(std::make_tuple(port_name, port_dirt, port_size));
                                //printf ("\n The port is : %s \n Direction is : %s and size is : %s",port_name.c_str(),port_dirt.c_str(), port_size.c_str());
                            }
                        }
                    }
                    ports.insert({cell_name, ports_vec});
                    ports_vec.clear();
                }
            }
            current = current->next;
        }

        return;
    }

    void find_cell_instances(struct SNode* head, std::string top_name) {
        int list_depth = 0;
        struct SNode* current = head;
        std::string cell_name;
        std::string inst_name;
        std::string inst_ref;
        std::string property_lut;
        std::string property_width = "1";
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
                            cell_name = current->value;
                        }
                    }
                    // A struct with the name cell is created
                    else {
                        cell_name = current->value;
                    }
                    //printf("\ncell name is %s",cell_name.c_str());

                    if (cell_name == top_name) {
                        //printf("\ntop cell found");

                        int cell_iteration = list_depth - 1;
                        //   Entering the cell it will iterate untill the cell ends
                        while (list_depth > cell_iteration) {
                            current = current->next;
                            if (current->type == 0 || current->type == 5) {
                                list_depth = current->list_counter;
                            } else if (current->type == 2) {
                                cmp_string = "instance";
                                input_string = current->value;
                                if (input_string == cmp_string) {
                                    current = current->next;
                                    if (current->type == 0 || current->type == 5) {
                                        current = current->next;
                                        cmp_string = "rename";
                                        input_string = current->value;
                                        if (input_string == cmp_string) {
                                            current = current->next;
                                            inst_name = current->value;
                                        }
                                    } else {
                                        inst_name = current->value;
                                    }
                                    //printf ("\n  The instance is created with the name of %s", inst_name.c_str());
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
                                                inst_ref = current->value;
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
                                                    property_lut = current->value;
                                                    //printf("\nThe lut property is %s",property_lut.c_str());
                                                }
                                                cmp_string = "WIDTH";
                                                if (input_string == cmp_string) {
                                                    current = current->next;
                                                    current = current->next;
                                                    current = current->next;
                                                    property_width = current->value;
                                                    //printf("\nThe width property is %s",property_width.c_str());
                                                }
                                            }
                                        }
                                    }
                                    instance_vec.push_back(std::make_tuple(inst_name, inst_ref, property_lut, property_width));
                                }
                            }
                        }
                    }
                }
            }
            current = current->next;
        }
    }

    void find_cell_net(struct SNode* head, std::string top_name)

    {
        int list_depth = 0;
        struct SNode* current = head;
        std::string cell_name;
        std::string net_name;
        std::string port_ref;
        std::string net_member;

        std::string member_num;
        std::string instance_ref;
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
                            cell_name = current->value;
                        }
                    }
                    // A struct with the name cell is created
                    else {
                        cell_name = current->value;
                    }
                    //printf("\ncell name is %s",cell_name.c_str());

                    if (cell_name == top_name) {
                        int cell_iteration = list_depth - 1;
                        //   Entering the cell it will iterate untill the cell ends
                        while (list_depth > cell_iteration) {
                            current = current->next;
                            if (current->type == 0 || current->type == 5) {
                                list_depth = current->list_counter;
                            } else if (current->type == 2) {
                                input_string = current->value;
                                cmp_string = "net";
                                //input_string= current->value;
                                if (input_string == cmp_string) {
                                    current = current->next;
                                    if (current->type == 0 || current->type == 5) {
                                        //list_depth=current->list_counter;
                                        current = current->next;
                                        cmp_string = "rename";
                                        input_string = current->value;
                                        if (input_string == cmp_string) {
                                            current = current->next;
                                            net_name = current->value;
                                        }
                                    } else {
                                        net_name = current->value;
                                    }
                                    //std:: string net_name_repeat= net_name;
                                    //printf ("\n The net is  :  %s",net_name.c_str());
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
                                                        port_ref = current->value;
                                                        //printf ("\n The port ref is  :  %s",port_ref.c_str());
                                                        current = current->next;
                                                        member_num = current->value;
                                                        //printf ("\n The member is  :  %s",member_num.c_str());
                                                    }
                                                } else {
                                                    port_ref = current->value;
                                                    //printf ("\n The port refis  :  %s",port_ref.c_str());

                                                    member_num = "0";
                                                    //printf ("\n The member is  :  %s",member_num.c_str());
                                                }
                                                int instance_ref_iteration = list_depth - 1;
                                                //bool instance_ref = false;
                                                instance_ref = top_name;
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

                                                            instance_ref = current->value;
                                                            //printf("\ninstance ref is  %s",instance_ref.c_str() );
                                                        }
                                                    }
                                                }
                                                port_ref = port_ref + instance_ref ;
                                                port_ref= port_ref + member_num;
                                                //printf ("\n\n\n\n%s",port_ref.c_str() );

                                                con_vec.push_back(std::make_tuple(port_ref, member_num, instance_ref));
                                                //printf("\nthe port_ref is %s\n", port_ref.c_str());
                                            }
                                        }
                                    }
                                    // printf ("\n The net name is %s", net_name.c_str());
                                    nets.insert({net_name, con_vec});
                                    printf("The size of nets map is %d", nets.size());
                                    con_vec.clear();
                                }
                            }
                        }
                    }
                }
            }
            current = current->next;
        }
    }

    std::vector<std::tuple<std::string, std::string, std::string>> find_cell_ports(std::string cell_name) {
        std::vector<std::tuple<std::string, std::string, std::string>> temp_vec;
        for (auto it = ports.begin(); it != ports.end(); it++) {
            //std::cout<< "\nmap value : " << it->first<< "\n";
            if (it->first == cell_name) {
                // std::cout<< "\nmap value : " << it->first<< "\n";
                temp_vec = (*it).second;
                //for (int k= 0; k< temp_vec.size(); k++)
                //{
                // printf ("\nport is %s & Direction is %s ",temp_vec[k].first.c_str(), temp_vec[k].second.c_str() );
                //s}
            }
        }
        return temp_vec;
    }
};
