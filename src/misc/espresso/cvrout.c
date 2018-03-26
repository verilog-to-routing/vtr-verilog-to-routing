/*
 * Revision Control Information
 *
 * $Source$
 * $Author$
 * $Revision$
 * $Date$
 *
 */
/*
    module: cvrout.c
    purpose: cube and cover output routines
*/

#include "espresso.h"

ABC_NAMESPACE_IMPL_START


void fprint_pla(fp, PLA, output_type)
INOUT FILE *fp;
IN pPLA PLA;
IN int output_type;
{
    int num;
    register pcube last, p;

    if ((output_type & CONSTRAINTS_type) != 0) {
    output_symbolic_constraints(fp, PLA, 0);
    output_type &= ~ CONSTRAINTS_type;
    if (output_type == 0) {
        return;
    }
    }

    if ((output_type & SYMBOLIC_CONSTRAINTS_type) != 0) {
    output_symbolic_constraints(fp, PLA, 1);
    output_type &= ~ SYMBOLIC_CONSTRAINTS_type;
    if (output_type == 0) {
        return;
    }
    }

    if (output_type == PLEASURE_type) {
    pls_output(PLA);
    } else if (output_type == EQNTOTT_type) {
    eqn_output(PLA);
    } else if (output_type == KISS_type) {
    kiss_output(fp, PLA);
    } else {
    fpr_header(fp, PLA, output_type);

    num = 0;
    if (output_type & F_type) num += (PLA->F)->count;
    if (output_type & D_type) num += (PLA->D)->count;
    if (output_type & R_type) num += (PLA->R)->count;
    (void) fprintf(fp, ".p %d\n", num);

    /* quick patch 01/17/85 to support TPLA ! */
    if (output_type == F_type) {
        foreach_set(PLA->F, last, p) {
        print_cube(fp, p, "01");
        }
        (void) fprintf(fp, ".e\n");
    } else {
        if (output_type & F_type) {
        foreach_set(PLA->F, last, p) {
            print_cube(fp, p, "~1");
        }
        }
        if (output_type & D_type) {
        foreach_set(PLA->D, last, p) {
            print_cube(fp, p, "~2");
        }
        }
        if (output_type & R_type) {
        foreach_set(PLA->R, last, p) {
            print_cube(fp, p, "~0");
        }
        }
        (void) fprintf(fp, ".end\n");
    }
    }
}

void fpr_header(fp, PLA, output_type)
FILE *fp;
pPLA PLA;
int output_type;
{
    register int i, var;
    int first, last;

    /* .type keyword gives logical type */
    if (output_type != F_type) {
    (void) fprintf(fp, ".type ");
    if (output_type & F_type) putc('f', fp);
    if (output_type & D_type) putc('d', fp);
    if (output_type & R_type) putc('r', fp);
    putc('\n', fp);
    }

    /* Check for binary or multiple-valued labels */
    if (cube.num_mv_vars <= 1) {
    (void) fprintf(fp, ".i %d\n", cube.num_binary_vars);
    if (cube.output != -1)
        (void) fprintf(fp, ".o %d\n", cube.part_size[cube.output]);
    } else {
    (void) fprintf(fp, ".mv %d %d", cube.num_vars, cube.num_binary_vars);
    for(var = cube.num_binary_vars; var < cube.num_vars; var++)
        (void) fprintf(fp, " %d", cube.part_size[var]);
    (void) fprintf(fp, "\n");
    }

    /* binary valued labels */
    if (PLA->label != NIL(char *) && PLA->label[1] != NIL(char)
        && cube.num_binary_vars > 0) {
    (void) fprintf(fp, ".ilb");
    for(var = 0; var < cube.num_binary_vars; var++)
      /* see (NIL) OUTLABELS comment below */
      if(INLABEL(var) == NIL(char)){
        (void) fprintf(fp, " (null)");
      }
      else{
        (void) fprintf(fp, " %s", INLABEL(var));
      }
    putc('\n', fp);
    }

    /* output-part (last multiple-valued variable) labels */
    if (PLA->label != NIL(char *) &&
        PLA->label[cube.first_part[cube.output]] != NIL(char)
        && cube.output != -1) {
    (void) fprintf(fp, ".ob");
    for(i = 0; i < cube.part_size[cube.output]; i++)
      /* (NIL) OUTLABELS caused espresso to segfault under solaris */
      if(OUTLABEL(i) == NIL(char)){
        (void) fprintf(fp, " (null)");
      }
      else{
        (void) fprintf(fp, " %s", OUTLABEL(i));
      }
    putc('\n', fp);
    }

    /* multiple-valued labels */
    for(var = cube.num_binary_vars; var < cube.num_vars-1; var++) {
    first = cube.first_part[var];
    last = cube.last_part[var];
    if (PLA->label != NULL && PLA->label[first] != NULL) {
        (void) fprintf(fp, ".label var=%d", var);
        for(i = first; i <= last; i++) {
        (void) fprintf(fp, " %s", PLA->label[i]);
        }
        putc('\n', fp);
    }
    }

    if (PLA->phase != (pcube) NULL) {
    first = cube.first_part[cube.output];
    last = cube.last_part[cube.output];
    (void) fprintf(fp, "#.phase ");
    for(i = first; i <= last; i++)
        putc(is_in_set(PLA->phase,i) ? '1' : '0', fp);
    (void) fprintf(fp, "\n");
    }
}

