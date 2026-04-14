#ifndef NO_GRAPHICS

#include "draw_debug.h"
#include "draw_global.h"
#include "vtr_expr_eval.h"

#include <QGroupBox>
#include <QScrollArea>

//keeps track of open windows to avoid reopenning windows that are already open
struct open_windows {
    bool debug_window = false;
    bool advanced_window = false;
};

//categorizes operators into boolean operators (&&, ||) and comperators (>=, <=, >, <, ==). struct is used when checking an expresion's validity
typedef enum operator_type_in_expression {
    BOOL_OP,
    COMP_OP

} op_type_in_expr;

//debugger global variables
class DrawDebuggerGlobals {
  public:
    std::vector<std::string> bp_labels; ///holds all breakpoint labels to be displayed in the GUI
    GtkWidget* bpGrid;                  ///holds the grid where all the labels are
    int bpList_row = -1;                ///keeps track of where to insert the next breakpoint label in the list
    open_windows openWindows;           ///keeps track of all open window (related to breakpoints)

    //destructor clears bp_labels to avoid memory leaks
    ~DrawDebuggerGlobals() {
        bp_labels.clear();
    }
};

//the global variable that holds all global variables related to breakpoint graphics
DrawDebuggerGlobals draw_debug_glob_vars;

//draws main debugger window
void draw_debug_window() {
    if (!draw_debug_glob_vars.openWindows.debug_window) {
        draw_debug_glob_vars.openWindows.debug_window = true;

        GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(window, "Debugger");
        gtk_window_set_position(window, GTK_WIN_POS_CENTER);

        GtkWidget* mainGrid = gtk_grid_new();
        gtk_widget_set_margin_top(mainGrid, 30);
        gtk_widget_set_margin_bottom(mainGrid, 30);
        gtk_widget_set_margin_start(mainGrid, 30);
        gtk_widget_set_margin_end(mainGrid, 20);

        GtkWidget* placerOpts = gtk_label_new(nullptr);
        gtk_label_set_markup((GtkLabel*)placerOpts, "<b>Placer Options</b>");
        gtk_widget_set_margin_bottom(placerOpts, 10);
        GtkWidget* routerOpts = gtk_label_new(nullptr);
        gtk_label_set_markup((GtkLabel*)routerOpts, "<b>Router Options</b>");
        gtk_widget_set_margin_bottom(routerOpts, 10);
        gtk_widget_set_margin_top(routerOpts, 30);
        GtkWidget* bplist = gtk_label_new(nullptr);
        gtk_label_set_markup((GtkLabel*)bplist, "<b>List of Breakpoints</b>");
        gtk_widget_set_margin_bottom(bplist, 10);
        gtk_widget_set_margin_top(bplist, 30);
        GtkWidget* movesLabel = gtk_label_new("Number of moves to proceed");
        gtk_widget_set_halign(movesLabel, GTK_ALIGN_END);
        gtk_widget_set_margin_end(movesLabel, 8);
        GtkWidget* tempsLabel = gtk_label_new("Temperatures to proceed:");
        gtk_widget_set_halign(tempsLabel, GTK_ALIGN_END);
        gtk_widget_set_margin_end(tempsLabel, 8);
        GtkWidget* blockLabel = gtk_label_new("Stop at from_block");
        gtk_widget_set_halign(blockLabel, GTK_ALIGN_END);
        gtk_widget_set_margin_end(blockLabel, 8);
        GtkWidget* iterLabel = gtk_label_new("Stop at router iteration");
        gtk_widget_set_halign(iterLabel, GTK_ALIGN_END);
        gtk_widget_set_margin_end(iterLabel, 8);
        GtkWidget* netLabel = gtk_label_new("Stop at route_net_id");
        gtk_widget_set_halign(netLabel, GTK_ALIGN_END);
        gtk_widget_set_margin_end(netLabel, 8);
        GtkWidget* star = gtk_label_new("*for handling multiple breakpoints at once using an expression can be more accurate");
        gtk_widget_set_margin_top(star, 15);

        GtkWidget* setM = gtk_button_new_with_label("Set");
        gtk_widget_set_halign(setM, GTK_ALIGN_START);
        gtk_widget_set_margin_bottom(setM, 10);
        gtk_widget_set_margin_start(setM, 10);
        GtkWidget* setT = gtk_button_new_with_label("Set");
        gtk_widget_set_halign(setT, GTK_ALIGN_START);
        gtk_widget_set_margin_bottom(setT, 10);
        gtk_widget_set_margin_start(setT, 10);
        GtkWidget* setB = gtk_button_new_with_label("Set");
        gtk_widget_set_halign(setB, GTK_ALIGN_START);
        gtk_widget_set_margin_start(setB, 10);
        GtkWidget* setI = gtk_button_new_with_label("Set");
        gtk_widget_set_halign(setI, GTK_ALIGN_START);
        gtk_widget_set_margin_start(setI, 10);
        GtkWidget* setN = gtk_button_new_with_label("Set");
        gtk_widget_set_halign(setN, GTK_ALIGN_START);
        gtk_widget_set_margin_start(setN, 10);
        GtkWidget* advanced = gtk_button_new_with_label("Advanced");
        gtk_widget_set_margin_start(advanced, 60);
        gtk_widget_set_margin_end(advanced, 10);
        gtk_widget_set_margin_top(advanced, 20);

        GtkWidget* movesEntry = gtk_entry_new();
        gtk_entry_set_text((GtkEntry*)movesEntry, "ex. 100");
        gtk_widget_set_margin_bottom(movesEntry, 10);
        GtkWidget* tempsEntry = gtk_entry_new();
        gtk_entry_set_text((GtkEntry*)tempsEntry, "ex. 5");
        gtk_widget_set_margin_bottom(tempsEntry, 10);
        GtkWidget* blockEntry = gtk_entry_new();
        gtk_entry_set_text((GtkEntry*)blockEntry, "ex. 83");
        GtkWidget* iterEntry = gtk_entry_new();
        gtk_entry_set_text((GtkEntry*)iterEntry, "ex. 3");
        GtkWidget* netEntry = gtk_entry_new();
        gtk_entry_set_text((GtkEntry*)netEntry, "ex. 12");

        draw_debug_glob_vars.bpGrid = gtk_grid_new();
        gtk_widget_set_margin_bottom(draw_debug_glob_vars.bpGrid, 20);
        refresh_bpList();

        auto* scrollArea = new QScrollArea();
        scrollArea->setWidgetResizable(true);
        scrollArea->setWidget(draw_debug_glob_vars.bpGrid);
        scrollArea->setMinimumHeight(100);

        gtk_grid_attach((GtkGrid*)mainGrid, placerOpts,  0, 0, 3, 1);
        gtk_grid_attach((GtkGrid*)mainGrid, movesLabel,  0, 1, 1, 1);
        gtk_grid_attach((GtkGrid*)mainGrid, movesEntry,  1, 1, 1, 1);
        gtk_grid_attach((GtkGrid*)mainGrid, setM,        2, 1, 1, 1);
        gtk_grid_attach((GtkGrid*)mainGrid, tempsLabel,  0, 2, 1, 1);
        gtk_grid_attach((GtkGrid*)mainGrid, tempsEntry,  1, 2, 1, 1);
        gtk_grid_attach((GtkGrid*)mainGrid, setT,        2, 2, 1, 1);
        gtk_grid_attach((GtkGrid*)mainGrid, blockLabel,  0, 3, 1, 1);
        gtk_grid_attach((GtkGrid*)mainGrid, blockEntry,  1, 3, 1, 1);
        gtk_grid_attach((GtkGrid*)mainGrid, setB,        2, 3, 1, 1);
        gtk_grid_attach((GtkGrid*)mainGrid, routerOpts,  0, 4, 3, 1);
        gtk_grid_attach((GtkGrid*)mainGrid, iterLabel,   0, 5, 1, 1);
        gtk_grid_attach((GtkGrid*)mainGrid, iterEntry,   1, 5, 1, 1);
        gtk_grid_attach((GtkGrid*)mainGrid, setI,        2, 5, 1, 1);
        gtk_grid_attach((GtkGrid*)mainGrid, netLabel,    0, 6, 1, 1);
        gtk_grid_attach((GtkGrid*)mainGrid, netEntry,    1, 6, 1, 1);
        gtk_grid_attach((GtkGrid*)mainGrid, setN,        2, 6, 1, 1);
        gtk_grid_attach((GtkGrid*)mainGrid, bplist,      0, 7, 3, 1);
        gtk_grid_attach((GtkGrid*)mainGrid, scrollArea,  0, 8, 3, 1);
        gtk_grid_attach((GtkGrid*)mainGrid, advanced,    2, 9, 1, 1);
        gtk_grid_attach((GtkGrid*)mainGrid, star,        0, 10, 3, 1);

        QObject::connect(Q_BUTTON(setM), &QAbstractButton::clicked,
                         [mainGrid]() { set_moves_button_callback(nullptr, mainGrid); });
        QObject::connect(Q_BUTTON(setT), &QAbstractButton::clicked,
                         [mainGrid]() { set_temp_button_callback(nullptr, mainGrid); });
        QObject::connect(Q_BUTTON(setB), &QAbstractButton::clicked,
                         [mainGrid]() { set_block_button_callback(nullptr, mainGrid); });
        QObject::connect(Q_BUTTON(setI), &QAbstractButton::clicked,
                         [mainGrid]() { set_router_iter_button_callback(nullptr, mainGrid); });
        QObject::connect(Q_BUTTON(setN), &QAbstractButton::clicked,
                         [mainGrid]() { set_net_id_button_callback(nullptr, mainGrid); });
        QObject::connect(Q_BUTTON(advanced), &QAbstractButton::clicked,
                         []() { advanced_button_callback(); });
        QObject::connect(window, &QObject::destroyed,
                         []() { close_debug_window(); });

        gtk_container_add(GTK_CONTAINER(window), mainGrid);
        gtk_widget_show_all(window);
    }
}

