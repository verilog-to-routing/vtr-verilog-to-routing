/************ Defines and types shared by all route files ********************/
/* IMPORTANT:
 * The following preprocessing flags are added to 
 * avoid compilation error when this headers are included in more than 1 times 
 */
#ifndef ROUTER_TRACEBACK_H
#define ROUTER_TRACEBACK_H

/*
 * Notes in include header files in a head file 
 * Only include the neccessary header files 
 * that is required by the data types in the function/class declarations!
 */
/* Header files should be included in a sequence */
/* Standard header files required go first */

/* use a namespace for the functions */
/* Namespace declaration */
/* All the routers should be placed in the namespace of router
 * A specific router may have it own namespace under the router namespace
 * To call a function in a router, function need a prefix of router::<your_router_namespace>::
 * This will ease the development in modularization.
 * The concern is to minimize/avoid conflicts between data type names as much as possible
 * Also, this will keep function names as short as possible.
 */

/* The trace struct is defined in vpr_types.h
 * If trace is only used by routers, 
 * we can move the struct definition here */
struct t_trace; //Forward declaration

namespace router { 


struct t_traceback {
    t_traceback() = default;
    ~t_traceback();

    t_traceback(const t_traceback&);
    t_traceback(t_traceback&&);
    t_traceback operator=(t_traceback);

    friend void swap(t_traceback& first, t_traceback& second);

    t_trace* head = nullptr;
    t_trace* tail = nullptr;
};

} /* end of namespace router */

#endif