void pls_output(PLA)
IN pPLA PLA;
{
    register pcube last, p;

    (void) printf(".option unmerged\n");
    makeup_labels(PLA);
    pls_label(PLA, stdout);
    pls_group(PLA, stdout);
    (void) printf(".p %d\n", PLA->F->count);
    foreach_set(PLA->F, last, p) {
    print_expanded_cube(stdout, p, PLA->phase);
    }
    (void) printf(".end\n");
}


void pls_group(PLA, fp)
pPLA PLA;
FILE *fp;
{
    int var, i, col, len;

    (void) fprintf(fp, "\n.group");
    col = 6;
    for(var = 0; var < cube.num_vars-1; var++) {
    (void) fprintf(fp, " ("), col += 2;
    for(i = cube.first_part[var]; i <= cube.last_part[var]; i++) {
        len = strlen(PLA->label[i]);
        if (col + len > 75)
        (void) fprintf(fp, " \\\n"), col = 0;
        else if (i != 0)
        putc(' ', fp), col += 1;
        (void) fprintf(fp, "%s", PLA->label[i]), col += len;
    }
    (void) fprintf(fp, ")"), col += 1;
    }
    (void) fprintf(fp, "\n");
}


void pls_label(PLA, fp)
pPLA PLA;
FILE *fp;
{
    int var, i, col, len;

    (void) fprintf(fp, ".label");
    col = 6;
    for(var = 0; var < cube.num_vars; var++)
    for(i = cube.first_part[var]; i <= cube.last_part[var]; i++) {
        len = strlen(PLA->label[i]);
        if (col + len > 75)
        (void) fprintf(fp, " \\\n"), col = 0;
        else
        putc(' ', fp), col += 1;
        (void) fprintf(fp, "%s", PLA->label[i]), col += len;
    }
}



/*
    eqntott output mode -- output algebraic equations
*/
void eqn_output(PLA)
pPLA PLA;
{
    register pcube p, last;
    register int i, var, col, len;
    int x;
    bool firstand, firstor;

    if (cube.output == -1)
    fatal("Cannot have no-output function for EQNTOTT output mode");
    if (cube.num_mv_vars != 1)
    fatal("Must have binary-valued function for EQNTOTT output mode");
    makeup_labels(PLA);

    /* Write a single equation for each output */
    for(i = 0; i < cube.part_size[cube.output]; i++) {
    (void) printf("%s = ", OUTLABEL(i));
    col = strlen(OUTLABEL(i)) + 3;
    firstor = TRUE;

    /* Write product terms for each cube in this output */
    foreach_set(PLA->F, last, p)
        if (is_in_set(p, i + cube.first_part[cube.output])) {
        if (firstor)
            (void) printf("("), col += 1;
        else
            (void) printf(" | ("), col += 4;
        firstor = FALSE;
        firstand = TRUE;

        /* print out a product term */
        for(var = 0; var < cube.num_binary_vars; var++)
            if ((x=GETINPUT(p, var)) != DASH) {
            len = strlen(INLABEL(var));
            if (col+len > 72)
                (void) printf("\n    "), col = 4;
            if (! firstand)
                (void) printf("&"), col += 1;
            firstand = FALSE;
            if (x == ZERO)
                (void) printf("!"), col += 1;
            (void) printf("%s", INLABEL(var)), col += len;
            }
        (void) printf(")"), col += 1;
        }
    (void) printf(";\n\n");
    }
}


char *fmt_cube(c, out_map, s)
register pcube c;
register char *out_map, *s;
{
    register int i, var, last, len = 0;

    for(var = 0; var < cube.num_binary_vars; var++) {
    s[len++] = "?01-" [GETINPUT(c, var)];
    }
    for(var = cube.num_binary_vars; var < cube.num_vars - 1; var++) {
    s[len++] = ' ';
    for(i = cube.first_part[var]; i <= cube.last_part[var]; i++) {
        s[len++] = "01" [is_in_set(c, i) != 0];
    }
    }
    if (cube.output != -1) {
    last = cube.last_part[cube.output];
    s[len++] = ' ';
    for(i = cube.first_part[cube.output]; i <= last; i++) {
        s[len++] = out_map [is_in_set(c, i) != 0];
    }
    }
    s[len] = '\0';
    return s;
}


