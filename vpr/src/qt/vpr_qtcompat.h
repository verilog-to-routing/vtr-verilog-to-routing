#ifndef VPR_QTCOMPAT_H
#define VPR_QTCOMPAT_H

#include <ezgl/qt/ezgl_qtcompat.hpp>

#include <QCheckBox>
#include <QBoxLayout>

void gtk_button_set_label(QAbstractButton* button, const char* text);

void gtk_combo_box_text_append(QComboBox* combo,
                               const char* id,
                               const char* text);

QCheckBox* gtk_check_button_new_with_label(const QString& label);

QString g_strdup(const char* str);

void gtk_box_pack_start(QBoxLayout* box,
                        QWidget* widget,
                        bool expand,
                        bool fill,
                        int padding);

void gtk_switch_set_active(QAbstractButton* button, bool flag);

using GtkBox = QBoxLayout;

#define GTK_SWITCH(w) qobject_cast<QAbstractButton*>(w)

#define Q_ABSTRACT_BUTTON(w) qobject_cast<QAbstractButton*>(w);

#include <QRadioButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QDialog>
#include <QDialogButtonBox>

using GtkToggleButton = QCheckBox;
using GtkSpinButton = QSpinBox;
using GtkSwitch = QAbstractButton;
using GtkWindow = QWidget;
using GtkEntry = QLineEdit;
using GtkLabel = QLabel;
using GtkGrid = QWidget; //QGridLayout is laying inside the widget

void gtk_widget_show_all(QWidget*);
bool gtk_toggle_button_get_active(GtkToggleButton*);
void gtk_toggle_button_set_active(GtkToggleButton*, bool flag);

#ifndef GTK_CHECK_VERSION
#define GTK_CHECK_VERSION(x,y,z) 1
#endif

void gtk_widget_set_margin_start(QWidget*, int);
void gtk_widget_set_margin_end(QWidget*, int);
void gtk_widget_set_margin_top(QWidget*, int);
void gtk_widget_set_margin_bottom(QWidget*, int);

void gtk_window_close(QWidget*);
int gtk_spin_button_get_value(QSpinBox* spinBox);
void gtk_combo_box_text_append_text(QComboBox* combo, const QString& item);
void gtk_combo_box_text_remove(QComboBox* combo, int index);
QComboBox* gtk_combo_box_text_new();
QLabel* gtk_label_new(const QString& text);
void gtk_label_set_markup(QLabel* label, const QString& text);
void gtk_entry_set_text(QLineEdit* lineEdit, const QString& text);
QLineEdit* gtk_entry_new();
const gchar* gtk_entry_get_text(QLineEdit* lineEdit);
void gtk_widget_set_name(QWidget* w, const QString& name);
void gtk_widget_set_sensitive(QWidget* w, bool flag);
const gchar* gtk_button_get_label(QPushButton* button);
QPushButton* gtk_button_new_with_label(const char* text);
const gchar* gtk_widget_get_name(QWidget* w);
void gtk_window_set_title(QWidget* w, const char* title);

#define GTK_WINDOW_TOPLEVEL 0
QWidget* gtk_window_new(int);

#define GTK_SPIN_BUTTON(w) qobject_cast<QSpinBox*>(w)
#define GTK_BUTTON(w) qobject_cast<QAbstractButton*>(w)
#define Q_BUTTON(w) qobject_cast<QAbstractButton*>(w)
#define GTK_TOGGLE_BUTTON(w) qobject_cast<QCheckBox*>(w)
#define GTK_COMBO_BOX_TEXT(w) qobject_cast<QComboBox*>(reinterpret_cast<QObject*>(w))
#define GTK_IS_CHECK_BUTTON(w) (qobject_cast<QCheckBox*>(w) != nullptr)
#define GTK_DIALOG(w) qobject_cast<QDialog*>(w)
#define GTK_ENTRY(w) qobject_cast<QLineEdit*>(w)
#define GTK_GRID(w) qobject_cast<QWidget*>(w)
#define GTK_CONTAINER(w) (w)
#define GTK_BOX(l) qobject_cast<QBoxLayout*>(l)

QGridLayout* get_grid_layout(QWidget* grid);
QWidget* gtk_grid_get_child_at(QWidget* widget, int col, int row);
void gtk_grid_attach(QWidget* widget, QWidget* child, int col, int row, int w, int h);
void gtk_container_add(QWidget* container, QWidget* w);
QWidget* gtk_grid_new();
int gtk_dialog_run(QDialog* dialog);
int gtk_spin_button_get_value_as_int(QSpinBox* spinBox);

#define GTK_DIALOG_MODAL 0x1
#define GTK_RESPONSE_ACCEPT QDialog::Accepted
#define GTK_RESPONSE_REJECT QDialog::Rejected
struct GtkDialogButton {
    const char* text;
    int response;
};
GtkWidget* gtk_dialog_new_with_buttons(
    const char* title,
    QWidget* parent,
    int flags,
    const GtkDialogButton* buttons,
    int button_count
);

QWidget* gtk_dialog_get_content_area(QWidget* dialog);

#define GTK_WIN_POS_CENTER 0
void gtk_window_set_position(QWidget* w, int key);

#define g_list_free(x) ((void)0)

QList<QWidget*> gtk_container_get_children(QWidget* container);

#define GTK_ALIGN_FILL   0
#define GTK_ALIGN_START  1
#define GTK_ALIGN_END    2
#define GTK_ALIGN_CENTER 3
void gtk_widget_set_halign(QWidget* w, int flag);
void gtk_window_set_transient_for(QWidget* dialog, QWidget* parent);
void gtk_widget_show(QWidget* widget);
void gtk_widget_hide(QWidget* widget);
QSpinBox* gtk_spin_button_new_with_range(int min, int max, int step);
QWidget* gtk_widget_get_parent_window(QWidget* w);
void gtk_spin_button_set_increments(QSpinBox* spin_box, int step, int page);
void gtk_spin_button_set_range(QSpinBox* spin_box, double min, double max);
void gtk_spin_button_set_value(QSpinBox* spin_box, double value);

#endif // VPR_QTCOMPAT_H
