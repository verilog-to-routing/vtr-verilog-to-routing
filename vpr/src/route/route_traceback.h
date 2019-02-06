#ifndef VPR_TRACEBACK_H
#define VPR_TRACEBACK_H

struct t_trace; //Forward declaration

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

#endif
