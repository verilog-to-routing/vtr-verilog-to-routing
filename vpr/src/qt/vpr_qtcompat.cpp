#include <vpr_qtcompat.h>

void gtk_button_set_label(QAbstractButton* button, const char* text)
{
    if (!button) {
        return;
    }
    button->setText(text);
}

void gtk_combo_box_text_append(QComboBox* combo,
                               const char* id,
                               const char* text)
{
  if (!combo) {
    return;
  }
  combo->addItem(QString::fromUtf8(text), QString::fromUtf8(id));
}

QCheckBox* gtk_check_button_new_with_label(const QString& label)
{
    return new QCheckBox(label);
}

QString g_strdup(const char* str)
{
    QString s = str ? QString::fromUtf8(str) : QString();
    return s;
}

void gtk_box_pack_start(QBoxLayout* box,
                        QWidget* widget,
                        bool expand,
                        bool fill,
                        int padding)
{
  if (!box || !widget) {
    return;
  }

  int stretch = expand ? 1 : 0;

  Qt::Alignment align = Qt::Alignment();
  if (!fill) {
    align = Qt::AlignLeft;
  }

  box->addWidget(widget, stretch, align);

  if (padding > 0) {
    box->setSpacing(padding);
  }
}

void gtk_switch_set_active(QAbstractButton* button, bool flag)
{
  if (!button) {
    return;
  }
  button->setChecked(flag);
}

void gtk_widget_show_all(QWidget* w)
{
    w->show();
    for (auto child : w->findChildren<QWidget*>()) {
      child->show();
    }
}

bool gtk_toggle_button_get_active(GtkToggleButton* button)
{
  if (!button) {
    return false;
  }
  return button->isChecked();
}

void gtk_toggle_button_set_active(GtkToggleButton* button, bool flag)
{
  if (!button) {
    return;
  }
  button->setChecked(flag);
}

void gtk_widget_set_margin_start(QWidget* w, int m)
{
  if (!w) {
    return;
  }

  QMargins margins = w->contentsMargins();
  margins.setLeft(m);
  w->setContentsMargins(margins);
}

void gtk_widget_set_margin_end(QWidget* w, int m)
{
  if (!w) {
    return;
  }

  QMargins margins = w->contentsMargins();
  margins.setRight(m);
  w->setContentsMargins(margins);
}

void gtk_widget_set_margin_top(QWidget* w, int m)
{
  if (!w) {
    return;
  }

  QMargins margins = w->contentsMargins();
  margins.setTop(m);
  w->setContentsMargins(margins);
}

void gtk_widget_set_margin_bottom(QWidget* w, int m)
{
  if (!w) {
    return;
  }

  QMargins margins = w->contentsMargins();
  margins.setBottom(m);
  w->setContentsMargins(margins);
}

void gtk_window_close(QWidget* w)
{
  if (!w) {
    return;
  }
  w->close();
}

int gtk_spin_button_get_value(QSpinBox* spinBox)
{
  return static_cast<int>(spinBox->value());
}

void gtk_combo_box_text_append_text(QComboBox* combo, const QString& item)
{
  combo->addItem(item);
}

void gtk_combo_box_text_remove(QComboBox* combo, int index)
{
  combo->removeItem(index);
}

QComboBox* gtk_combo_box_text_new()
{
  return new QComboBox;
}

QLabel* gtk_label_new(const QString& text)
{
  return new QLabel(text);
}

void gtk_label_set_markup(QLabel* label, const QString& text)
{
  label->setTextFormat(Qt::RichText);
  label->setText(text);
}

void gtk_entry_set_text(QLineEdit* lineEdit, const QString& text)
{
  lineEdit->setText(text);
}

QLineEdit* gtk_entry_new()
{
  return new QLineEdit;
}

const gchar* gtk_entry_get_text(QLineEdit* lineEdit)
{
  return lineEdit->text().toStdString().c_str();
}

void gtk_widget_set_name(QWidget* w, const QString& name)
{
  w->setObjectName(name);
}

void gtk_widget_set_sensitive(QWidget* w, bool flag)
{
  w->setEnabled(flag);
}

const gchar* gtk_button_get_label(QPushButton* button)
{
  return button->text().toStdString().c_str();
}

QPushButton* gtk_button_new_with_label(const char* text)
{
  QPushButton* button = new QPushButton(text);
  return button;
}

const gchar* gtk_widget_get_name(QWidget* w)
{
  return w->objectName().toStdString().c_str();
}

void gtk_window_set_title(QWidget* w, const char* title)
{
  w->setWindowTitle(title);
}

QWidget* gtk_window_new(int)
{
  return new QWidget;
}

QGridLayout* get_grid_layout(QWidget* grid)
{
  if (!grid) {
    return nullptr;
  }

  QGridLayout* gridLayout = qobject_cast<QGridLayout*>(grid->layout());
  if (!gridLayout) {
    return nullptr;
  }
  return gridLayout;
}