void print_cube(fp, c, out_map)
register FILE *fp;
register pcube c;
register char *out_map;
{
    register int i, var, ch;
    int last;

    for(var = 0; var < cube.num_binary_vars; var++) {
    ch = "?01-" [GETINPUT(c, var)];
    putc(ch, fp);
    }
    for(var = cube.num_binary_vars; var < cube.num_vars - 1; var++) {
    putc(' ', fp);
    for(i = cube.first_part[var]; i <= cube.last_part[var]; i++) {
        ch = "01" [is_in_set(c, i) != 0];
        putc(ch, fp);
    }
    }
    if (cube.output != -1) {
    last = cube.last_part[cube.output];
    putc(' ', fp);
    for(i = cube.first_part[cube.output]; i <= last; i++) {
        ch = out_map [is_in_set(c, i) != 0];
        putc(ch, fp);
    }
    }
    putc('\n', fp);
}


void print_expanded_cube(fp, c, phase)
register FILE *fp;
register pcube c;
pcube phase;
{
    register int i, var, ch;
    char *out_map;

    for(var = 0; var < cube.num_binary_vars; var++) {
    for(i = cube.first_part[var]; i <= cube.last_part[var]; i++) {
        ch = "~1" [is_in_set(c, i) != 0];
        putc(ch, fp);
    }
    }
    for(var = cube.num_binary_vars; var < cube.num_vars - 1; var++) {
    for(i = cube.first_part[var]; i <= cube.last_part[var]; i++) {
        ch = "1~" [is_in_set(c, i) != 0];
        putc(ch, fp);
    }
    }
    if (cube.output != -1) {
    var = cube.num_vars - 1;
    putc(' ', fp);
    for(i = cube.first_part[var]; i <= cube.last_part[var]; i++) {
        if (phase == (pcube) NULL || is_in_set(phase, i)) {
        out_map = "~1";
        } else {
        out_map = "~0";
        }
        ch = out_map[is_in_set(c, i) != 0];
        putc(ch, fp);
    }
    }
    putc('\n', fp);
}


char *pc1(c) pcube c;
{static char s1[256];return fmt_cube(c, "01", s1);}
char *pc2(c) pcube c;
{static char s2[256];return fmt_cube(c, "01", s2);}


void debug_print(T, name, level)
pcube *T;
char *name;
int level;
{
    register pcube *T1, p, temp;
    register int cnt;

    cnt = CUBELISTSIZE(T);
    temp = new_cube();
    if (verbose_debug && level == 0)
    (void) printf("\n");
    (void) printf("%s[%d]: ord(T)=%d\n", name, level, cnt);
    if (verbose_debug) {
    (void) printf("cofactor=%s\n", pc1(T[0]));
    for(T1 = T+2, cnt = 1; (p = *T1++) != (pcube) NULL; cnt++)
        (void) printf("%4d. %s\n", cnt, pc1(set_or(temp, p, T[0])));
    }
    free_cube(temp);
}


void debug1_print(T, name, num)
pcover T;
char *name;
int num;
{
    register int cnt = 1;
    register pcube p, last;

    if (verbose_debug && num == 0)
    (void) printf("\n");
    (void) printf("%s[%d]: ord(T)=%d\n", name, num, T->count);
    if (verbose_debug)
    foreach_set(T, last, p)
        (void) printf("%4d. %s\n", cnt++, pc1(p));
}


void cprint(T)
pcover T;
{
    register pcube p, last;

    foreach_set(T, last, p)
    (void) printf("%s\n", pc1(p));
}


void makeup_labels(PLA)
pPLA PLA;
{
    int var, i, ind;

    if (PLA->label == (char **) NULL)
    PLA_labels(PLA);

    for(var = 0; var < cube.num_vars; var++)
    for(i = 0; i < cube.part_size[var]; i++) {
        ind = cube.first_part[var] + i;
        if (PLA->label[ind] == (char *) NULL) {
        PLA->label[ind] = ALLOC(char, 15);
        if (var < cube.num_binary_vars)
            if ((i % 2) == 0)
            (void) sprintf(PLA->label[ind], "v%d.bar", var);
            else
            (void) sprintf(PLA->label[ind], "v%d", var);
        else
            (void) sprintf(PLA->label[ind], "v%d.%d", var, i);
        }
    }
}