//window for setting advanced breakpoints
void advanced_button_callback() {
    if (!draw_debug_glob_vars.openWindows.advanced_window) {
        draw_debug_glob_vars.openWindows.advanced_window = true;

        GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_position(window, GTK_WIN_POS_CENTER);
        gtk_window_set_title(window, "Advanced Debugger Options");

        GtkWidget* set = gtk_button_new_with_label("set");
        GtkWidget* entry = gtk_entry_new();
        entry->setMinimumWidth(entry->fontMetrics().horizontalAdvance(QString(40, 'x')));
        GtkWidget* instructions = gtk_label_new("You can use % == > < <= >= && || operators with temp_count, move_num, and from_block to set your desired breakpoint. To see the full list of variables refer to the variables tab on the left\nex. move_num == 4 || from_block == 83");
        auto* instrLabel = qobject_cast<QLabel*>(instructions);
        instrLabel->setAlignment(Qt::AlignCenter);
        instrLabel->setWordWrap(true);
        instrLabel->setMaximumWidth(instrLabel->fontMetrics().horizontalAdvance(QString(40, 'x')));
        GtkWidget* expression_here = gtk_label_new("Write expression below:");

        // GtkExpander equivalent: QGroupBox
        auto* expander = new QGroupBox("Variables");
        GtkWidget* varGrid = gtk_grid_new();
        GtkWidget* pLabel  = gtk_label_new(nullptr);
        gtk_label_set_markup((GtkLabel*)pLabel, "<b>Placer Variables:</b>");
        GtkWidget* mLabel  = gtk_label_new("move_num");
        GtkWidget* tLabel  = gtk_label_new("temp_count");
        GtkWidget* bLabel  = gtk_label_new("from_block");
        GtkWidget* iLabel  = gtk_label_new("in_blocks_affected");
        GtkWidget* roLabel = gtk_label_new(nullptr);
        gtk_label_set_markup((GtkLabel*)roLabel, "<b>Router Variables:</b>");
        GtkWidget* rLabel  = gtk_label_new("router_iter");
        GtkWidget* nLabel  = gtk_label_new("route_net_id");
        gtk_widget_set_halign(mLabel,  GTK_ALIGN_START);
        gtk_widget_set_halign(tLabel,  GTK_ALIGN_START);
        gtk_widget_set_halign(bLabel,  GTK_ALIGN_START);
        gtk_widget_set_halign(iLabel,  GTK_ALIGN_START);
        gtk_widget_set_halign(rLabel,  GTK_ALIGN_START);
        gtk_widget_set_halign(nLabel,  GTK_ALIGN_START);
        gtk_grid_attach((GtkGrid*)varGrid, pLabel,  0, 0, 1, 1);
        gtk_grid_attach((GtkGrid*)varGrid, mLabel,  0, 1, 1, 1);
        gtk_grid_attach((GtkGrid*)varGrid, tLabel,  0, 2, 1, 1);
        gtk_grid_attach((GtkGrid*)varGrid, bLabel,  0, 3, 1, 1);
        gtk_grid_attach((GtkGrid*)varGrid, iLabel,  0, 4, 1, 1);
        gtk_grid_attach((GtkGrid*)varGrid, roLabel, 0, 5, 1, 1);
        gtk_grid_attach((GtkGrid*)varGrid, rLabel,  0, 6, 1, 1);
        gtk_grid_attach((GtkGrid*)varGrid, nLabel,  0, 7, 1, 1);
        gtk_container_add(expander, varGrid);
        gtk_widget_set_halign(expander, GTK_ALIGN_START);

        gtk_widget_set_margin_start(instructions, 30);
        gtk_widget_set_margin_end(instructions, 30);
        gtk_widget_set_margin_top(instructions, 30);
        gtk_widget_set_margin_bottom(instructions, 30);
        gtk_widget_set_margin_bottom(expression_here, 5);
        gtk_widget_set_margin_start(entry, 30);
        gtk_widget_set_margin_end(set, 30);
        gtk_widget_set_margin_start(set, 40);
        gtk_widget_set_margin_start(expander, 10);
        gtk_widget_set_margin_top(expander, 20);

        GtkWidget* advancedGrid = gtk_grid_new();
        gtk_grid_attach((GtkGrid*)advancedGrid, instructions,    1, 0, 2, 1);
        gtk_grid_attach((GtkGrid*)advancedGrid, expression_here, 1, 1, 1, 1);
        gtk_grid_attach((GtkGrid*)advancedGrid, entry,           1, 2, 1, 1);
        gtk_grid_attach((GtkGrid*)advancedGrid, set,             2, 2, 1, 1);
        gtk_grid_attach((GtkGrid*)advancedGrid, expander,        0, 0, 1, 1);

        QObject::connect(Q_BUTTON(set), &QAbstractButton::clicked,
                         [advancedGrid]() { set_expression_button_callback(nullptr, advancedGrid); });
        QObject::connect(window, &QObject::destroyed,
                         []() { close_advanced_window(); });

        gtk_container_add(GTK_CONTAINER(window), advancedGrid);
        gtk_widget_show_all(window);
    }
}

