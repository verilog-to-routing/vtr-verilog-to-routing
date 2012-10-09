#include <stdlib.h>

#include "ace.h"
#include "io_ace.h"

#include "abc.h"

char * hdl_name_ptr = NULL;

bool check_if_fanout_is_po(Abc_Ntk_t * ntk, Abc_Obj_t * obj) {
	Abc_Obj_t * fanout;
	int i;

	if (Abc_ObjIsCo(obj)) {
		return FALSE;
	}

	Abc_ObjForEachFanout(obj, fanout, i)
	{
		if (Abc_ObjIsPo(fanout)) {
			return TRUE;
		}
	}

	return FALSE;
}

void ace_io_print_activity(Abc_Ntk_t * ntk, FILE * fp) {
	Abc_Obj_t * obj;
	Abc_Obj_t * obj_new;
	int i;

	Abc_NtkForEachObj(ntk, obj, i)
	{
		assert(obj->pCopy);
		obj_new = obj->pCopy;

		Ace_Obj_Info_t * info = Ace_ObjInfo(obj);
		//Ace_Obj_Info_t * info = malloc(sizeof(Ace_Obj_Info_t));

		char * name = NULL;

		if (check_if_fanout_is_po(ntk, obj)) {
			//continue;
		}

		switch (Abc_ObjType(obj)) {

		case ABC_OBJ_PI:

			name = Abc_ObjName(Abc_ObjFanout0(obj_new));
			break;

		case ABC_OBJ_BO:
			name = Abc_ObjName(Abc_ObjFanout0(obj_new));
			break;

		case ABC_OBJ_LATCH:
		case ABC_OBJ_PO:
		case ABC_OBJ_BI:
			break;

		case ABC_OBJ_NODE:
			name = Abc_ObjName(Abc_ObjFanout0(obj_new));
			//name = Abc_ObjName(obj);
			break;

		default:
			//printf("Unkown Type: %d\n", Abc_ObjType(obj));
			//assert(0);
			break;
		}

		/*
		 if (Abc_ObjType(obj) == ABC_OBJ_BI || Abc_ObjType(obj) == ABC_OBJ_LATCH)
		 {
		 continue;
		 }

		 */

		if (name && strcmp(name, "unconn")) {
			if (fp != NULL) {
				//fprintf (fp, "%d-%d %s\n", Abc_ObjId(obj), Abc_ObjType(obj), name);
				//fprintf (fp, "%d-%d %s %f %f %f\n", Abc_ObjId(obj), Abc_ObjType(obj), name, info->static_prob, info->switch_prob, info->switch_act);
				fprintf(fp, "%s %f %f\n", name, info->static_prob,
						info->switch_act);

			} else {
				printf("%s %f %f\n", Abc_ObjName(obj), info->static_prob,
						info->switch_act);
			}
		}
	}
}

int ace_io_parse_argv(int argc, char ** argv, FILE ** BLIF, FILE ** IN_ACT,
		FILE ** OUT_ACT, char * blif_file_name, char * new_blif_file_name,
		ace_pi_format_t * pi_format, double *p, double * d) {
	int i;
	char option;

	if (argc <= 1) {
		printf("Error: no parameters specified.\n");
		ace_io_print_usage();
		exit(1);
	}

	i = 1;
	while (i < argc) {
		if (argv[i][0] == '-') {
			option = argv[i][1];
			i++;
			switch (option) {
			case 'b':
				*BLIF = fopen(argv[i], "r");
				strncpy(blif_file_name, argv[i], BLIF_FILE_NAME_LEN - 1);
				blif_file_name[strlen(argv[i])] = '\0';
				break;
			case 'n':
				strncpy(new_blif_file_name, argv[i], BLIF_FILE_NAME_LEN - 1);
				new_blif_file_name[strlen(argv[i])] = '\0';
				break;
			case 'o':
				*OUT_ACT = fopen(argv[i], "w");
				break;
			case 'a':
				*pi_format = ACE_ACT;
				*IN_ACT = fopen(argv[i], "r");
				break;
			case 'v':
				*pi_format = ACE_VEC;
				*IN_ACT = fopen(argv[i], "r");
				break;
			case 'p':
				*pi_format = ACE_PD;
				*p = atof(argv[i]);
				break;
			case 'd':
				*pi_format = ACE_PD;
				*d = atof(argv[i]);
				break;
			default:
				ace_io_print_usage();
				exit(1);
			}
		}
		i++;
	}

	if (*BLIF == NULL) {
		ace_io_print_usage();
		exit(1);
	}
	return 0;
}

