#ifndef NO_GRAPHICS

#include "draw_debug.h"
#include "draw_global.h"
#include "vtr_expr_eval.h"

#include <ezgl/qt/qtutils.hpp>

#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QToolButton>

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
    QWidget* bpGrid;                    ///holds the grid where all the labels are
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

        QWidget* window = new QWidget;
        window->setAttribute(Qt::WA_DeleteOnClose);
        window->setWindowTitle("Debugger");
        new QVBoxLayout(window);
        ezgl::center_window(window);

        QWidget* mainGrid = ezgl::grid_new();
        ezgl::widget_set_margin_top(mainGrid, 30);
        ezgl::widget_set_margin_bottom(mainGrid, 30);
        ezgl::widget_set_margin_start(mainGrid, 30);
        ezgl::widget_set_margin_end(mainGrid, 20);

        QLabel* placerOpts = new QLabel;
        placerOpts->setTextFormat(Qt::RichText);
        placerOpts->setText("<b>Placer Options</b>");
        ezgl::widget_set_margin_bottom(placerOpts, 10);
        QLabel* routerOpts = new QLabel;
        routerOpts->setTextFormat(Qt::RichText);
        routerOpts->setText("<b>Router Options</b>");
        ezgl::widget_set_margin_bottom(routerOpts, 10);
        ezgl::widget_set_margin_top(routerOpts, 30);
        QLabel* bplist = new QLabel;
        bplist->setTextFormat(Qt::RichText);
        bplist->setText("<b>List of Breakpoints</b>");
        ezgl::widget_set_margin_bottom(bplist, 10);
        ezgl::widget_set_margin_top(bplist, 30);
        QLabel* movesLabel = new QLabel("Number of moves to proceed");
        ezgl::widget_set_halign(movesLabel, Qt::AlignRight);
        ezgl::widget_set_margin_end(movesLabel, 8);
        QLabel* tempsLabel = new QLabel("Temperatures to proceed:");
        ezgl::widget_set_halign(tempsLabel, Qt::AlignRight);
        ezgl::widget_set_margin_end(tempsLabel, 8);
        QLabel* blockLabel = new QLabel("Stop at from_block");
        ezgl::widget_set_halign(blockLabel, Qt::AlignRight);
        ezgl::widget_set_margin_end(blockLabel, 8);
        QLabel* iterLabel = new QLabel("Stop at router iteration");
        ezgl::widget_set_halign(iterLabel, Qt::AlignRight);
        ezgl::widget_set_margin_end(iterLabel, 8);
        QLabel* netLabel = new QLabel("Stop at route_net_id");
        ezgl::widget_set_halign(netLabel, Qt::AlignRight);
        ezgl::widget_set_margin_end(netLabel, 8);
        QLabel* star = new QLabel("*for handling multiple breakpoints at once using an expression can be more accurate");
        ezgl::widget_set_margin_top(star, 15);

        QPushButton* setM = new QPushButton("Set");
        ezgl::widget_set_halign(setM, Qt::AlignLeft);
        ezgl::widget_set_margin_bottom(setM, 10);
        ezgl::widget_set_margin_start(setM, 10);
        QPushButton* setT = new QPushButton("Set");
        ezgl::widget_set_halign(setT, Qt::AlignLeft);
        ezgl::widget_set_margin_bottom(setT, 10);
        ezgl::widget_set_margin_start(setT, 10);
        QPushButton* setB = new QPushButton("Set");
        ezgl::widget_set_halign(setB, Qt::AlignLeft);
        ezgl::widget_set_margin_start(setB, 10);
        QPushButton* setI = new QPushButton("Set");
        ezgl::widget_set_halign(setI, Qt::AlignLeft);
        ezgl::widget_set_margin_start(setI, 10);
        QPushButton* setN = new QPushButton("Set");
        ezgl::widget_set_halign(setN, Qt::AlignLeft);
        ezgl::widget_set_margin_start(setN, 10);
        QPushButton* advanced = new QPushButton("Advanced");
        ezgl::widget_set_margin_start(advanced, 60);
        ezgl::widget_set_margin_end(advanced, 10);
        ezgl::widget_set_margin_top(advanced, 20);

        QLineEdit* movesEntry = new QLineEdit;
        movesEntry->setText("ex. 100");
        ezgl::widget_set_margin_bottom(movesEntry, 10);
        QLineEdit* tempsEntry = new QLineEdit;
        tempsEntry->setText("ex. 5");
        ezgl::widget_set_margin_bottom(tempsEntry, 10);
        QLineEdit* blockEntry = new QLineEdit;
        blockEntry->setText("ex. 83");
        QLineEdit* iterEntry = new QLineEdit;
        iterEntry->setText("ex. 3");
        QLineEdit* netEntry = new QLineEdit;
        netEntry->setText("ex. 12");

        draw_debug_glob_vars.bpGrid = ezgl::grid_new();
        ezgl::widget_set_margin_bottom(draw_debug_glob_vars.bpGrid, 20);
        refresh_bpList();

        auto* scrollArea = new QScrollArea();
        scrollArea->setWidgetResizable(true);
        scrollArea->setWidget(draw_debug_glob_vars.bpGrid);
        scrollArea->setMinimumHeight(100);

        ezgl::grid_attach(mainGrid, placerOpts, 0, 0, 3, 1);
        ezgl::grid_attach(mainGrid, movesLabel, 0, 1, 1, 1);
        ezgl::grid_attach(mainGrid, movesEntry, 1, 1, 1, 1);
        ezgl::grid_attach(mainGrid, setM, 2, 1, 1, 1);
        ezgl::grid_attach(mainGrid, tempsLabel, 0, 2, 1, 1);
        ezgl::grid_attach(mainGrid, tempsEntry, 1, 2, 1, 1);
        ezgl::grid_attach(mainGrid, setT, 2, 2, 1, 1);
        ezgl::grid_attach(mainGrid, blockLabel, 0, 3, 1, 1);
        ezgl::grid_attach(mainGrid, blockEntry, 1, 3, 1, 1);
        ezgl::grid_attach(mainGrid, setB, 2, 3, 1, 1);
        ezgl::grid_attach(mainGrid, routerOpts, 0, 4, 3, 1);
        ezgl::grid_attach(mainGrid, iterLabel, 0, 5, 1, 1);
        ezgl::grid_attach(mainGrid, iterEntry, 1, 5, 1, 1);
        ezgl::grid_attach(mainGrid, setI, 2, 5, 1, 1);
        ezgl::grid_attach(mainGrid, netLabel, 0, 6, 1, 1);
        ezgl::grid_attach(mainGrid, netEntry, 1, 6, 1, 1);
        ezgl::grid_attach(mainGrid, setN, 2, 6, 1, 1);
        ezgl::grid_attach(mainGrid, bplist, 0, 7, 3, 1);
        ezgl::grid_attach(mainGrid, scrollArea, 0, 8, 3, 1);
        ezgl::grid_attach(mainGrid, advanced, 2, 9, 1, 1);
        ezgl::grid_attach(mainGrid, star, 0, 10, 3, 1);

        QObject::connect(setM, &QPushButton::clicked,
                         [mainGrid]() { set_moves_button_callback(nullptr, mainGrid); });
        QObject::connect(setT, &QPushButton::clicked,
                         [mainGrid]() { set_temp_button_callback(nullptr, mainGrid); });
        QObject::connect(setB, &QPushButton::clicked,
                         [mainGrid]() { set_block_button_callback(nullptr, mainGrid); });
        QObject::connect(setI, &QPushButton::clicked,
                         [mainGrid]() { set_router_iter_button_callback(nullptr, mainGrid); });
        QObject::connect(setN, &QPushButton::clicked,
                         [mainGrid]() { set_net_id_button_callback(nullptr, mainGrid); });
        QObject::connect(advanced, &QPushButton::clicked,
                         []() { advanced_button_callback(); });
        QObject::connect(window, &QObject::destroyed,
                         []() { close_debug_window(); });

        window->layout()->addWidget(mainGrid);
        window->show();
    }
}