//refreshes breakpoint list for when a breakpoint is deleted
void refresh_bpList() {
    for (auto* child : gtk_container_get_children(draw_debug_glob_vars.bpGrid))
        gtk_widget_destroy(child);

    t_draw_state* draw_state = get_draw_state_vars();
    for (size_t i = 0; i < draw_debug_glob_vars.bp_labels.size(); i++) {
        GtkWidget* label = gtk_label_new(draw_debug_glob_vars.bp_labels[i].c_str());
        gtk_grid_attach((GtkGrid*)draw_debug_glob_vars.bpGrid, label, 0, i, 1, 1);
        gtk_widget_set_halign(label, GTK_ALIGN_START);

        auto* checkbox = new QCheckBox();
        std::string c = "c" + std::to_string(i);
        gtk_widget_set_name(checkbox, c.c_str());
        if (draw_state->list_of_breakpoints[i].active)
            checkbox->setChecked(true);
        QObject::connect(checkbox, &QCheckBox::toggled,
                         [checkbox]() { checkbox_callback(checkbox); });
        gtk_grid_attach((GtkGrid*)draw_debug_glob_vars.bpGrid, checkbox, 1, i, 1, 1);
        gtk_widget_set_margin_start(checkbox, 290 - draw_debug_glob_vars.bp_labels[i].size());

        auto* deleteButton = new QPushButton(QIcon("src/draw/trash.png"), "");
        std::string d = "d" + std::to_string(i);
        gtk_widget_set_name(deleteButton, d.c_str());
        QObject::connect(deleteButton, &QPushButton::clicked,
                         [deleteButton]() { delete_bp_callback(deleteButton); });
        gtk_grid_attach((GtkGrid*)draw_debug_glob_vars.bpGrid, deleteButton, 2, i, 1, 1);
        gtk_widget_set_margin_start(deleteButton, 10);

        gtk_widget_show_all(draw_debug_glob_vars.bpGrid);
    }
}