QWidget* gtk_grid_get_child_at(QWidget* widget, int col, int row)
{
  QGridLayout* grid = get_grid_layout(widget);
  if (!grid) {
    return nullptr;
  }

  for (int i = 0; i < grid->count(); ++i) {
    int r, c, rs, cs;
    grid->getItemPosition(i, &r, &c, &rs, &cs);

    if (r == row && c == col) {
      if (auto item = grid->itemAt(i)) {
          return item->widget();
      }
    }
  }
  return nullptr;
}

void gtk_grid_attach(QWidget* widget, QWidget* child, int col, int row, int w, int h)
{
  QGridLayout* grid = get_grid_layout(widget);
  if (!grid) {
    return;
  }
  grid->addWidget(child, row, col, h, w);
}

void gtk_container_add(QWidget* container, QWidget* w)
{
  if (!container->layout()) {
    container->setLayout(new QVBoxLayout(container));
  }

  container->layout()->addWidget(w);
}

QWidget* gtk_grid_new()
{
  QWidget* w = new QWidget;
  w->setLayout(new QGridLayout);
  return w;
}

int gtk_dialog_run(QDialog* dialog)
{
  return dialog->exec();
}

int gtk_spin_button_get_value_as_int(QSpinBox* spinBox)
{
  return static_cast<int>(spinBox->value());
}

GtkWidget* gtk_dialog_new_with_buttons(
    const char* title,
    QWidget* parent,
    int flags,
    const GtkDialogButton* buttons,
    int button_count
)
{
    QDialog* dialog = new QDialog(parent);
    dialog->setWindowTitle(title);

    if (flags & GTK_DIALOG_MODAL) {
      dialog->setModal(true);
    }

    auto* layout = new QVBoxLayout(dialog);
    auto* buttonBox = new QDialogButtonBox(dialog);

    for (int i = 0; i < button_count; ++i) {
        const auto& b = buttons[i];

        QPushButton* btn =
            buttonBox->addButton(b.text, QDialogButtonBox::ActionRole);

        QObject::connect(btn, &QPushButton::clicked, dialog, [dialog, b]() {
            dialog->done(b.response);
        });
    }

    layout->addWidget(buttonBox);

    return dialog;
}

QWidget* gtk_dialog_get_content_area(QWidget* dialog)
{
  if (!dialog) return nullptr;

  if (!dialog->layout()) {
    auto* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
  }

  return dialog;
}

void gtk_window_set_position(QWidget* w, int key)
{
  if (key == GTK_WIN_POS_CENTER) {
    QRect screenGeometry = w->screen()->availableGeometry();

    int x = (screenGeometry.width() - w->width()) / 2;
    int y = (screenGeometry.height() - w->height()) / 2;

    w->move(x, y);
  } else {
    qWarning() << key << "for gtk_window_set_position is not implemented";
  }
}

QList<QWidget*> gtk_container_get_children(QWidget* container)
{
  return container->findChildren<QWidget*>(
      QString(),
      Qt::FindDirectChildrenOnly
  );
}

void gtk_widget_set_halign(QWidget* w, int flag)
{
    Qt::Alignment qtAlign;
    if (flag == GTK_ALIGN_START) qtAlign = Qt::AlignLeft;
    else if (flag == GTK_ALIGN_END) qtAlign = Qt::AlignRight;
    else qtAlign = Qt::AlignHCenter;

    if (auto* label = qobject_cast<QLabel*>(w)) {
        label->setAlignment((label->alignment() & ~Qt::AlignHorizontal_Mask) | qtAlign);
    } else if (w->parentWidget() && w->parentWidget()->layout()) {
        w->parentWidget()->layout()->setAlignment(w, qtAlign);
    }
}

void gtk_window_set_transient_for(QWidget* dialog, QWidget* parent)
{
  dialog->setParent(parent);
  dialog->setWindowFlag(Qt::Dialog);
  dialog->setWindowModality(Qt::WindowModal);
}

void gtk_widget_show(QWidget* widget)
{
  widget->show();
}

void gtk_widget_hide(QWidget* widget)
{
  widget->hide();
}

QSpinBox* gtk_spin_button_new_with_range(int min, int max, int step)
{
  QSpinBox* spin_box = new QSpinBox();
  spin_box->setRange(min, max);
  spin_box->setSingleStep(step);
  return spin_box;
}

QWidget* gtk_widget_get_parent_window(QWidget* w)
{
  return qobject_cast<QWidget*>(w->parent());
}

void gtk_spin_button_set_increments(QSpinBox* spin_box, int step, int page)
{
  if (!spin_box) {
    return;
  }

  spin_box->setSingleStep(step);
}

void gtk_spin_button_set_range(QSpinBox* spin_box, double min, double max)
{
  if (!spin_box) {
    return;
  }

  spin_box->setRange(min, max);
}

void gtk_spin_button_set_value(QSpinBox* spin_box, double value)
{
  if (!spin_box) {
    return;
  }

  spin_box->setValue(value);
}
