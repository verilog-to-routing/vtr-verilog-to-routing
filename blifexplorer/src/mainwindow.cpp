/*

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/
#include <QtGui>
#include <QLabel>
#include <QDebug>
#include <QHash>

#include "mainwindow.h"
#include "explorerscene.h"
#include "diagramtextitem.h"
#include "wire.h"
#include "container.h"

//length of a standard text
const int InsertTextButton = 12;


MainWindow::MainWindow()
{
    createActions();
    createToolBox();
    createMenus();

    scene = new ExplorerScene(itemMenu, this);

    //set the dimensions of the whole scene.
    //will be changed as soon as a file is opened.
    //The container adjusts the dimensions according to circuit size
    scene->setSceneRect(QRectF(0, 0, 50000, 50000));
    connect(scene, SIGNAL(itemInserted(LogicUnit*)),
            this, SLOT(itemInserted(LogicUnit*)));
    connect(scene, SIGNAL(textInserted(QGraphicsTextItem*)),
        this, SLOT(textInserted(QGraphicsTextItem*)));
    connect(scene, SIGNAL(itemSelected(QGraphicsItem*)),
        this, SLOT(itemSelected(QGraphicsItem*)));
    createToolbars();

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(toolBox);
    view = new QGraphicsView(scene);
    //Refresh mode. Can be changed if refresh errors occur
    view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    layout->addWidget(view);

    QWidget *widget = new QWidget;
    widget->setLayout(layout);

    setCentralWidget(widget);
    setWindowTitle(tr("Circuit Explorer"));
    setWindowIcon(QIcon(":/images/icon.png"));
    setUnifiedTitleAndToolBarOnMac(true);

    myContainer = new Container(scene);
    actSimStep = 0;
    activityBase = 5000;
    fileopen = false;
}

/*---------------------------------------------------------------------------------------------
 * (function: simulationButtonGroupClicked)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::simulationButtonGroupClicked(QAbstractButton* button)
{
    QList<QAbstractButton *> buttons = simulationButtonGroup->buttons();
    foreach (QAbstractButton *myButton, buttons) {
    if (myButton != button)
        button->setChecked(false);
    }
    QString text = button->text();
    if(text == tr("Previous")){
        actSimStep = actSimStep-1;
        if(actSimStep<0){
            actSimStep =0;
        }
        myContainer->showSimulationStep(actSimStep);
    }else if(text == tr("Next")){
        actSimStep = (actSimStep+1);//%myContainer->getMaxSimStep();
        if(actSimStep>=myContainer->getMaxSimStep())
        {
            myContainer->simulateNextWave();
        }
        myContainer->showSimulationStep(actSimStep);
    }else if(text == tr("Start")){
        //configure clocks
        configureClocks();
        //start simulation
        myContainer->startSimulator();
    }

    QString simstat = "";
    simstat.append(QString("%1").arg(actSimStep));
    simstat.append(" / ");
    simstat.append(QString("%1").arg(myContainer->getMaxSimStep()));
    fprintf(stderr,"simstep: %d\n", actSimStep);

    simstatLabel->setText(simstat);


}


/*---------------------------------------------------------------------------------------------
 * (function: buttonGroupClicked)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::buttonGroupClicked(int id)
{
    QList<QAbstractButton *> buttons = nodeButtonGroup->buttons();
    foreach (QAbstractButton *button, buttons) {
    if (nodeButtonGroup->button(id) != button)
        button->setChecked(false);
    }
    if (id == InsertTextButton) {
        scene->setMode(ExplorerScene::InsertText);
    } else {
        scene->setUnitType(LogicUnit::UnitType(id));
        scene->setMode(ExplorerScene::InsertItem);
    }
}

void MainWindow::powerButtonGroupClicked(QAbstractButton *button)
{
    QList<QAbstractButton *> buttons = powerButtonGroup->buttons();
    foreach (QAbstractButton *myButton, buttons) {
    if (myButton != button)
        button->setChecked(false);
    }

    QString text = button->text();
    //if(text == tr("Start")){
        myContainer->getActivityInformation();
        myContainer->showActivity();
    //}
}


/*---------------------------------------------------------------------------------------------
 * (function: deleteItem)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::deleteItem()
{
    foreach (QGraphicsItem *item, scene->selectedItems()) {
        if (item->type() == Wire::Type) {
            scene->removeItem(item);
            Wire *wire = qgraphicsitem_cast<Wire *>(item);
            wire->startUnit()->removeConnection(wire);
            wire->endUnit()->removeConnection(wire);
            delete wire;
        }
    }

    foreach (QGraphicsItem *item, scene->selectedItems()) {
         if (item->type() == LogicUnit::Type) {
             qgraphicsitem_cast<LogicUnit *>(item)->removeConnections();

         }
         scene->removeItem(item);
         delete item;
     }
}


/*---------------------------------------------------------------------------------------------
 * (function: pointerGroupClicked)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::pointerGroupClicked(int)
{
    scene->setMode(ExplorerScene::Mode(pointerTypeGroup->checkedId()));
}


/*---------------------------------------------------------------------------------------------
 * (function: bringToFront)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::bringToFront()
{
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *selectedItem = scene->selectedItems().first();
    QList<QGraphicsItem *> overlapItems = selectedItem->collidingItems();

    qreal zValue = 0;
    foreach (QGraphicsItem *item, overlapItems) {
        if (item->zValue() >= zValue &&
            item->type() == LogicUnit::Type)
            zValue = item->zValue() + 0.1;
    }
    selectedItem->setZValue(zValue);
}

/*---------------------------------------------------------------------------------------------
 * (function: sendToBack)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::sendToBack()
{
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *selectedItem = scene->selectedItems().first();
    QList<QGraphicsItem *> overlapItems = selectedItem->collidingItems();

    qreal zValue = 0;
    foreach (QGraphicsItem *item, overlapItems) {
        if (item->zValue() <= zValue &&
            item->type() == LogicUnit::Type)
            zValue = item->zValue() - 0.1;
    }
    selectedItem->setZValue(zValue);
}

/*---------------------------------------------------------------------------------------------
 * (function: itemInserted)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::itemInserted(LogicUnit *item)
{
    pointerTypeGroup->button(int(ExplorerScene::MoveItem))->setChecked(true);
    scene->setMode(ExplorerScene::Mode(pointerTypeGroup->checkedId()));
    nodeButtonGroup->button(int(item->unitType()))->setChecked(false);
}

/*---------------------------------------------------------------------------------------------
 * (function: textInserted)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::textInserted(QGraphicsTextItem *)
{
    nodeButtonGroup->button(InsertTextButton)->setChecked(false);
    scene->setMode(ExplorerScene::Mode(pointerTypeGroup->checkedId()));
}

/*---------------------------------------------------------------------------------------------
 * (function: currentFontChanged)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::currentFontChanged(const QFont &)
{
    handleFontChange();
}


/*---------------------------------------------------------------------------------------------
 * (function: fontSizeChanged)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::fontSizeChanged(const QString &)
{
    handleFontChange();
}


/*---------------------------------------------------------------------------------------------
 * (function: sceneScaleChanged)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::sceneScaleChanged(int scale)
{
    double newScale = scale/100.0;
    QMatrix oldMatrix = view->matrix();
    view->resetMatrix();
    view->translate(oldMatrix.dx(), oldMatrix.dy());
    view->scale(newScale, newScale);
}


/*---------------------------------------------------------------------------------------------
 * (function: textColorChanged)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::textColorChanged()
{
    textAction = qobject_cast<QAction *>(sender());
    fontColorToolButton->setIcon(createColorToolButtonIcon(
                ":/images/textpointer.png",
                qVariantValue<QColor>(textAction->data())));
    textButtonTriggered();
}


/*---------------------------------------------------------------------------------------------
 * (function: itemColorChanged)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::itemColorChanged()
{
    fillAction = qobject_cast<QAction *>(sender());
    fillColorToolButton->setIcon(createColorToolButtonIcon(
                 ":/images/floodfill.png",
                 qVariantValue<QColor>(fillAction->data())));
    fillButtonTriggered();
}

/*---------------------------------------------------------------------------------------------
 * (function: lineColorChanged)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::lineColorChanged()
{
    lineAction = qobject_cast<QAction *>(sender());
    lineColorToolButton->setIcon(createColorToolButtonIcon(
                 ":/images/linecolor.png",
                 qVariantValue<QColor>(lineAction->data())));
    lineButtonTriggered();
}

/*---------------------------------------------------------------------------------------------
 * (function: textButtonTriggered)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::textButtonTriggered()
{
    scene->setTextColor(qVariantValue<QColor>(textAction->data()));
}


/*---------------------------------------------------------------------------------------------
 * (function: fillButtonTriggered)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::fillButtonTriggered()
{
    scene->setItemColor(qVariantValue<QColor>(fillAction->data()));
}

/*---------------------------------------------------------------------------------------------
 * (function: lineButtonTriggered)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::lineButtonTriggered()
{
    scene->setLineColor(qVariantValue<QColor>(lineAction->data()));
}


/*---------------------------------------------------------------------------------------------
 * (function: handleFontChange)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::handleFontChange()
{
    QFont font = fontCombo->currentFont();
    font.setPointSize(fontSizeCombo->currentText().toInt());
    font.setWeight(boldAction->isChecked() ? QFont::Bold : QFont::Normal);
    font.setItalic(italicAction->isChecked());
    font.setUnderline(underlineAction->isChecked());

    scene->setFont(font);
}


/*---------------------------------------------------------------------------------------------
 * (function: itemSelected)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::itemSelected(QGraphicsItem *item)
{
    DiagramTextItem *textItem =
    qgraphicsitem_cast<DiagramTextItem *>(item);

    QFont font = textItem->font();
    //QColor color = textItem->defaultTextColor();
    fontCombo->setCurrentFont(font);
    fontSizeCombo->setEditText(QString().setNum(font.pointSize()));
    boldAction->setChecked(font.weight() == QFont::Bold);
    italicAction->setChecked(font.italic());
    underlineAction->setChecked(font.underline());
}


/*---------------------------------------------------------------------------------------------
 * (function: about)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::about()
{
    QMessageBox::about(this, tr("BLIF explorer"),
                       tr("BLIF explorer \n"
                          "v 0.3a/n For more information visit: \n http://code.google.com/p/vtr-verilog-to-routing/"));
}


/*---------------------------------------------------------------------------------------------
 * (function: about)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::createToolBox()
{
    nodeButtonGroup = new QButtonGroup(this);
    nodeButtonGroup->setExclusive(false);
    connect(nodeButtonGroup, SIGNAL(buttonClicked(int)),
            this, SLOT(buttonGroupClicked(int)));
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(createCellWidget(tr("LogicUnit"),
                               LogicUnit::LogicGate), 0, 0);
    layout->addWidget(createCellWidget(tr("Input"),
                               LogicUnit::Input), 1, 0);
    layout->addWidget(createCellWidget(tr("Latch"),
                               LogicUnit::Latch), 2, 0);
    layout->addWidget(createCellWidget(tr("Clock"),
                               LogicUnit::Clock), 3, 0);
    layout->addWidget(createCellWidget(tr("AND"),
                               LogicUnit::And), 4, 0);
    layout->addWidget(createCellWidget(tr("NAND"),
                               LogicUnit::Nand), 5, 0);
    layout->addWidget(createCellWidget(tr("OR"),
                               LogicUnit::Or), 6, 0);
    layout->addWidget(createCellWidget(tr("NOR"),
                               LogicUnit::Nor), 7, 0);
    layout->addWidget(createCellWidget(tr("XOR"),
                               LogicUnit::Xor), 8, 0);
    layout->addWidget(createCellWidget(tr("XNOR"),
                               LogicUnit::Xnor), 9, 0);
    layout->addWidget(createCellWidget(tr("NOT"),
                               LogicUnit::Not), 10, 0);
    layout->addWidget(createCellWidget(tr("MUX"),
                               LogicUnit::MUX), 0, 1);
    layout->addWidget(createCellWidget(tr("ADDER"),
                               LogicUnit::ADDER_FUNC), 1, 1);
    layout->addWidget(createCellWidget(tr("CARRY"),
                               LogicUnit::CARRY_FUNC), 2, 1);
    layout->addWidget(createCellWidget(tr("Hard ADD"),
                               LogicUnit::ADD), 3, 1);
    layout->addWidget(createCellWidget(tr("Hard Minus"),
                               LogicUnit::MINUS), 4, 1);
    layout->addWidget(createCellWidget(tr("Hard Mult"),
                               LogicUnit::MULTIPLY), 5, 1);
    layout->addWidget(createCellWidget(tr("Hard Memory"),
                               LogicUnit::MEMORY), 6, 1);
    layout->addWidget(createCellWidget(tr("Module"),
                               LogicUnit::Module), 7, 1);

    QToolButton *textButton = new QToolButton;
    textButton->setCheckable(true);
    nodeButtonGroup->addButton(textButton, InsertTextButton);
    textButton->setIcon(QIcon(QPixmap(":/images/textpointer.png")
                        .scaled(30, 30)));
    textButton->setIconSize(QSize(50, 50));
    QGridLayout *textLayout = new QGridLayout;
    textLayout->addWidget(textButton, 0, 0, Qt::AlignHCenter);
    textLayout->addWidget(new QLabel(tr("Text")), 1, 0, Qt::AlignCenter);
    QWidget *textWidget = new QWidget;
    textWidget->setLayout(textLayout);
    layout->addWidget(textWidget, 15, 0);

    layout->setRowStretch(3, 10);
    layout->setColumnStretch(2, 10);

    QWidget *itemWidget = new QWidget;
    itemWidget->setLayout(layout);

    //create widget for simulation

    simulationButtonGroup = new QButtonGroup(this);
    connect(simulationButtonGroup, SIGNAL(buttonReleased(QAbstractButton*)),
            this, SLOT(simulationButtonGroupClicked(QAbstractButton*)));

    QGridLayout* simulationLayout = new QGridLayout;
    simulationLayout->addWidget(createSimulationCellWidget(tr("Start"),
                ":/images/start.png"), 0, 0);
    simulationLayout->addWidget(createSimulationCellWidget(tr("Previous"),
                ":/images/previous.png"), 1, 0);
    simulationLayout->addWidget(createSimulationCellWidget(tr("Next"),
                ":/images/next.png"), 2, 0);


    //add edge control for the simulation
    edgeRadioLayout = new QVBoxLayout;
    QGroupBox* groupBox = new QGroupBox(tr("Show Edge"));
    QRadioButton* radFall = new QRadioButton(tr("F"));
    QRadioButton* radRise = new QRadioButton(tr("R"));
    QRadioButton* radFallRise = new QRadioButton(tr("RF"));
    radFallRise->setChecked(true);
    edgeRadioLayout->addWidget(radFall);
    edgeRadioLayout->addWidget(radRise);
    edgeRadioLayout->addWidget(radFallRise);
    groupBox->setLayout(edgeRadioLayout);
    simulationLayout->addWidget(groupBox,3,0);
    connect(radFall, SIGNAL(toggled(bool)),
        this, SLOT(setEdgeFall(bool)));
    connect(radRise, SIGNAL(toggled(bool)),
        this, SLOT(setEdgeRise(bool)));
    connect(radFallRise, SIGNAL(toggled(bool)),
        this, SLOT(setEdgeFallRise(bool)));

    QLabel* legend = new QLabel();
    QSize size(100,100);
    QPixmap* pixmap = new QPixmap(size);
    pixmap->convertFromImage(QImage(":/images/legend.png"));
    legend->setPixmap(*pixmap);
    simulationLayout->addWidget(legend,4,0);

    simStatLayout = new QGridLayout;
    simstatLabel = new QLabel();
    simStatLayout->addWidget(simstatLabel, 0, 0, Qt::AlignCenter);

    QWidget *simwidget = new QWidget;
    simwidget->setLayout(simStatLayout);
    simulationLayout->addWidget(simwidget,5,0);

    simulationLayout->setRowStretch(6,10);
    simulationLayout->setColumnStretch(2,10);

    QWidget* simulationWidget = new QWidget;
    simulationWidget->setLayout(simulationLayout);

    powerButtonGroup = new QButtonGroup(this);
    connect(powerButtonGroup, SIGNAL(buttonReleased(QAbstractButton*)),
            this, SLOT(powerButtonGroupClicked(QAbstractButton*)));

    QGridLayout* powerLayout = new QGridLayout;
    powerLayout->addWidget(createPowerCellWidget(tr("Static Probability"),
                ":/images/start.png"), 0, 0);
    powerLayout->addWidget(createPowerCellWidget(tr("Switching Probability"),
                ":/images/start.png"), 1, 0);
    powerLayout->addWidget(createPowerCellWidget(tr("Switching Activity"),
                ":/images/start.png"), 2, 0);


    //create cycle input field
    QSpinBox *activitySpinBox = new QSpinBox;
    activitySpinBox->setRange(100, 1000000);
    activitySpinBox->setSingleStep(100);
    //activitySpinBox->setSuffix(" cycles");
    activitySpinBox->setValue(5000);
    connect(activitySpinBox, SIGNAL(valueChanged(int)),
            this, SLOT(activityCycleCountChangedChanged(int)));
    //add caption
    QGridLayout *activityLayout = new QGridLayout;
    activityLayout->addWidget(activitySpinBox, 0, 0, Qt::AlignHCenter);
    activityLayout->addWidget(new QLabel("Estimation Cycle Base"), 1, 0, Qt::AlignCenter);
    QWidget *activityWidget = new QWidget;
    activityWidget->setLayout(activityLayout);

    //place cycle input field in tab
    powerLayout->addWidget(activityWidget,3,0);

    powerLayout->setRowStretch(6,10);
    powerLayout->setColumnStretch(2,10);
    QWidget* powerWidget = new QWidget;
    powerWidget->setLayout(powerLayout);


    toolBox = new QToolBox;
    toolBox->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Ignored));
    toolBox->setMinimumWidth(itemWidget->sizeHint().width());
    toolBox->addItem(itemWidget, tr("Tools"));
    toolBox->addItem(simulationWidget, tr("Simulation"));
    toolBox->addItem(powerWidget, tr("Power Estimation"));
}


/*---------------------------------------------------------------------------------------------
 * (function: createActions)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::createActions()
{
    toFrontAction = new QAction(QIcon(":/images/bringtofront.png"),
                                tr("Bring to F&ront"), this);
    toFrontAction->setShortcut(tr("Ctrl+R"));
    toFrontAction->setStatusTip(tr("Bring item to front"));
    connect(toFrontAction, SIGNAL(triggered()),
            this, SLOT(bringToFront()));

    sendBackAction = new QAction(QIcon(":/images/sendtoback.png"),
                                 tr("Send to &Back"), this);
    sendBackAction->setShortcut(tr("Ctrl+B"));
    sendBackAction->setStatusTip(tr("Send item to back"));
    connect(sendBackAction, SIGNAL(triggered()),
        this, SLOT(sendToBack()));

    deleteAction = new QAction(QIcon(":/images/delete.png"),
                               tr("&Delete"), this);
    deleteAction->setShortcut(tr("Delete"));
    deleteAction->setStatusTip(tr("Delete item from circuit"));
    connect(deleteAction, SIGNAL(triggered()),
        this, SLOT(deleteItem()));

    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcuts(QKeySequence::Quit);
    exitAction->setStatusTip(tr("Quit Circuit Explorer"));
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

    boldAction = new QAction(tr("Bold"), this);
    boldAction->setCheckable(true);
    QPixmap pixmap(":/images/bold.png");
    boldAction->setIcon(QIcon(pixmap));
    boldAction->setShortcut(tr("Ctrl+B"));
    connect(boldAction, SIGNAL(triggered()),
            this, SLOT(handleFontChange()));

    italicAction = new QAction(QIcon(":/images/italic.png"),
                               tr("Italic"), this);
    italicAction->setCheckable(true);
    italicAction->setShortcut(tr("Ctrl+I"));
    connect(italicAction, SIGNAL(triggered()),
            this, SLOT(handleFontChange()));

    underlineAction = new QAction(QIcon(":/images/underline.png"),
                                  tr("Underline"), this);
    underlineAction->setCheckable(true);
    underlineAction->setShortcut(tr("Ctrl+U"));
    connect(underlineAction, SIGNAL(triggered()),
            this, SLOT(handleFontChange()));

    aboutAction = new QAction(tr("A&bout"), this);
    aboutAction->setShortcut(tr("Ctrl+I"));
    connect(aboutAction, SIGNAL(triggered()),
            this, SLOT(about()));

/*Opens the file in Odin II and creates the visualization based on the netlist*/
    openFileOdinAction = new QAction(tr("O&din Open BLIF"), this);
    openFileOdinAction->setShortcut(tr("Ctrl+O"));
    openFileOdinAction->setToolTip("Opens and visualizes a file in the BLIF format using Odin II");
    connect(openFileOdinAction, SIGNAL(triggered()),
            this, SLOT(openFileWithOdin()));

    /*Changes the name of a node*/
    setNameAction = new QAction(tr("&Change Name"), this);
    setNameAction->setShortcut(tr("Ctrl+C"));
    setNameAction->setStatusTip("Change the name of a node");
    connect(setNameAction, SIGNAL(triggered()),this,SLOT(setName()));

    selectAllAction = new QAction(tr("Select &All"), this);
    selectAllAction->setShortcut(tr("Ctrl+A"));
    selectAllAction->setStatusTip("Select all items");
    connect(selectAllAction, SIGNAL(triggered()),this,SLOT(selectAll()));

    findAction = new QAction(tr("&Find Logic Block"), this);
    findAction->setShortcut(tr("Ctrl+f"));
    findAction->setStatusTip("Searches for a specific logic block by its name");
    connect(findAction, SIGNAL(triggered()),this,SLOT(findBlock()));

    showAllConnectionsAction = new QAction(tr("Highlight Connections"), this);
    showAllConnectionsAction->setShortcut(tr("Ctrl+H"));
    showAllConnectionsAction->setStatusTip("Highlight the logic block and all connections connected to it.");
    connect(showAllConnectionsAction, SIGNAL(triggered()),this,SLOT(showAllConnection()));

    removeHighlightingAction = new QAction(tr("Reset Highlighting"), this);
    removeHighlightingAction->setStatusTip("Highlight the logic block and all connections connected to it.");
    connect(removeHighlightingAction, SIGNAL(triggered()),this,SLOT(resetHighlighting()));

    showNodeAndNeighboursOnlyAction = new QAction(tr("Show Node and Neighbours Only"), this);
    showNodeAndNeighboursOnlyAction->setStatusTip("Hides all nodes except for the selected node and its parents and children.");
    connect(showNodeAndNeighboursOnlyAction, SIGNAL(triggered()),this,SLOT(showNodeAndNeighboursOnly()));

    addParentsToHighlightingAction = new QAction(tr("Add Parents to Visibility"), this);
    addParentsToHighlightingAction->setStatusTip("Adds parents of the selected node to visualization.");
    connect(addParentsToHighlightingAction, SIGNAL(triggered()),this,SLOT(addParentsToHighlighting()));

    addChildrenToHighlightingAction = new QAction(tr("Add Children to Visibility"), this);
    addChildrenToHighlightingAction->setStatusTip("Adds child nodes of the selected node to visualization.");
    connect(addChildrenToHighlightingAction, SIGNAL(triggered()),this,SLOT(addChildrenToHighlighting()));

    showRelevantGraphAction = new QAction(tr("Show relevant Graph"), this);
    showRelevantGraphAction->setStatusTip("Shows the relevant sub graph for the current node.");
    connect(showRelevantGraphAction, SIGNAL(triggered()),this,SLOT(showRelevantGraph()));

    expandCollapseAction = new QAction(tr("Expand/Collapse Module"), this);
    connect(expandCollapseAction, SIGNAL(triggered()),this,SLOT(expandCollapse()));
}