//adds new breakpoint to the breakpoint list in the ui
void add_to_bpList(std::string bpDescription) {
    draw_debug_glob_vars.bp_labels.push_back(bpDescription);
    int row = ++draw_debug_glob_vars.bpList_row;

    GtkWidget* label = gtk_label_new(bpDescription.c_str());
    gtk_grid_attach((GtkGrid*)draw_debug_glob_vars.bpGrid, label, 0, row, 1, 1);
    gtk_widget_set_halign(label, GTK_ALIGN_START);

    auto* checkbox = new QCheckBox();
    std::string c = "c" + std::to_string(row);
    gtk_widget_set_name(checkbox, c.c_str());
    checkbox->setChecked(true);
    QObject::connect(checkbox, &QCheckBox::toggled,
                     [checkbox]() { checkbox_callback(checkbox); });
    gtk_grid_attach((GtkGrid*)draw_debug_glob_vars.bpGrid, checkbox, 1, row, 1, 1);
    gtk_widget_set_margin_start(checkbox, 290 - bpDescription.size());

    auto* deleteButton = new QPushButton(QIcon("src/draw/trash.png"), "");
    std::string d = "d" + std::to_string(row);
    gtk_widget_set_name(deleteButton, d.c_str());
    QObject::connect(deleteButton, &QPushButton::clicked,
                     [deleteButton]() { delete_bp_callback(deleteButton); });
    gtk_grid_attach((GtkGrid*)draw_debug_glob_vars.bpGrid, deleteButton, 2, row, 1, 1);
    gtk_widget_set_margin_start(deleteButton, 10);

    gtk_widget_show_all(draw_debug_glob_vars.bpGrid);
}

