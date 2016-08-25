#ifndef	READ_BLIF_H
#define READ_BLIF_H

void read_and_process_blif(const char *blif_file,
		bool sweep_hanging_nets_and_inputs, bool absorb_buffer_luts,           
        const t_model *user_models,
		const t_model *library_models, bool read_activity_file,
		char * activity_file);
void echo_input(const char *blif_file, const char *echo_file, const t_model *library_models);
void dum_parse(char *buf);

#endif /*READ_BLIF_H*/