/*---------------------------------------------------------------------------------------------
 * (function: createMenus)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openFileOdinAction);

    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    itemMenu = menuBar()->addMenu(tr("&Item"));
    itemMenu->addAction(expandCollapseAction);
    itemMenu->addSeparator();
    itemMenu->addAction(setNameAction);
    itemMenu->addAction(deleteAction);
    itemMenu->addAction(selectAllAction);
    itemMenu->addSeparator();
    itemMenu->addAction(findAction);
    itemMenu->addSeparator();
    itemMenu->addAction(toFrontAction);
    itemMenu->addAction(sendBackAction);
    itemMenu->addSeparator();
    itemMenu->addAction(showAllConnectionsAction);
    itemMenu->addAction(removeHighlightingAction);
    itemMenu->addSeparator();
    itemMenu->addAction(showNodeAndNeighboursOnlyAction);
    itemMenu->addAction(addParentsToHighlightingAction);
    itemMenu->addAction(addChildrenToHighlightingAction);
    itemMenu->addAction(showRelevantGraphAction);

    aboutMenu = menuBar()->addMenu(tr("&Help"));
    aboutMenu->addAction(aboutAction);
}


/*---------------------------------------------------------------------------------------------
 * (function: createToolbars)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::createToolbars()
{
    openToolBar = addToolBar("Open");
    openToolBar->addAction(openFileOdinAction);

    editToolBar = addToolBar(tr("Edit"));
    editToolBar->addAction(deleteAction);
    editToolBar->addAction(toFrontAction);
    editToolBar->addAction(sendBackAction);

    fontCombo = new QFontComboBox();
    connect(fontCombo, SIGNAL(currentFontChanged(QFont)),
            this, SLOT(currentFontChanged(QFont)));

    fontSizeCombo = new QComboBox;
    fontSizeCombo->setEditable(true);
    for (int i = 8; i < 30; i = i + 2)
        fontSizeCombo->addItem(QString().setNum(i));
    QIntValidator *validator = new QIntValidator(2, 64, this);
    fontSizeCombo->setValidator(validator);
    connect(fontSizeCombo, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(fontSizeChanged(QString)));


    fontColorToolButton = new QToolButton;
    fontColorToolButton->setPopupMode(QToolButton::MenuButtonPopup);
    fontColorToolButton->setMenu(createColorMenu(SLOT(textColorChanged()),
                                                 Qt::black));
    textAction = fontColorToolButton->menu()->defaultAction();
    fontColorToolButton->setIcon(createColorToolButtonIcon(
    ":/images/textpointer.png", Qt::black));
    fontColorToolButton->setAutoFillBackground(true);
    connect(fontColorToolButton, SIGNAL(clicked()),
            this, SLOT(textButtonTriggered()));


    fillColorToolButton = new QToolButton;
    fillColorToolButton->setPopupMode(QToolButton::MenuButtonPopup);
    fillColorToolButton->setMenu(createColorMenu(SLOT(itemColorChanged()),
                         Qt::white));
    fillAction = fillColorToolButton->menu()->defaultAction();
    fillColorToolButton->setIcon(createColorToolButtonIcon(
    ":/images/floodfill.png", Qt::white));
    connect(fillColorToolButton, SIGNAL(clicked()),
            this, SLOT(fillButtonTriggered()));


    lineColorToolButton = new QToolButton;
    lineColorToolButton->setPopupMode(QToolButton::MenuButtonPopup);
    lineColorToolButton->setMenu(createColorMenu(SLOT(lineColorChanged()),
                                 Qt::black));
    lineAction = lineColorToolButton->menu()->defaultAction();
    lineColorToolButton->setIcon(createColorToolButtonIcon(
        ":/images/linecolor.png", Qt::black));
    connect(lineColorToolButton, SIGNAL(clicked()),
            this, SLOT(lineButtonTriggered()));




    colorToolBar = addToolBar(tr("Color"));
    colorToolBar->addWidget(fontColorToolButton);
    colorToolBar->addWidget(fillColorToolButton);
    colorToolBar->addWidget(lineColorToolButton);

    QToolButton *pointerButton = new QToolButton;
    pointerButton->setCheckable(true);
    pointerButton->setChecked(true);
    pointerButton->setIcon(QIcon(":/images/pointer.png"));
    QToolButton *linePointerButton = new QToolButton;
    linePointerButton->setCheckable(true);
    linePointerButton->setIcon(QIcon(":/images/linepointer.png"));

    pointerTypeGroup = new QButtonGroup(this);
    pointerTypeGroup->addButton(pointerButton, int(ExplorerScene::MoveItem));
    pointerTypeGroup->addButton(linePointerButton,
                                int(ExplorerScene::InsertLine));
    connect(pointerTypeGroup, SIGNAL(buttonClicked(int)),
            this, SLOT(pointerGroupClicked(int)));


    QSpinBox *zoomSpinBox = new QSpinBox;
    zoomSpinBox->setRange(5, 150);
    zoomSpinBox->setSingleStep(5);
    zoomSpinBox->setSuffix("%");
    zoomSpinBox->setSpecialValueText(tr("Automatic"));
    zoomSpinBox->setValue(100);

    connect(zoomSpinBox, SIGNAL(valueChanged(int)),
            this, SLOT(sceneScaleChanged(int)));

    pointerToolbar = addToolBar(tr("Pointer type"));
    pointerToolbar->addWidget(zoomSpinBox);
    pointerToolbar->addWidget(pointerButton);
    pointerToolbar->addWidget(linePointerButton);

    textToolBar = addToolBar(tr("Font"));
    textToolBar->addWidget(fontCombo);
    textToolBar->addWidget(fontSizeCombo);
    textToolBar->addAction(boldAction);
    textToolBar->addAction(italicAction);
    textToolBar->addAction(underlineAction);

}


/*---------------------------------------------------------------------------------------------
 * (function: createBackgroundCellWidget)
 *-------------------------------------------------------------------------------------------*/