//window for setting advanced breakpoints
void advanced_button_callback() {
    if (!draw_debug_glob_vars.openWindows.advanced_window) {
        draw_debug_glob_vars.openWindows.advanced_window = true;

        QWidget* window = new QWidget;
        window->setAttribute(Qt::WA_DeleteOnClose);
        new QVBoxLayout(window);
        ezgl::center_window(window);
        window->setWindowTitle("Advanced Debugger Options");

        QPushButton* set = new QPushButton("set");
        QLineEdit* entry = new QLineEdit;
        entry->setMinimumWidth(entry->fontMetrics().horizontalAdvance(QString(40, 'x')));
        QLabel* instructions = new QLabel("You can use % == > < <= >= && || operators with temp_count, move_num, and from_block to set your desired breakpoint. To see the full list of variables refer to the variables tab on the left\nex. move_num == 4 || from_block == 83");
        instructions->setAlignment(Qt::AlignCenter);
        instructions->setWordWrap(true);
        instructions->setMaximumWidth(instructions->fontMetrics().horizontalAdvance(QString(40, 'x')));
        QLabel* expression_here = new QLabel("Write expression below:");

        // GtkExpander equivalent: a QToolButton acting as the disclosure
        // header (arrow + label) over a content widget whose visibility is
        // toggled by the button. Collapsed by default.
        auto* expander = new QWidget;
        auto* expanderLayout = new QVBoxLayout(expander);
        expanderLayout->setContentsMargins(0, 0, 0, 0);
        expanderLayout->setSpacing(0);
        auto* expanderToggle = new QToolButton;
        expanderToggle->setText("Variables");
        expanderToggle->setCheckable(true);
        expanderToggle->setChecked(false);
        expanderToggle->setStyleSheet("QToolButton { border: none; }");
        expanderToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        expanderToggle->setArrowType(Qt::RightArrow);
        expanderLayout->addWidget(expanderToggle);
        QWidget* varGrid = ezgl::grid_new();
        varGrid->setVisible(false);
        QObject::connect(expanderToggle, &QToolButton::toggled,
                         [expanderToggle, varGrid](bool checked) {
                             expanderToggle->setArrowType(checked ? Qt::DownArrow : Qt::RightArrow);
                             varGrid->setVisible(checked);
                         });
        QLabel* pLabel = new QLabel;
        pLabel->setTextFormat(Qt::RichText);
        pLabel->setText("<b>Placer Variables:</b>");
        QLabel* mLabel = new QLabel("move_num");
        QLabel* tLabel = new QLabel("temp_count");
        QLabel* bLabel = new QLabel("from_block");
        QLabel* iLabel = new QLabel("in_blocks_affected");
        QLabel* roLabel = new QLabel;
        roLabel->setTextFormat(Qt::RichText);
        roLabel->setText("<b>Router Variables:</b>");
        QLabel* rLabel = new QLabel("router_iter");
        QLabel* nLabel = new QLabel("route_net_id");
        ezgl::widget_set_halign(mLabel, Qt::AlignLeft);
        ezgl::widget_set_halign(tLabel, Qt::AlignLeft);
        ezgl::widget_set_halign(bLabel, Qt::AlignLeft);
        ezgl::widget_set_halign(iLabel, Qt::AlignLeft);
        ezgl::widget_set_halign(rLabel, Qt::AlignLeft);
        ezgl::widget_set_halign(nLabel, Qt::AlignLeft);
        ezgl::grid_attach(varGrid, pLabel, 0, 0, 1, 1);
        ezgl::grid_attach(varGrid, mLabel, 0, 1, 1, 1);
        ezgl::grid_attach(varGrid, tLabel, 0, 2, 1, 1);
        ezgl::grid_attach(varGrid, bLabel, 0, 3, 1, 1);
        ezgl::grid_attach(varGrid, iLabel, 0, 4, 1, 1);
        ezgl::grid_attach(varGrid, roLabel, 0, 5, 1, 1);
        ezgl::grid_attach(varGrid, rLabel, 0, 6, 1, 1);
        ezgl::grid_attach(varGrid, nLabel, 0, 7, 1, 1);
        expander->layout()->addWidget(varGrid);
        ezgl::widget_set_halign(expander, Qt::AlignLeft);

        ezgl::widget_set_margin_start(instructions, 30);
        ezgl::widget_set_margin_end(instructions, 30);
        ezgl::widget_set_margin_top(instructions, 30);
        ezgl::widget_set_margin_bottom(instructions, 30);
        ezgl::widget_set_margin_bottom(expression_here, 5);
        ezgl::widget_set_margin_start(entry, 30);
        ezgl::widget_set_margin_end(set, 30);
        ezgl::widget_set_margin_start(set, 40);
        ezgl::widget_set_margin_start(expander, 10);
        ezgl::widget_set_margin_top(expander, 20);

        QWidget* advancedGrid = ezgl::grid_new();
        ezgl::grid_attach(advancedGrid, instructions, 1, 0, 2, 1);
        ezgl::grid_attach(advancedGrid, expression_here, 1, 1, 1, 1);
        ezgl::grid_attach(advancedGrid, entry, 1, 2, 1, 1);
        ezgl::grid_attach(advancedGrid, set, 2, 2, 1, 1);
        ezgl::grid_attach(advancedGrid, expander, 0, 0, 1, 1);

        QObject::connect(set, &QPushButton::clicked,
                         [advancedGrid]() { set_expression_button_callback(nullptr, advancedGrid); });
        QObject::connect(window, &QObject::destroyed,
                         []() { close_advanced_window(); });

        window->layout()->addWidget(advancedGrid);
        window->show();
    }
}

