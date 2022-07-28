#include <algorithm> // std::all_of

#include "vtr_util.h"   // vtr::fopen
#include "vtr_assert.h" // VTR ASSERT

#include "write_models_bb.h"

using vtr::t_linked_vptr;

/* the output file description */
#define OUTPUT_HEADER_COMMENT(Echo, ArchFile)                                                                                       \
    {                                                                                                                               \
        fprintf(Echo,                                                                                                               \
                "/*********************************************************************************************************/\n");   \
        fprintf(Echo, "/*   %-100s*/\n", "");                                                                                       \
        fprintf(Echo, "/*   %-100s*/\n",                                                                                            \
                "This is a machine-generated Verilog code, including the black box declaration of");                                \
        fprintf(Echo, "/*   %-100s*/\n",                                                                                            \
                "complex blocks defined in the following architecture file:");                                                      \
        fprintf(Echo, "/*   %-100s*/\n", "");                                                                                       \
        fprintf(Echo, "/*             %-90s*/\n", strrchr(ArchFile, '/') + 1);                                                      \
        fprintf(Echo, "/*   %-100s*/\n", "");                                                                                       \
        fprintf(Echo,                                                                                                               \
                "/*********************************************************************************************************/\n\n"); \
    }

/* a comment for the body of black box modules */
const char* HARD_BLOCK_COMMENT = "/* the body of the complex block module is empty since it should be seen as a black box */";
/* list of vtr primitives blocks */
static constexpr short num_vtr_primitives = 8;
static constexpr const char* vtr_primitives[num_vtr_primitives] = {
    "LUT_K",
    "DFF",
    "fpga_interconnect",
    "mux",
    "adder",
    "multiply",
    "single_port_ram",
    "dual_port_ram"};

/* declarations */
void DeclareModel_bb(FILE* Echo, const t_model* model);

/**
 * (function: WriteModels_bb)
 * 
 * @brief Output the black box declaration of
 * complex blocks in the Verilog format
 * 
 * @param ArchFile path to the architecture file
 * @param VEchoFile path to the architecture file
 * @param arch pointer to the arch data structure
 */
void WriteModels_bb(const char* ArchFile,
                    const char* VEchoFile,
                    const t_arch* arch) {
    // validate the arch
    VTR_ASSERT(arch);

    FILE* Echo = vtr::fopen(VEchoFile, "w");
    t_model* cur_model = arch->models;

    /* the output file description */
    OUTPUT_HEADER_COMMENT(Echo, ArchFile)

    // iterate over models
    while (cur_model) {
        // avoid printing vtr primitives
        if (std::all_of(vtr_primitives,
                        vtr_primitives + num_vtr_primitives,
                        [&](const auto& e) { return strcmp(e, cur_model->name); }))
            DeclareModel_bb(Echo, cur_model);

        // moving forward with the next complex block
        cur_model = cur_model->next;
    }

    // CLEAN UP
    fclose(Echo);
}

/**
 * (function: DeclareModel_bb)
 * 
 * @brief prints the declaration of the given 
 * complex block model into the Echo file
 * 
 * @param Echo pointer output file
 * @param model pointer to the complex block t_model
 */
void DeclareModel_bb(FILE* Echo, const t_model* model) {
    // validate the blackbox name
    VTR_ASSERT(model);

    // module
    fprintf(Echo, "module %s(\n", model->name);

    // input/output ports
    t_model_ports* input_port = model->inputs;
    while (input_port) {
        fprintf(Echo, "\tinput\t[%d:0]\t%s,\n", input_port->size - 1, input_port->name);
        // move forward until the end of input ports' list
        input_port = input_port->next;
    }

    t_model_ports* output_port = model->outputs;
    while (output_port) {
        fprintf(Echo, "\toutput\t[%d:0]\t%s,\n", output_port->size - 1, output_port->name);
        // move forward until the end of output ports' list
        output_port = output_port->next;
    }
    fprintf(Echo, ");\n");

    // body
    fprintf(Echo, "%s\n", HARD_BLOCK_COMMENT);

    // endmodule
    fprintf(Echo, "endmodule\n\n");
}