void ace_io_print_usage() {
	(void) fprintf(stderr, "usage: ace\n");

	(void) fprintf(stderr, "                                --+\n");
	(void) fprintf(stderr, "    -b [input circuitname.blif]   | required\n");
	(void) fprintf(stderr, "    -n [new circuitname.blif]     |\n");
	(void) fprintf(stderr, "                                --+\n");
	(void) fprintf(stderr, "\n");
	(void) fprintf(stderr, "                                --+\n");
	(void) fprintf(stderr, "    -o [output activity filename] | optional\n");
	(void) fprintf(stderr, "                                --+\n");
	(void) fprintf(stderr, "\n");
	(void) fprintf(stderr, "                                --+\n");
	(void) fprintf(stderr, "    -a [input activity filename]  | optional\n");
	(void) fprintf(stderr, "          or                      |\n");
	(void) fprintf(stderr, "    -v [input vector filename]    |\n");
	(void) fprintf(stderr, "          or                      |\n");
	(void) fprintf(stderr, "    -p [PI static probability]    |\n");
	(void) fprintf(stderr, "    -d [PI switching activity]    |\n");
	(void) fprintf(stderr, "                                --+\n");
}

int ace_io_read_activity(Abc_Ntk_t * ntk, FILE * in_file_desc,
		ace_pi_format_t pi_format, double p, double d, const char * clk_name) {
	int error = 0;
	int i;
	Abc_Obj_t * obj_ptr;
	Ace_Obj_Info_t * info;
	Nm_Man_t * name_manager = ntk->pManName;
	int num_Pi = Abc_NtkPiNum(ntk);

	printf("Name Manager Entries: %d\n", Nm_ManNumEntries(name_manager));

	/*
	 int clk_pi_obj_id = Nm_ManFindIdByName(name_manager, (char *) clk_name, ABC_OBJ_PI);
	 printf ("Clk PI ID: %d\n", clk_pi_obj_id);

	 Abc_Obj_t * clk_obj_ptr;
	 clk_obj_ptr = Abc_NtkObj(ntk, clk_pi_obj_id);
	 printf("Clock Fanouts: %d\n", clk_obj_ptr->vFanouts.nSize);
	 */

	Vec_Ptr_t * node_vec;
	node_vec = Abc_NtkDfsSeq(ntk);

	// Initialize node information structure
	Vec_PtrForEachEntry(node_vec, obj_ptr, i)
	{
		info = Ace_ObjInfo(obj_ptr);
		info->static_prob = ACE_OPEN;
		info->switch_prob = ACE_OPEN;
		info->switch_act = ACE_OPEN;
	}

	//info = Ace_InfoPtrGet(clk_obj_ptr);
	//info->static_prob = 0.5;
	//info->switch_prob = 1.0;
	//info->switch_act = 1.0;

	if (in_file_desc == NULL) {
		if (pi_format == ACE_ACT || pi_format == ACE_VEC) {
			printf("Cannot open input file\n");
			error = ACE_ERROR;
		} else {
			assert(p >= 0.0 && p <= 1.0);
			assert(d >= 0.0 && d <= 1.0);
			assert(d <= 2.0 * p);
			assert(d <= 2.0 * (1.0 - p));

			Abc_NtkForEachPi(ntk, obj_ptr, i)
			{
				info = Ace_ObjInfo(obj_ptr);
				info->static_prob = p;
				info->switch_prob = d;
				info->switch_act = d;
			}
		}
	} else {
		char line[ACE_CHAR_BUFFER_SIZE];

		if (pi_format == ACE_ACT) {
			char pi_name[ACE_CHAR_BUFFER_SIZE];
			double static_prob, switch_prob;
			Abc_Obj_t * pi_obj_ptr;
			int pi_obj_id;

			printf("Reading activity file...\n");

			// Read real PIs activity values from file
			fgets(line, ACE_CHAR_BUFFER_SIZE, in_file_desc);
			while (!feof(in_file_desc)) {
				sscanf(line, "%s %lf %lf\n", pi_name, &static_prob,
						&switch_prob);

				pi_obj_id = Nm_ManFindIdByName(name_manager, pi_name,
						ABC_OBJ_PI);
				if (pi_obj_id == -1) {
					printf("Primary input %s does not exist\n", pi_name);
					error = ACE_ERROR;
					break;
				}
				pi_obj_ptr = Abc_NtkObj(ntk, pi_obj_id);

				assert(static_prob >= 0.0 && static_prob <= 1.0);
				assert(switch_prob >= 0.0 && switch_prob <= 1.0);

				info = Ace_ObjInfo(pi_obj_ptr);
				info->static_prob = static_prob;
				info->switch_prob = switch_prob;
				info->switch_act = switch_prob;

				fgets(line, ACE_CHAR_BUFFER_SIZE, in_file_desc);
			}
		} else if (pi_format == ACE_VEC) {
			printf("Reading vector file...\n");

			int num_vec = 0;
			int * high;
			int * toggles;
			int * current;
			char vector[ACE_CHAR_BUFFER_SIZE];

			fgets(line, ACE_CHAR_BUFFER_SIZE, in_file_desc);
			while (!feof(in_file_desc)) {
				fgets(line, ACE_CHAR_BUFFER_SIZE, in_file_desc);
				num_vec++;
			}
			Abc_NtkForEachPi(ntk, obj_ptr, i)
			{
				info = Ace_ObjInfo(obj_ptr);
				info->values = (int *) malloc(num_vec * sizeof(int));
			}
			fseek(in_file_desc, 0, SEEK_SET);

			high = (int *) malloc(num_Pi * sizeof(int));
			toggles = (int *) malloc(num_Pi * sizeof(int));
			current = (int *) malloc(num_Pi * sizeof(int));

			num_vec = 0;
			fgets(line, ACE_CHAR_BUFFER_SIZE, in_file_desc);
			while (!feof(in_file_desc)) {
				sscanf(line, "%s\n", vector);

				if (strlen(vector) != num_Pi) {
					printf(
							"Error: vector length (%d) doesn't match number of inputs (%d)\n",
							strlen(vector), num_Pi);
					error = ACE_ERROR;
					break;
				}
				assert(strlen(vector) == num_Pi);

				if (num_vec == 0) {
					Abc_NtkForEachPi(ntk, obj_ptr, i)
					{
						high[i] = (vector[i] == '1');
						toggles[i] = 0;
						current[i] = (vector[i] == '1');
					}
				} else {
					Abc_NtkForEachPi(ntk, obj_ptr, i)
					{
						high[i] += (vector[i] == '1');
						if ((vector[i] == '0' && current[i])
								|| (vector[i] == '1' && !current[i])) {
							toggles[i]++;
						}
						current[i] = (vector[i] == '1');
					}
				}

				Abc_NtkForEachPi(ntk, obj_ptr, i)
				{
					info = Ace_ObjInfo(obj_ptr);
					info->values[num_vec] = (vector[i] == '1');
				}

				fgets(line, ACE_CHAR_BUFFER_SIZE, in_file_desc);
				num_vec++;
			}

			if (!error) {
				Abc_NtkForEachPi(ntk, obj_ptr, i)
				{
					info = Ace_ObjInfo(obj_ptr);
					info->static_prob = (double) high[i] / (double) num_vec;
					info->static_prob = MAX(0.0, info->static_prob);
					info->static_prob = MIN(1.0, info->static_prob);

					info->switch_prob = (double) toggles[i] / (double) num_vec;
					info->switch_prob = MAX(0.0, info->switch_prob);
					info->switch_prob = MIN(1.0, info->switch_prob);

					info->switch_act = info->switch_prob;
				}
			}

			free(toggles);
			free(high);
			free(current);
		} else {
			printf("Error: Unkown activity file format.\n");
			error = ACE_ERROR;
		}
	}

	Vec_PtrFree(node_vec);

	return error;
}