//refreshes breakpoint list for when a breakpoint is deleted
void refresh_bpList() {
    for (auto* child : ezgl::widget_get_direct_children(draw_debug_glob_vars.bpGrid))
        child->deleteLater();

    t_draw_state* draw_state = get_draw_state_vars();
    for (size_t i = 0; i < draw_debug_glob_vars.bp_labels.size(); i++) {
        QLabel* label = new QLabel(draw_debug_glob_vars.bp_labels[i].c_str());
        ezgl::grid_attach(draw_debug_glob_vars.bpGrid, label, 0, i, 1, 1);
        ezgl::widget_set_halign(label, Qt::AlignLeft);

        auto* checkbox = new QCheckBox();
        std::string c = "c" + std::to_string(i);
        checkbox->setObjectName(c.c_str());
        if (draw_state->list_of_breakpoints[i].active)
            checkbox->setChecked(true);
        QObject::connect(checkbox, &QCheckBox::toggled,
                         [checkbox]() { checkbox_callback(checkbox); });
        ezgl::grid_attach(draw_debug_glob_vars.bpGrid, checkbox, 1, i, 1, 1);
        ezgl::widget_set_margin_start(checkbox, 290 - draw_debug_glob_vars.bp_labels[i].size());

        auto* deleteButton = new QPushButton(QIcon("src/draw/trash.png"), "");
        std::string d = "d" + std::to_string(i);
        deleteButton->setObjectName(d.c_str());
        QObject::connect(deleteButton, &QPushButton::clicked,
                         [deleteButton]() { delete_bp_callback(deleteButton); });
        ezgl::grid_attach(draw_debug_glob_vars.bpGrid, deleteButton, 2, i, 1, 1);
        ezgl::widget_set_margin_start(deleteButton, 10);

        draw_debug_glob_vars.bpGrid->show();
    }
}