//enables and disables a breakpoint when the checkbox is activated
void checkbox_callback(GtkWidget* widget) {
    std::string name = gtk_widget_get_name(widget);
    name.erase(name.begin());
    int location = stoi(name);
    activate_breakpoint_by_index(location, GTK_TOGGLE_BUTTON(widget)->isChecked());
}

//deletes breakpoint when indicated by the user using the delete button in the ui
void delete_bp_callback(GtkWidget* widget) {
    draw_debug_glob_vars.bpList_row--;
    std::string name = gtk_widget_get_name(widget);
    name.erase(name.begin());
    int location = stoi(name);
    draw_debug_glob_vars.bp_labels.erase(draw_debug_glob_vars.bp_labels.begin() + location);
    delete_breakpoint_by_index(location);
    refresh_bpList();
}

//sets a new move type breakpoint
void set_moves_button_callback(GtkWidget* /*widget*/, GtkWidget* grid) {
    t_draw_state* draw_state = get_draw_state_vars();
    GtkWidget* entry = gtk_grid_get_child_at(GTK_GRID(grid), 1, 1);

    //check for input validity
    int moves = atoi(gtk_entry_get_text((GtkEntry*)entry));
    if (moves >= 1 && strchr(gtk_entry_get_text((GtkEntry*)entry), '.') == NULL) {
        draw_state->list_of_breakpoints.push_back(Breakpoint(BT_MOVE_NUM, moves));
        std::string bpDescription = "Breakpoint at move_num += " + std::to_string(moves);
        add_to_bpList(bpDescription);
    } else
        invalid_breakpoint_entry_window("Invalid Move Number");
}

//sets a new temperature type breakpoint
void set_temp_button_callback(GtkWidget* /*widget*/, GtkWidget* grid) {
    t_draw_state* draw_state = get_draw_state_vars();
    GtkWidget* entry = gtk_grid_get_child_at((GtkGrid*)grid, 1, 2);

    //input validity
    int temps = atoi(gtk_entry_get_text((GtkEntry*)entry));
    if (temps >= 1 && strchr(gtk_entry_get_text((GtkEntry*)entry), '.') == NULL) {
        draw_state->list_of_breakpoints.push_back(Breakpoint(BT_TEMP_NUM, temps));
        std::string bpDescription = "Breakpoint at temp_count += " + std::to_string(temps);
        add_to_bpList(bpDescription);
    } else
        invalid_breakpoint_entry_window("Invalid temperature");
}