// not used any more, but can be if background pattern is required
QWidget *MainWindow::createBackgroundCellWidget(const QString &text,
                        const QString &image)
{
    QToolButton *button = new QToolButton;
    button->setText(text);
    button->setIcon(QIcon(image));
    button->setIconSize(QSize(50, 50));
    button->setCheckable(true);
    backgroundButtonGroup->addButton(button);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(button, 0, 0, Qt::AlignHCenter);
    layout->addWidget(new QLabel(text), 1, 0, Qt::AlignCenter);

    QWidget *widget = new QWidget;
    widget->setLayout(layout);

    return widget;
}


/*---------------------------------------------------------------------------------------------
 * (function: createCellWidget)
 *-------------------------------------------------------------------------------------------*/
QWidget *MainWindow::createCellWidget(const QString &text,
                      LogicUnit::UnitType type)
{

    LogicUnit item("Unit",type, itemMenu);
    QIcon icon(*item.image());


    QToolButton *button = new QToolButton;
    button->setIcon(icon);
    button->setIconSize(QSize(50, 50));
    button->setCheckable(true);
    nodeButtonGroup->addButton(button, int(type));

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(button, 0, 0, Qt::AlignHCenter);
    layout->addWidget(new QLabel(text), 1, 0, Qt::AlignCenter);

    QWidget *widget = new QWidget;
    widget->setLayout(layout);

    return widget;
}