//adds new breakpoint to the breakpoint list in the ui
void add_to_bpList(std::string bpDescription) {
    //create description label
    draw_debug_glob_vars.bp_labels.push_back(bpDescription);
    int row = ++draw_debug_glob_vars.bpList_row;

    QLabel* label = new QLabel(bpDescription.c_str());
    ezgl::grid_attach(draw_debug_glob_vars.bpGrid, label, 0, row, 1, 1);
    ezgl::widget_set_halign(label, Qt::AlignLeft);

    auto* checkbox = new QCheckBox();
    std::string c = "c" + std::to_string(row);
    checkbox->setObjectName(c.c_str());
    checkbox->setChecked(true);
    QObject::connect(checkbox, &QCheckBox::toggled,
                     [checkbox]() { checkbox_callback(checkbox); });
    ezgl::grid_attach(draw_debug_glob_vars.bpGrid, checkbox, 1, row, 1, 1);
    ezgl::widget_set_margin_start(checkbox, 290 - bpDescription.size());

    auto* deleteButton = new QPushButton(QIcon("src/draw/trash.png"), "");
    std::string d = "d" + std::to_string(row);
    deleteButton->setObjectName(d.c_str());
    QObject::connect(deleteButton, &QPushButton::clicked,
                     [deleteButton]() { delete_bp_callback(deleteButton); });
    ezgl::grid_attach(draw_debug_glob_vars.bpGrid, deleteButton, 2, row, 1, 1);
    ezgl::widget_set_margin_start(deleteButton, 10);

    draw_debug_glob_vars.bpGrid->show();
}

