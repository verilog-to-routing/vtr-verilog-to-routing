/*
 * Revision Control Information
 *
 * $Source$
 * $Author$
 * $Revision$
 * $Date$
 *
 */
#ifndef ABC__misc__espresso__sparse_h
#define ABC__misc__espresso__sparse_h


ABC_NAMESPACE_HEADER_START


/*
 *  sparse.h -- sparse matrix package header file
 */

typedef struct sm_element_struct sm_element;
typedef struct sm_row_struct sm_row;
typedef struct sm_col_struct sm_col;
typedef struct sm_matrix_struct sm_matrix;


/*
 *  sparse matrix element
 */
struct sm_element_struct {
    int row_num;        /* row number of this element */
    int col_num;        /* column number of this element */
    sm_element *next_row;    /* next row in this column */
    sm_element *prev_row;    /* previous row in this column */
    sm_element *next_col;    /* next column in this row */
    sm_element *prev_col;    /* previous column in this row */
    char *user_word;        /* user-defined word */
};


/*
 *  row header
 */
struct sm_row_struct {
    int row_num;        /* the row number */
    int length;            /* number of elements in this row */
    int flag;            /* user-defined word */
    sm_element *first_col;    /* first element in this row */
    sm_element *last_col;    /* last element in this row */
    sm_row *next_row;        /* next row (in sm_matrix linked list) */
    sm_row *prev_row;        /* previous row (in sm_matrix linked list) */
    char *user_word;        /* user-defined word */
};


/*
 *  column header
 */
struct sm_col_struct {
    int col_num;        /* the column number */
    int length;            /* number of elements in this column */
    int flag;            /* user-defined word */
    sm_element *first_row;    /* first element in this column */
    sm_element *last_row;    /* last element in this column */
    sm_col *next_col;        /* next column (in sm_matrix linked list) */
    sm_col *prev_col;        /* prev column (in sm_matrix linked list) */
    char *user_word;        /* user-defined word */
};


/*
 *  A sparse matrix
 */
struct sm_matrix_struct {
    sm_row **rows;        /* pointer to row headers (by row #) */
    int rows_size;        /* alloc'ed size of above array */
    sm_col **cols;        /* pointer to column headers (by col #) */
    int cols_size;        /* alloc'ed size of above array */
    sm_row *first_row;        /* first row (linked list of all rows) */
    sm_row *last_row;        /* last row (linked list of all rows) */
    int nrows;            /* number of rows */
    sm_col *first_col;        /* first column (linked list of columns) */
    sm_col *last_col;        /* last column (linked list of columns) */
    int ncols;            /* number of columns */
    char *user_word;        /* user-defined word */
};


#define sm_get_col(A, colnum)    \
    (((colnum) >= 0 && (colnum) < (A)->cols_size) ? \
    (A)->cols[colnum] : (sm_col *) 0)

#define sm_get_row(A, rownum)    \
    (((rownum) >= 0 && (rownum) < (A)->rows_size) ? \
    (A)->rows[rownum] : (sm_row *) 0)

#define sm_foreach_row(A, prow)    \
    for(prow = A->first_row; prow != 0; prow = prow->next_row)

#define sm_foreach_col(A, pcol)    \
    for(pcol = A->first_col; pcol != 0; pcol = pcol->next_col)

#define sm_foreach_row_element(prow, p)    \
    for(p = prow->first_col; p != 0; p = p->next_col)

#define sm_foreach_col_element(pcol, p)    \
    for(p = pcol->first_row; p != 0; p = p->next_row)

#define sm_put(x, val) \
    (x->user_word = (char *) val)

#define sm_get(type, x) \
    ((type) (x->user_word))

extern sm_matrix *sm_alloc(), *sm_alloc_size(), *sm_dup();
extern void sm_free(), sm_delrow(), sm_delcol(), sm_resize();
extern void sm_write(), sm_print(), sm_dump(), sm_cleanup();
extern void sm_copy_row(), sm_copy_col();
extern void sm_remove(), sm_remove_element();
extern sm_element *sm_insert(), *sm_find();
extern sm_row *sm_longest_row();
extern sm_col *sm_longest_col();
extern int sm_read(), sm_read_compressed();

extern sm_row *sm_row_alloc(), *sm_row_dup(), *sm_row_and();
extern void sm_row_free(), sm_row_remove(), sm_row_print();
extern sm_element *sm_row_insert(), *sm_row_find();
extern int sm_row_contains(), sm_row_intersects();
extern int sm_row_compare(), sm_row_hash();

extern sm_col *sm_col_alloc(), *sm_col_dup(), *sm_col_and();
extern void sm_col_free(), sm_col_remove(), sm_col_print();
extern sm_element *sm_col_insert(), *sm_col_find();
extern int sm_col_contains(), sm_col_intersects();
extern int sm_col_compare(), sm_col_hash();

extern int sm_row_dominance(), sm_col_dominance(), sm_block_partition();



ABC_NAMESPACE_HEADER_END

#endif