//sets a new block type breakpoint
void set_block_button_callback(GtkWidget* /*widget*/, GtkWidget* grid) {
    t_draw_state* draw_state = get_draw_state_vars();
    GtkWidget* entry = gtk_grid_get_child_at((GtkGrid*)grid, 1, 3);

    draw_state->list_of_breakpoints.push_back(Breakpoint(BT_FROM_BLOCK, atoi(gtk_entry_get_text((GtkEntry*)entry))));
    std::string s(gtk_entry_get_text((GtkEntry*)entry));
    std::string bpDescription = "Breakpoint from_block == " + s;
    add_to_bpList(bpDescription);
}

//sets a new router_iter type breakpoint
void set_router_iter_button_callback(GtkWidget* /*widget*/, GtkWidget* grid) {
    t_draw_state* draw_state = get_draw_state_vars();
    GtkWidget* entry = gtk_grid_get_child_at(GTK_GRID(grid), 1, 5);

    //check for input validity
    int iters = atoi(gtk_entry_get_text((GtkEntry*)entry));
    if (iters >= 1 && strchr(gtk_entry_get_text((GtkEntry*)entry), '.') == NULL) {
        draw_state->list_of_breakpoints.push_back(Breakpoint(BT_ROUTER_ITER, iters));
        std::string bpDescription = "Breakpoint at router_iter == " + std::to_string(iters);
        add_to_bpList(bpDescription);
    } else
        invalid_breakpoint_entry_window("Invalid Router Iteration");
}

//sets a new net_id type breakpoint
void set_net_id_button_callback(GtkWidget* /*widget*/, GtkWidget* grid) {
    t_draw_state* draw_state = get_draw_state_vars();
    GtkWidget* entry = gtk_grid_get_child_at((GtkGrid*)grid, 1, 6);

    draw_state->list_of_breakpoints.push_back(Breakpoint(BT_ROUTE_NET_ID, atoi(gtk_entry_get_text((GtkEntry*)entry))));
    std::string s(gtk_entry_get_text((GtkEntry*)entry));
    std::string bpDescription = "Breakpoint route_net_id == " + s;
    add_to_bpList(bpDescription);
}

//sets a new expression type breakpoint
void set_expression_button_callback(GtkWidget* /*widget*/, GtkWidget* grid) {
    t_draw_state* draw_state = get_draw_state_vars();
    GtkWidget* entry = gtk_grid_get_child_at((GtkGrid*)grid, 1, 2);

    //check input validity
    std::string expr = gtk_entry_get_text((GtkEntry*)entry);
    if (valid_expression(expr)) {
        draw_state->list_of_breakpoints.push_back(Breakpoint(BT_EXPRESSION, expr));
        std::string bpDescription = "Breakpoint at " + expr;
        add_to_bpList(bpDescription);
    } else
        invalid_breakpoint_entry_window("Invalid expression");
}

//window that pops up when an entry is not valid
void invalid_breakpoint_entry_window(std::string error) {
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(window, GTK_WIN_POS_CENTER);
    gtk_window_set_title(window, "ERROR");
    window->setWindowModality(Qt::ApplicationModal);

    GtkWidget* grid = gtk_grid_new();

    GtkWidget* label = gtk_label_new(error.c_str());
    gtk_widget_set_margin_start(label, 30);
    gtk_widget_set_margin_end(label, 30);
    gtk_widget_set_margin_top(label, 30);
    gtk_widget_set_margin_bottom(label, 30);
    gtk_grid_attach((GtkGrid*)grid, label, 0, 0, 1, 1);

    GtkWidget* button = gtk_button_new_with_label("OK");
    gtk_widget_set_margin_bottom(button, 30);
    gtk_widget_set_margin_end(button, 30);
    gtk_widget_set_margin_start(button, 30);
    gtk_grid_attach((GtkGrid*)grid, button, 0, 1, 1, 1);
    QObject::connect(Q_BUTTON(button), &QAbstractButton::clicked,
                     [window]() { ok_close_window(nullptr, window); });

    gtk_container_add(GTK_CONTAINER(window), grid);
    gtk_widget_show_all(window);
}