/*---------------------------------------------------------------------------------------------
 * (function: createColorMenu)
 *-------------------------------------------------------------------------------------------*/
QMenu *MainWindow::createColorMenu(const char *slot, QColor defaultColor)
{
    QList<QColor> colors;
    colors << Qt::black << Qt::white << Qt::red << Qt::blue << Qt::yellow;
    QStringList names;
    names << tr("black") << tr("white") << tr("red") << tr("blue")
          << tr("yellow");

    QMenu *colorMenu = new QMenu(this);
    for (int i = 0; i < colors.count(); ++i) {
        QAction *action = new QAction(names.at(i), this);
        action->setData(colors.at(i));
        action->setIcon(createColorIcon(colors.at(i)));
        connect(action, SIGNAL(triggered()),
                this, slot);
        colorMenu->addAction(action);
        if (colors.at(i) == defaultColor) {
            colorMenu->setDefaultAction(action);
        }
    }
    return colorMenu;
}


/*---------------------------------------------------------------------------------------------
 * (function: createColorToolButtonIcon)
 *-------------------------------------------------------------------------------------------*/
QIcon MainWindow::createColorToolButtonIcon(const QString &imageFile,
                        QColor color)
{
    QPixmap pixmap(50, 80);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    QPixmap image(imageFile);
    QRect target(0, 0, 50, 60);
    QRect source(0, 0, 42, 42);
    painter.fillRect(QRect(0, 60, 50, 80), color);
    painter.drawPixmap(target, image, source);

    return QIcon(pixmap);
}