//enables and disables a breakpoint when the checkbox is activated
void checkbox_callback(QCheckBox* checkbox) {
    std::string name = checkbox->objectName().toStdString();
    name.erase(name.begin());
    int location = stoi(name);
    activate_breakpoint_by_index(location, checkbox->isChecked());
}

//deletes breakpoint when indicated by the user using the delete button in the ui
void delete_bp_callback(QWidget* widget) {
    draw_debug_glob_vars.bpList_row--;
    std::string name = widget->objectName().toStdString();
    name.erase(name.begin());
    int location = stoi(name);
    draw_debug_glob_vars.bp_labels.erase(draw_debug_glob_vars.bp_labels.begin() + location);
    delete_breakpoint_by_index(location);
    refresh_bpList();
}

//sets a new move type breakpoint
void set_moves_button_callback(QWidget* /*widget*/, QWidget* grid) {
    t_draw_state* draw_state = get_draw_state_vars();
    QLineEdit* entry = ezgl::grid_get_child_at<QLineEdit>(grid, 1, 1);

    //check for input validity
    int moves = entry->text().toInt();
    if (moves >= 1 && entry->text().contains('.')) {
        draw_state->list_of_breakpoints.push_back(Breakpoint(BT_MOVE_NUM, moves));
        std::string bpDescription = "Breakpoint at move_num += " + std::to_string(moves);
        add_to_bpList(bpDescription);
    } else
        invalid_breakpoint_entry_window("Invalid Move Number");
}

//sets a new temperature type breakpoint
void set_temp_button_callback(QWidget* /*widget*/, QWidget* grid) {
    t_draw_state* draw_state = get_draw_state_vars();
    QLineEdit* entry = ezgl::grid_get_child_at<QLineEdit>(grid, 1, 2);

    //input validity
    int temps = entry->text().toInt();
    if (temps >= 1 && entry->text().contains('.')) {
        draw_state->list_of_breakpoints.push_back(Breakpoint(BT_TEMP_NUM, temps));
        std::string bpDescription = "Breakpoint at temp_count += " + std::to_string(temps);
        add_to_bpList(bpDescription);
    } else
        invalid_breakpoint_entry_window("Invalid temperature");
}

//sets a new block type breakpoint
void set_block_button_callback(QWidget* /*widget*/, QWidget* grid) {
    t_draw_state* draw_state = get_draw_state_vars();
    QLineEdit* entry = ezgl::grid_get_child_at<QLineEdit>(grid, 1, 3);

    draw_state->list_of_breakpoints.push_back(Breakpoint(BT_FROM_BLOCK, entry->text().toInt()));
    std::string s(entry->text().toStdString());
    std::string bpDescription = "Breakpoint from_block == " + s;
    add_to_bpList(bpDescription);
}

//sets a new router_iter type breakpoint
void set_router_iter_button_callback(QWidget* /*widget*/, QWidget* grid) {
    t_draw_state* draw_state = get_draw_state_vars();
    QLineEdit* entry = ezgl::grid_get_child_at<QLineEdit>(grid, 1, 5);

    //check for input validity
    int iters = entry->text().toInt();
    if (iters >= 1 && entry->text().contains('.')) {
        draw_state->list_of_breakpoints.push_back(Breakpoint(BT_ROUTER_ITER, iters));
        std::string bpDescription = "Breakpoint at router_iter == " + std::to_string(iters);
        add_to_bpList(bpDescription);
    } else
        invalid_breakpoint_entry_window("Invalid Router Iteration");
}