//window that pops up when a breakpoint is reached
//shows which breakpoint the program has stopped at and gives an info summary
void breakpoint_info_window(std::string bpDescription, BreakpointState draw_breakpoint_state, bool in_placer) {
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(window, GTK_WIN_POS_CENTER);
    gtk_window_set_title(window, "Breakpoint");

    GtkWidget* grid = gtk_grid_new();

    GtkWidget* label = gtk_label_new(bpDescription.c_str());
    gtk_widget_set_margin_start(label, 30);
    gtk_widget_set_margin_end(label, 30);
    gtk_widget_set_margin_top(label, 30);
    gtk_widget_set_margin_bottom(label, 30);
    gtk_grid_attach((GtkGrid*)grid, label, 0, 0, 1, 1);

    GtkWidget* curr_info = gtk_label_new(nullptr);
    gtk_label_set_markup((GtkLabel*)curr_info, "<b>Current Information</b>");
    gtk_widget_set_margin_start(curr_info, 30);
    gtk_widget_set_margin_end(curr_info, 30);
    gtk_widget_set_margin_bottom(curr_info, 15);
    gtk_grid_attach((GtkGrid*)grid, curr_info, 0, 1, 1, 1);

    GtkWidget* info_grid = gtk_grid_new();
    gtk_widget_set_margin_start(info_grid, 30);
    gtk_widget_set_margin_end(info_grid, 30);
    gtk_widget_set_margin_bottom(info_grid, 20);

    auto make_image = [](const char* path) -> GtkWidget* {
        auto* lbl = new QLabel();
        lbl->setPixmap(QPixmap(path));
        return lbl;
    };
    GtkWidget* m = make_image("src/draw/m.png");
    GtkWidget* t = make_image("src/draw/t.png");
    GtkWidget* r = make_image("src/draw/r.png");
    GtkWidget* n = make_image("src/draw/n.png");
    gtk_widget_set_margin_start(n, 18);
    GtkWidget* i = make_image("src/draw/i.png");
    gtk_widget_set_margin_start(i, 16);
    GtkWidget* b = make_image("src/draw/b.png");
    gtk_widget_set_margin_start(b, 18);

    std::string move_num = "move_num: " + std::to_string(draw_breakpoint_state.move_num);
    GtkWidget* move_info = gtk_label_new(move_num.c_str());
    gtk_widget_set_margin_start(move_info, 5);
    gtk_widget_set_halign(move_info, GTK_ALIGN_START);
    std::string temp_count = "temp_count: " + std::to_string(draw_breakpoint_state.temp_count);
    GtkWidget* temp_info = gtk_label_new(temp_count.c_str());
    gtk_widget_set_margin_start(temp_info, 5);
    gtk_widget_set_halign(temp_info, GTK_ALIGN_START);
    std::string in_blocks_affected = "in_blocks_affected: " + std::to_string(get_bp_state_globals()->get_glob_breakpoint_state()->block_affected);
    GtkWidget* ba_info = gtk_label_new(in_blocks_affected.c_str());
    gtk_widget_set_halign(ba_info, GTK_ALIGN_START);
    std::string block_id = "from_block: " + std::to_string(draw_breakpoint_state.from_block);
    GtkWidget* block_info = gtk_label_new(block_id.c_str());
    gtk_widget_set_margin_start(block_info, 5);
    gtk_widget_set_halign(block_info, GTK_ALIGN_START);
    std::string router_iter = "router_iter: " + std::to_string(draw_breakpoint_state.router_iter);
    GtkWidget* ri_info = gtk_label_new(router_iter.c_str());
    gtk_widget_set_margin_start(ri_info, 5);
    gtk_widget_set_halign(ri_info, GTK_ALIGN_START);
    std::string net_id = "route_net_id: " + std::to_string(draw_breakpoint_state.route_net_id);
    GtkWidget* net_info = gtk_label_new(net_id.c_str());
    gtk_widget_set_margin_start(net_info, 5);
    gtk_widget_set_halign(net_info, GTK_ALIGN_START);

    if (in_placer) {
        gtk_grid_attach((GtkGrid*)info_grid, m,          0, 0, 1, 1);
        gtk_grid_attach((GtkGrid*)info_grid, t,          0, 1, 1, 1);
        gtk_grid_attach((GtkGrid*)info_grid, i,          2, 0, 1, 1);
        gtk_grid_attach((GtkGrid*)info_grid, b,          2, 1, 1, 1);
        gtk_grid_attach((GtkGrid*)info_grid, move_info,  1, 0, 1, 1);
        gtk_grid_attach((GtkGrid*)info_grid, temp_info,  1, 1, 1, 1);
        gtk_grid_attach((GtkGrid*)info_grid, ba_info,    3, 0, 1, 1);
        gtk_grid_attach((GtkGrid*)info_grid, block_info, 3, 1, 1, 1);
    } else {
        gtk_grid_attach((GtkGrid*)info_grid, n,       2, 2, 1, 1);
        gtk_grid_attach((GtkGrid*)info_grid, r,       0, 2, 1, 1);
        gtk_grid_attach((GtkGrid*)info_grid, ri_info, 1, 2, 1, 1);
        gtk_grid_attach((GtkGrid*)info_grid, net_info,3, 2, 1, 1);
    }

    gtk_grid_attach((GtkGrid*)grid, info_grid, 0, 2, 1, 1);

    GtkWidget* button = gtk_button_new_with_label("OK");
    gtk_widget_set_margin_bottom(button, 30);
    gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
    gtk_grid_attach((GtkGrid*)grid, button, 0, 3, 1, 1);
    QObject::connect(Q_BUTTON(button), &QAbstractButton::clicked,
                     [window]() { ok_close_window(nullptr, window); });

    gtk_container_add(GTK_CONTAINER(window), grid);
    gtk_widget_show_all(window);
}