/*---------------------------------------------------------------------------------------------
 * (function: createColorIcon)
 *-------------------------------------------------------------------------------------------*/
QIcon MainWindow::createColorIcon(QColor color)
{
    QPixmap pixmap(20, 20);
    QPainter painter(&pixmap);
    painter.setPen(Qt::NoPen);
    painter.fillRect(QRect(0, 0, 20, 20), color);

    return QIcon(pixmap);
}

/*---------------------------------------------------------------------------------------------
 * (function: openFileWithOdin)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::openFileWithOdin(){

    selectAll();
    deleteItem();

    actBlifFilename = QFileDialog::getOpenFileName(this,
                                                   tr("Open BLIF"),
                                                   QDir::homePath(),
                                                   tr("BLIF files (*.blif);;All files (*.*)"),
                                                   0,
                                                   QFileDialog::DontUseNativeDialog);

    myContainer->setFilename(actBlifFilename);
    int ok = myContainer->readInFileOdinOnly();

    if(ok == -1){
        //An error occured
        QMessageBox msgBox(QMessageBox::Warning, tr("No Structure Found in File"),
                           "The file you tried to explore does not contain any structures or could not be opened. Please select another file."
                           , 0, this);
        msgBox.addButton(tr("Open &Again"), QMessageBox::AcceptRole);
        msgBox.addButton(tr("&Continue"), QMessageBox::RejectRole);
        if (msgBox.exec() == QMessageBox::AcceptRole)
            openFileWithOdin();

    }else{
    myContainer->arrangeContainer();
    QString title = "Circuit Explorer - ";
    title.append(actBlifFilename);
    setWindowTitle(title);
    view->centerOn(0,0);
    fileopen = true;
    }

}

/*---------------------------------------------------------------------------------------------
 * (function: setName)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::setName()
{
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *item = scene->selectedItems().first();

        if (item->type() == LogicUnit::Type) {
            LogicUnit *unit =
                qgraphicsitem_cast<LogicUnit *>(scene->selectedItems().first());
            QString Name = unit->getName();
            QString newName;
            bool ok;

            newName = QInputDialog::getText(this, tr("QInputDialog::getText()"),
                                                   tr("User name:"), QLineEdit::Normal,
                                                   Name, &ok);

            if (ok && !newName.isEmpty()){
                unit->setName(newName);
                unit->update();
            }
        }


}

/*---------------------------------------------------------------------------------------------
 * (function: selectAll)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::selectAll()
{
    QPainterPath path;
    path.moveTo(0,0);
    path.addRect(0,0,view->width(),view->height());
    scene->setSelectionArea(path);
}

/*---------------------------------------------------------------------------------------------
 * (function: findBlock)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::findBlock()
{
    QHash<QString,LogicUnit *> result;
    bool ok;
    QString itemName = QInputDialog::getText(this, tr("Find logic block by Name"),
                                         tr("Logic block name:"), QLineEdit::Normal,
                                         "top^b", &ok);
    if (ok && !itemName.isEmpty()){
        selectAll();
        result = myContainer->findByName(itemName);
        scene->clearSelection();
        fprintf(stderr, "Find process done\n");
        if(result.count()!=0){
            LogicUnit* match = result.constBegin().value();
            view->centerOn(match->x(),match->y());
        }

        QString message;
        message = "Search complete.\n Items found: ";
        message.append(QString("%1").arg(result.count()));
        QMessageBox::information(this, tr("Search results."), message);

    }
}

/*---------------------------------------------------------------------------------------------
 * (function: showAllConnection)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::showAllConnection()
{
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *item = scene->selectedItems().first();

        if (item->type() == LogicUnit::Type) {
            LogicUnit *unit =
                qgraphicsitem_cast<LogicUnit *>(scene->selectedItems().first());
            QList<Wire *> list = unit->getAllCons();
            foreach (Wire *wire, list) {
                wire->setColor(QColor(0,0,255,255));
                wire->update();
                wire->setZValue(905);
            }
            list = unit->getOutCons();
            foreach (Wire *wire, list) {
                wire->setColor(QColor(0,200,200,255));
                wire->update();
                wire->setZValue(905);
            }
            unit->setBrush(QColor(0,0,255,255));
            unit->setZValue(900);
        }

}

/*---------------------------------------------------------------------------------------------
 * (function: resetHighlighting)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::resetHighlighting()
{
    if (scene->selectedItems().isEmpty()){
       myContainer->resetAllHighlights();
       myContainer->setVisibilityForAll(true);
       myContainer->arrangeContainer();
       return;
    }

    QGraphicsItem *item = scene->selectedItems().first();

    if (item->type() == LogicUnit::Type) {
        LogicUnit *unit =
            qgraphicsitem_cast<LogicUnit *>(scene->selectedItems().first());
        unit->setBrush(QColor(255,255,255,255));
        unit->setZValue(0);
        QList<Wire *> list = unit->getAllCons();
        foreach (Wire *wire, list) {
            wire->setColor(QColor(0,0,0,255));
            wire->setZValue(-1000);
            wire->update();
        }
    }



}

void MainWindow::showNodeAndNeighboursOnly()
{
    //nothing to do if no node is selected
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *item = scene->selectedItems().first();

        if (item->type() == LogicUnit::Type) {
            LogicUnit *unit =
                qgraphicsitem_cast<LogicUnit *>(scene->selectedItems().first());

            //set all nodes to invisible
            myContainer->setVisibilityForAll(false);


            unit->setShown(true);

            //make children visible
            QList<LogicUnit*> kidslist = unit->getChildren();
            foreach(LogicUnit* kid, kidslist){
                kid->setShown(true);
            }

            //make parents visible
            QList<LogicUnit*> parentslist = unit->getParents();
            foreach(LogicUnit* parent, parentslist){
                parent->setShown(true);
            }

            QList<Wire *> list = unit->getAllCons();
            foreach (Wire *wire, list) {
                wire->updatePosition();
            }

            myContainer->arrangeContainer();
            view->centerOn(unit->x(),unit->y());
        }
}

void MainWindow::addParentsToHighlighting()
{
    //nothing to do if no node is selected
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *item = scene->selectedItems().first();

        if (item->type() == LogicUnit::Type) {
            LogicUnit *unit =
                qgraphicsitem_cast<LogicUnit *>(scene->selectedItems().first());

            //make parents visible
            QList<LogicUnit*> parentslist = unit->getParents();
            foreach(LogicUnit* parent, parentslist){
                parent->setShown(true);
            }

            QList<Wire *> list = unit->getAllCons();
            foreach (Wire *wire, list) {
                wire->updatePosition();
            }

            myContainer->arrangeContainer();
        }
}

void MainWindow::addChildrenToHighlighting()
{
    //nothing to do if no node is selected
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *item = scene->selectedItems().first();

        if (item->type() == LogicUnit::Type) {
            LogicUnit *unit =
                qgraphicsitem_cast<LogicUnit *>(scene->selectedItems().first());

            //make parents visible
            QList<LogicUnit*> kidslist = unit->getChildren();
            foreach(LogicUnit* kid, kidslist){
                kid->setShown(true);
            }

            QList<Wire *> list = unit->getAllCons();
            foreach (Wire *wire, list) {
                wire->updatePosition();
            }

            myContainer->arrangeContainer();
        }
}

void MainWindow::showRelevantGraph()
{
    //nothing to do if no node is selected
    if (scene->selectedItems().isEmpty())
        return;

    QQueue<LogicUnit*> queue;
    QList<LogicUnit*> donelist;
    LogicUnit *node;

    QGraphicsItem *item = scene->selectedItems().first();

    if (item->type() == LogicUnit::Type) {
        node =
            qgraphicsitem_cast<LogicUnit *>(scene->selectedItems().first());
    } else {
        return;
    }

    //set all nodes to invisible
    myContainer->setVisibilityForAll(false);


    node->setShown(true);


    //go left in the graph until inputs are reached
        queue.enqueue(node);
        while(!queue.empty()){
            LogicUnit *actNode = queue.dequeue();

            //make parents visible
            QList<LogicUnit*> parentslist = actNode->getParents();
            foreach(LogicUnit* parent, parentslist){
                if(!queue.contains(parent) && !donelist.contains(parent)){
                    queue.enqueue(parent);
                }
                parent->setShown(true);

            }

            QList<Wire *> list = actNode->getAllCons();
            foreach (Wire *wire, list) {
                wire->updatePosition();
            }

            donelist.append(actNode);
        }

    //go right in the graph until outputs are reached
    queue.enqueue(node);
    while(!queue.empty()){
        LogicUnit *actNode =
            queue.dequeue();

        //make parents visible
        QList<LogicUnit*> kidslist = actNode->getChildren();
        foreach(LogicUnit* kid, kidslist){
            if(!queue.contains(kid) && !donelist.contains(kid)){
                queue.enqueue(kid);
            }
            kid->setShown(true);
        }

        QList<Wire *> list = actNode->getAllCons();
        foreach (Wire *wire, list) {
            wire->updatePosition();
        }

        donelist.append(actNode);
    }

    myContainer->arrangeContainer();
    view->centerOn(node->x(),node->y());
    QString message;
    message = "Relevant graph node count: ";
    message.append(QString("%1").arg(donelist.count()+1));//the constant 1 re[resents the initiator node
    QMessageBox::information(this, tr("Relevant graph"), message);

}

/*---------------------------------------------------------------------------------------------
 * (function: createSimulationCellWidget)
 *-------------------------------------------------------------------------------------------*/
