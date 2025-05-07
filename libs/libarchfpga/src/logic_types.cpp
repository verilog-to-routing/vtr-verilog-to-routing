/**
 * @file
 * @author  Alex Singer
 * @date    April 2025
 * @brief   Implementation of the LogicalModels data structure.
 */

#include "logic_types.h"
#include "vtr_assert.h"
#include "vtr_util.h"

/**
 * @brief Frees all the model ports in a linked list
 */
static void free_arch_model_ports(t_model_ports* model_ports);

/**
 * @brief Frees the specified model_port, and returns the next model_port (if
 *        any) in the linked list
 */
static t_model_ports* free_arch_model_port(t_model_ports* model_port);

LogicalModels::LogicalModels() {
    // Create the logical models. These must be created first and stored at the
    // start of the list of models.
    VTR_ASSERT_MSG(all_models().size() == 0,
                   "The first models created must be the library models");
    //INPAD
    {
        LogicalModelId inpad_model_id = create_logical_model(MODEL_INPUT);
        VTR_ASSERT_OPT(inpad_model_id == MODEL_INPUT_ID);
        t_model& inpad_model = get_model(inpad_model_id);

        inpad_model.inputs = nullptr;

        inpad_model.instances = nullptr;

        inpad_model.outputs = new t_model_ports;
        inpad_model.outputs->dir = OUT_PORT;
        inpad_model.outputs->name = vtr::strdup("inpad");
        inpad_model.outputs->next = nullptr;
        inpad_model.outputs->size = 1;
        inpad_model.outputs->min_size = 1;
        inpad_model.outputs->index = 0;
        inpad_model.outputs->is_clock = false;
    }

    //OUTPAD
    {
        LogicalModelId outpad_model_id = create_logical_model(MODEL_OUTPUT);
        VTR_ASSERT_OPT(outpad_model_id == MODEL_OUTPUT_ID);
        t_model& outpad_model = get_model(outpad_model_id);

        outpad_model.inputs = new t_model_ports;
        outpad_model.inputs->dir = IN_PORT;
        outpad_model.inputs->name = vtr::strdup("outpad");
        outpad_model.inputs->next = nullptr;
        outpad_model.inputs->size = 1;
        outpad_model.inputs->min_size = 1;
        outpad_model.inputs->index = 0;
        outpad_model.inputs->is_clock = false;

        outpad_model.instances = nullptr;

        outpad_model.outputs = nullptr;
    }

    //LATCH
    {
        LogicalModelId latch_model_id = create_logical_model(MODEL_LATCH);
        VTR_ASSERT_OPT(latch_model_id == MODEL_LATCH_ID);
        t_model& latch_model = get_model(latch_model_id);
        t_model_ports* latch_model_input_port_1 = new t_model_ports;
        t_model_ports* latch_model_input_port_2 = new t_model_ports;

        latch_model.inputs = latch_model_input_port_1;
        latch_model_input_port_1->dir = IN_PORT;
        latch_model_input_port_1->name = vtr::strdup("D");
        latch_model_input_port_1->next = latch_model_input_port_2;
        latch_model_input_port_1->size = 1;
        latch_model_input_port_1->min_size = 1;
        latch_model_input_port_1->index = 0;
        latch_model_input_port_1->is_clock = false;
        latch_model_input_port_1->clock = "clk";

        latch_model_input_port_2->dir = IN_PORT;
        latch_model_input_port_2->name = vtr::strdup("clk");
        latch_model_input_port_2->next = nullptr;
        latch_model_input_port_2->size = 1;
        latch_model_input_port_2->min_size = 1;
        latch_model_input_port_2->index = 0;
        latch_model_input_port_2->is_clock = true;

        latch_model.instances = nullptr;

        latch_model.outputs = new t_model_ports;
        latch_model.outputs->dir = OUT_PORT;
        latch_model.outputs->name = vtr::strdup("Q");
        latch_model.outputs->next = nullptr;
        latch_model.outputs->size = 1;
        latch_model.outputs->min_size = 1;
        latch_model.outputs->index = 0;
        latch_model.outputs->is_clock = false;
        latch_model.outputs->clock = "clk";
    }

    //NAMES
    {
        LogicalModelId names_model_id = create_logical_model(MODEL_NAMES);
        VTR_ASSERT_OPT(names_model_id == MODEL_NAMES_ID);
        t_model& names_model = get_model(names_model_id);

        names_model.inputs = new t_model_ports;
        names_model.inputs->dir = IN_PORT;
        names_model.inputs->name = vtr::strdup("in");
        names_model.inputs->next = nullptr;
        names_model.inputs->size = 1;
        names_model.inputs->min_size = 1;
        names_model.inputs->index = 0;
        names_model.inputs->is_clock = false;
        names_model.inputs->combinational_sink_ports = {"out"};

        names_model.instances = nullptr;

        names_model.outputs = new t_model_ports;
        names_model.outputs->dir = OUT_PORT;
        names_model.outputs->name = vtr::strdup("out");
        names_model.outputs->next = nullptr;
        names_model.outputs->size = 1;
        names_model.outputs->min_size = 1;
        names_model.outputs->index = 0;
        names_model.outputs->is_clock = false;
    }

    // Checks to ensure that all library models have been successfully created
    // and the number of library models matches the NUM_MODELS_IN_LIBRARY variable.
    VTR_ASSERT_MSG(all_models().size() == library_models().size(),
                   "The only models that have been created should be the library models");
    VTR_ASSERT_MSG(library_models().size() == NUM_MODELS_IN_LIBRARY,
                   "The number of models in the library must be the expected number");
}

void LogicalModels::free_model_data(t_model& model) {
    free_arch_model_ports(model.inputs);
    free_arch_model_ports(model.outputs);

    vtr::t_linked_vptr* vptr = model.pb_types;
    while (vptr) {
        vtr::t_linked_vptr* vptr_prev = vptr;
        vptr = vptr->next;
        vtr::free(vptr_prev);
    }

    if (model.instances)
        vtr::free(model.instances);
    vtr::free(model.name);
}

static void free_arch_model_ports(t_model_ports* model_ports) {
    t_model_ports* model_port = model_ports;
    while (model_port) {
        model_port = free_arch_model_port(model_port);
    }
}

static t_model_ports* free_arch_model_port(t_model_ports* model_port) {
    if (!model_port) return nullptr;

    t_model_ports* next_port = model_port->next;

    vtr::free(model_port->name);
    delete model_port;

    return next_port;
}