//sets a new net_id type breakpoint
void set_net_id_button_callback(QWidget* /*widget*/, QWidget* grid) {
    t_draw_state* draw_state = get_draw_state_vars();
    QLineEdit* entry = ezgl::grid_get_child_at<QLineEdit>(grid, 1, 6);

    draw_state->list_of_breakpoints.push_back(Breakpoint(BT_ROUTE_NET_ID, entry->text().toInt()));
    std::string s(entry->text().toStdString());
    std::string bpDescription = "Breakpoint route_net_id == " + s;
    add_to_bpList(bpDescription);
}

//sets a new expression type breakpoint
void set_expression_button_callback(QWidget* /*widget*/, QWidget* grid) {
    t_draw_state* draw_state = get_draw_state_vars();
    QLineEdit* entry = ezgl::grid_get_child_at<QLineEdit>(grid, 1, 2);

    //check input validity
    std::string expr = entry->text().toStdString();
    if (valid_expression(expr)) {
        draw_state->list_of_breakpoints.push_back(Breakpoint(BT_EXPRESSION, expr));
        std::string bpDescription = "Breakpoint at " + expr;
        add_to_bpList(bpDescription);
    } else
        invalid_breakpoint_entry_window("Invalid expression");
}

//window that pops up when an entry is not valid
void invalid_breakpoint_entry_window(std::string error) {
    QWidget* window = new QWidget;
    window->setAttribute(Qt::WA_DeleteOnClose);
    new QVBoxLayout(window);
    ezgl::center_window(window);
    window->setWindowTitle("ERROR");
    window->setWindowModality(Qt::ApplicationModal);

    QWidget* grid = ezgl::grid_new();

    QLabel* label = new QLabel(error.c_str());
    ezgl::widget_set_margin_start(label, 30);
    ezgl::widget_set_margin_end(label, 30);
    ezgl::widget_set_margin_top(label, 30);
    ezgl::widget_set_margin_bottom(label, 30);
    ezgl::grid_attach(grid, label, 0, 0, 1, 1);

    QPushButton* button = new QPushButton("OK");
    ezgl::widget_set_margin_bottom(button, 30);
    ezgl::widget_set_margin_end(button, 30);
    ezgl::widget_set_margin_start(button, 30);
    ezgl::grid_attach(grid, button, 0, 1, 1, 1);
    QObject::connect(button, &QPushButton::clicked,
                     [window]() { ok_close_window(nullptr, window); });

    window->layout()->addWidget(grid);
    window->show();
}