QWidget * MainWindow::createSimulationCellWidget(const QString &text, const QString &image)
{
    QToolButton *button = new QToolButton;
    button->setText(text);
    button->setIcon(QIcon(image));
    button->setIconSize(QSize(50, 50));
    button->setCheckable(true);
    simulationButtonGroup->addButton(button);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(button, 0, 0, Qt::AlignHCenter);
    layout->addWidget(new QLabel(text), 1, 0, Qt::AlignCenter);

    QWidget *widget = new QWidget;
    widget->setLayout(layout);

    return widget;
}

QWidget * MainWindow::createPowerCellWidget(const QString &text, const QString &image)
{
    QToolButton *button = new QToolButton;
    button->setText(text);
    button->setIcon(QIcon(image));
    button->setIconSize(QSize(50, 50));
    button->setCheckable(true);
    powerButtonGroup->addButton(button);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(button, 0, 0, Qt::AlignHCenter);
    layout->addWidget(new QLabel(text), 1, 0, Qt::AlignCenter);

    QWidget *widget = new QWidget;
    widget->setLayout(layout);

    return widget;
}

/*---------------------------------------------------------------------------------------------
 * (function: setEdgeFall)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::setEdgeFall(bool val){
    if(val){
        myContainer->setEdge(1);
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: setEdgeRise)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::setEdgeRise(bool val){
    if(val){
        myContainer->setEdge(0);
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: setEdgeFallRise)
 *-------------------------------------------------------------------------------------------*/
void MainWindow::setEdgeFallRise(bool val){
    if(val){
        myContainer->setEdge(-1);
    }
}

void MainWindow::activityCycleCountChangedChanged(int number)
{
    activityBase = number;
}

void MainWindow::configureClocks()
{
    QList<LogicUnit*> clocks = myContainer->getClocks();

    //if less than 2 clocks available, end configuration
    if(!fileopen || clocks.length() < 2)
        return;


    clkconfig.passClockList(clocks);
    clkconfig.exec();
}

void MainWindow::expandCollapse()
{
    //nothing to do if no node is selected
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *item = scene->selectedItems().first();

        if (item->type() == LogicUnit::Type) {
            LogicUnit *unit =
                qgraphicsitem_cast<LogicUnit *>(scene->selectedItems().first());

            //if module start right away, otherwise extract module name
            if(unit->unitType() == LogicUnit::Module){
                myContainer->expandCollapse(unit->getName());
            } else if(unit->getName().contains("+")){
                QString moduleName = myContainer->extractModuleFromName(
                            unit->getName());
                myContainer->expandCollapse(moduleName);
            }
        }
}