void kiss_output(fp, PLA)
FILE *fp;
pPLA PLA;
{
    register pset last, p;

    foreach_set(PLA->F, last, p) {
    kiss_print_cube(fp, PLA, p, "~1");
    }
    foreach_set(PLA->D, last, p) {
    kiss_print_cube(fp, PLA, p, "~2");
    }
}


void kiss_print_cube(fp, PLA, p, out_string)
FILE *fp;
pPLA PLA;
pcube p;
char *out_string;
{
    register int i, var;
    int part, x;

    for(var = 0; var < cube.num_binary_vars; var++) {
    x = "?01-" [GETINPUT(p, var)];
    putc(x, fp);
    }

    for(var = cube.num_binary_vars; var < cube.num_vars - 1; var++) {
    putc(' ', fp);
    if (setp_implies(cube.var_mask[var], p)) {
        putc('-', fp);
    } else {
        part = -1;
        for(i = cube.first_part[var]; i <= cube.last_part[var]; i++) {
        if (is_in_set(p, i)) {
            if (part != -1) {
            fatal("more than 1 part in a symbolic variable\n");
            }
            part = i;
        }
        }
        if (part == -1) {
        putc('~', fp);    /* no parts, hope its an output ... */
        } else {
        (void) fputs(PLA->label[part], fp);
        }
    }
    }

    if ((var = cube.output) != -1) {
    putc(' ', fp);
    for(i = cube.first_part[var]; i <= cube.last_part[var]; i++) {
        x = out_string [is_in_set(p, i) != 0];
        putc(x, fp);
    }
    }

    putc('\n', fp);
}

void output_symbolic_constraints(fp, PLA, output_symbolic)
FILE *fp;
pPLA PLA;
int output_symbolic;
{
    pset_family A;
    register int i, j;
    int size, var, npermute, *permute, *weight, noweight;

    if ((cube.num_vars - cube.num_binary_vars) <= 1) {
    return;
    }
    makeup_labels(PLA);

    for(var=cube.num_binary_vars; var < cube.num_vars-1; var++) {

    /* pull out the columns for variable "var" */
    npermute = cube.part_size[var];
    permute = ALLOC(int, npermute);
    for(i=0; i < npermute; i++) {
        permute[i] = cube.first_part[var] + i;
    }
    A = sf_permute(sf_save(PLA->F), permute, npermute);
    FREE(permute);


    /* Delete the singletons and the full sets */
    noweight = 0;
    for(i = 0; i < A->count; i++) {
        size = set_ord(GETSET(A,i));
        if (size == 1 || size == A->sf_size) {
        sf_delset(A, i--);
        noweight++;
        }
    }


    /* Count how many times each is duplicated */
    weight = ALLOC(int, A->count);
    for(i = 0; i < A->count; i++) {
        RESET(GETSET(A, i), COVERED);
    }
    for(i = 0; i < A->count; i++) {
        weight[i] = 0;
        if (! TESTP(GETSET(A,i), COVERED)) {
        weight[i] = 1;
        for(j = i+1; j < A->count; j++) {
            if (setp_equal(GETSET(A,i), GETSET(A,j))) {
            weight[i]++;
            SET(GETSET(A,j), COVERED);
            }
        }
        }
    }


    /* Print out the constraints */
    if (! output_symbolic) {
        (void) fprintf(fp,
        "# Symbolic constraints for variable %d (Numeric form)\n", var);
        (void) fprintf(fp, "# unconstrained weight = %d\n", noweight);
        (void) fprintf(fp, "num_codes=%d\n", cube.part_size[var]);
        for(i = 0; i < A->count; i++) {
        if (weight[i] > 0) {
            (void) fprintf(fp, "weight=%d: ", weight[i]);
            for(j = 0; j < A->sf_size; j++) {
            if (is_in_set(GETSET(A,i), j)) {
                (void) fprintf(fp, " %d", j);
            }
            }
            (void) fprintf(fp, "\n");
        }
        }
    } else {
        (void) fprintf(fp,
        "# Symbolic constraints for variable %d (Symbolic form)\n", var);
        for(i = 0; i < A->count; i++) {
        if (weight[i] > 0) {
            (void) fprintf(fp, "#   w=%d: (", weight[i]);
            for(j = 0; j < A->sf_size; j++) {
            if (is_in_set(GETSET(A,i), j)) {
                (void) fprintf(fp, " %s",
                PLA->label[cube.first_part[var]+j]);
            }
            }
            (void) fprintf(fp, " )\n");
        }
        }
        FREE(weight);
    }
    }
}
ABC_NAMESPACE_IMPL_END

