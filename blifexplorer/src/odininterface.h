/*

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef ODININTERFACE_H
#define ODININTERFACE_H

#include <QtGui>

extern "C" {
    #include <stdlib.h>
    #include <stdio.h>
    #include <unistd.h>
    #include <string.h>
    #include "globals.h"
    #include "types.h"
    #include "util.h"
    #include "netlist_utils.h"
    #include "arch_types.h"
    #include "parse_making_ast.h"
    #include "netlist_create_from_ast.h"
    #include "outputs.h"
    #include "netlist_optimizations.h"
    #include "read_xml_config_file.h"
    #include "read_xml_arch_file.h"
    #include "partial_map.h"
    #include "multipliers.h"
    #include "netlist_check.h"
    #include "read_blif.h"
    #include "read_netlist.h"
    #include "activity_estimation.h"
    #include "high_level_data.h"
    #include "hard_blocks.h"
    #include "memories.h"
    #include "simulate_blif.h"
    #include "errors.h"
    #include "netlist_visualizer.h"
    #include "odin_ii_func.h"
}


class OdinInterface
{
public:
    OdinInterface();
    void startOdin();
    //void connectNodes();
    //void connectNodes(QHash<QString, LogicUnit *> unithashtable);
    QHash<QString, nnode_t *> getNodeTable();
    void setFilename(QString filename);
    void setUpSimulation();
    int simulateNextWave();
    void endSimulation();
    int getOutputValue(nnode_t* node, int pin, int actstep);
    void setEdge(int i);
private:
    void init_options();
    //void do_high_level_synthesis();
    void do_simulation_of_netlist();
    global_args_t global_args;
   // t_arch Arch;
    //int current_parse_file;
    t_type_descriptor* type_descriptors;
    int block_tag;
    QHash<QString, nnode_t *> nodehash;
    QQueue<nnode_t *> nodequeue;
    QString myFilename;
    int myCycle;
    //simulation
    netlist_t* netlist;
    lines_t* input_lines;
    lines_t* output_lines;
    FILE* out;
    FILE* in;
    FILE* in_out;
    FILE* modelsim_out;
    int num_vectors;
    char* input_vector_file;
    int output_edge;
    int cycle;
    stages* stgs;
    pin_names* hold_high;
    pin_names* hold_low;
    hashtable_t* hold_high_index;
    hashtable_t* hold_low_index;
    int num_cycles;
    int num_waves;
    int wave;
    test_vector* tvector;
    bool simulationStarted;
};

#endif // ODININTERFACE_H