//window that pops up when a breakpoint is reached
//shows which breakpoint the program has stopped at and gives an info summary
void breakpoint_info_window(std::string bpDescription, BreakpointState draw_breakpoint_state, bool in_placer) {
    QWidget* window = new QWidget;
    window->setAttribute(Qt::WA_DeleteOnClose);
    new QVBoxLayout(window);
    ezgl::center_window(window);
    window->setWindowTitle("Breakpoint");

    QWidget* grid = ezgl::grid_new();

    QLabel* label = new QLabel(bpDescription.c_str());
    ezgl::widget_set_margin_start(label, 30);
    ezgl::widget_set_margin_end(label, 30);
    ezgl::widget_set_margin_top(label, 30);
    ezgl::widget_set_margin_bottom(label, 30);
    ezgl::grid_attach(grid, label, 0, 0, 1, 1);

    QLabel* curr_info = new QLabel;
    curr_info->setTextFormat(Qt::RichText);
    curr_info->setText("<b>Current Information</b>");
    ezgl::widget_set_margin_start(curr_info, 30);
    ezgl::widget_set_margin_end(curr_info, 30);
    ezgl::widget_set_margin_bottom(curr_info, 15);
    ezgl::grid_attach(grid, curr_info, 0, 1, 1, 1);

    QWidget* info_grid = ezgl::grid_new();
    ezgl::widget_set_margin_start(info_grid, 30);
    ezgl::widget_set_margin_end(info_grid, 30);
    ezgl::widget_set_margin_bottom(info_grid, 20);

    auto make_image = [](const char* path) -> QWidget* {
        auto* lbl = new QLabel();
        lbl->setPixmap(QPixmap(path));
        return lbl;
    };
    QWidget* m = make_image("src/draw/m.png");
    QWidget* t = make_image("src/draw/t.png");
    QWidget* r = make_image("src/draw/r.png");
    QWidget* n = make_image("src/draw/n.png");
    ezgl::widget_set_margin_start(n, 18);
    QWidget* i = make_image("src/draw/i.png");
    ezgl::widget_set_margin_start(i, 16);
    QWidget* b = make_image("src/draw/b.png");
    ezgl::widget_set_margin_start(b, 18);

    //info grid labels
    std::string move_num = "move_num: " + std::to_string(draw_breakpoint_state.move_num);
    QLabel* move_info = new QLabel(move_num.c_str());
    ezgl::widget_set_margin_start(move_info, 5);
    ezgl::widget_set_halign(move_info, Qt::AlignLeft);
    std::string temp_count = "temp_count: " + std::to_string(draw_breakpoint_state.temp_count);
    QLabel* temp_info = new QLabel(temp_count.c_str());
    ezgl::widget_set_margin_start(temp_info, 5);
    ezgl::widget_set_halign(temp_info, Qt::AlignLeft);
    std::string in_blocks_affected = "in_blocks_affected: " + std::to_string(get_bp_state_globals()->get_glob_breakpoint_state()->block_affected);
    QLabel* ba_info = new QLabel(in_blocks_affected.c_str());
    ezgl::widget_set_halign(ba_info, Qt::AlignLeft);
    std::string block_id = "from_block: " + std::to_string(draw_breakpoint_state.from_block);
    QLabel* block_info = new QLabel(block_id.c_str());
    ezgl::widget_set_margin_start(block_info, 5);
    ezgl::widget_set_halign(block_info, Qt::AlignLeft);
    std::string router_iter = "router_iter: " + std::to_string(draw_breakpoint_state.router_iter);
    QLabel* ri_info = new QLabel(router_iter.c_str());
    ezgl::widget_set_margin_start(ri_info, 5);
    ezgl::widget_set_halign(ri_info, Qt::AlignLeft);
    std::string net_id = "route_net_id: " + std::to_string(draw_breakpoint_state.route_net_id);
    QLabel* net_info = new QLabel(net_id.c_str());
    ezgl::widget_set_margin_start(net_info, 5);
    ezgl::widget_set_halign(net_info, Qt::AlignLeft);

    //attach to grid
    if (in_placer) {
        ezgl::grid_attach(info_grid, m, 0, 0, 1, 1);
        ezgl::grid_attach(info_grid, t, 0, 1, 1, 1);
        ezgl::grid_attach(info_grid, i, 2, 0, 1, 1);
        ezgl::grid_attach(info_grid, b, 2, 1, 1, 1);
        ezgl::grid_attach(info_grid, move_info, 1, 0, 1, 1);
        ezgl::grid_attach(info_grid, temp_info, 1, 1, 1, 1);
        ezgl::grid_attach(info_grid, ba_info, 3, 0, 1, 1);
        ezgl::grid_attach(info_grid, block_info, 3, 1, 1, 1);
    } else {
        ezgl::grid_attach(info_grid, n, 2, 2, 1, 1);
        ezgl::grid_attach(info_grid, r, 0, 2, 1, 1);
        ezgl::grid_attach(info_grid, ri_info, 1, 2, 1, 1);
        ezgl::grid_attach(info_grid, net_info, 3, 2, 1, 1);
    }

    ezgl::grid_attach(grid, info_grid, 0, 2, 1, 1);

    QPushButton* button = new QPushButton("OK");
    ezgl::widget_set_margin_bottom(button, 30);
    ezgl::widget_set_halign(button, Qt::AlignHCenter);
    ezgl::grid_attach(grid, button, 0, 3, 1, 1);
    QObject::connect(button, &QPushButton::clicked,
                     [window]() { ok_close_window(nullptr, window); });

    window->layout()->addWidget(grid);
    window->show();
}

//closes the "invalid entry" window
void ok_close_window(QWidget* /*widget*/, QWidget* window) {
    window->close();
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