//closes the "invalid entry" window
void ok_close_window(GtkWidget* /*widget*/, GtkWidget* window) {
    gtk_window_close((GtkWindow*)window);
}

//checks if an expression is valid by checking the order of operators
// i.e expression can't start or end with an operator, && and || can't be the first operators used
bool valid_expression(std::string exp) {
    //create a vector that holds the type for all operators in the order they were entered
    //the comparing operators are COMP_OP and bool operators are BOOL_OP
    std::vector<op_type_in_expr> ops;
    for (size_t i = 0; i < exp.size(); i++) {
        if (exp[i + 1] != '\0' && ((exp[i] == '=' && exp[i + 1] == '=') || (exp[i] == '>' && exp[i + 1] == '=') || (exp[i] == '<' && exp[i + 1] == '=') || (exp[i] == '+' && exp[i + 1] == '='))) {
            ops.push_back(COMP_OP);
            i++;
        } else if (exp[i + 1] != '\0' && ((exp[i] == '>' && exp[i + 1] != '=') || (exp[i] == '<' && exp[i + 1] != '=')))
            ops.push_back(COMP_OP);
        else if (exp[i] == '&' || exp[i] == '|') {
            ops.push_back(BOOL_OP);
            i++;
        }
    }

    //if expression started or ended with a bool operand or vector has and even number of operators (since in a valid expression number of operators are always odd)
    //checks if ops is empty first so trying to access ops[0] doesn't produce a seg fault
    if (ops.size() == 0 || ops[0] == BOOL_OP || ops[ops.size() - 1] == BOOL_OP || ops.size() % 2 == 0)
        return false;

    //checks pattern (should be 0 1 0 1 0 ...) since in a valid expression comperator operators and  bool operators are used in that pattern (e.g move_num == 3 || from_block == 11)
    for (size_t j = 0; j < ops.size(); j++) {
        if (j % 2 == 0 && ops[j] == BOOL_OP)
            return false;
        else if (j % 2 != 0 && ops[j] == COMP_OP)
            return false;
    }

    //use the formula parser for checking the validity of the formula.
    //we ignore the actual result here, since we only care about whether parsing succeeds without a VtrError.
    vtr::FormulaParser fp;
    vtr::t_formula_data dummy;
    try {
        int result = fp.parse_formula(exp, dummy, true);
        (void)result;
    } catch (const vtr::VtrError& e) {
        return false;
    }

    return true;
}

//sets boolean in openWindows to false when window closes so user can't open the same window twice
void close_debug_window() {
    draw_debug_glob_vars.openWindows.debug_window = false;
}

//sets boolean in openWindows to false when window closes so user can't open the same window twice
void close_advanced_window() {
    draw_debug_glob_vars.openWindows.advanced_window = false;
}

#endif
