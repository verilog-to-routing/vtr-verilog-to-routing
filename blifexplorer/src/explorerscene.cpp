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
#include <QDebug>

#include "explorerscene.h"
#include "wire.h"


ExplorerScene::ExplorerScene(QMenu *itemMenu, QObject *parent)
    : QGraphicsScene(parent)
{
    myUnitMenu = itemMenu;
    myMode = MoveItem;
    myUnitType = LogicUnit::LogicGate;
    line = 0;
    textItem = 0;
    myUnitColor = Qt::white;
    myTextColor = Qt::black;
    myLineColor = Qt::black;
}


/*---------------------------------------------------------------------------------------------
 * (function: setLineColor)
 *-------------------------------------------------------------------------------------------*/
void ExplorerScene::setLineColor(const QColor &color)
{
    myLineColor = color;
    if (isItemChange(Wire::Type)) {
        Wire *item =
            qgraphicsitem_cast<Wire *>(selectedItems().first());
        item->setColor(myLineColor);
        update();
    }
}


/*---------------------------------------------------------------------------------------------
 * (function: setTextColor)
 *-------------------------------------------------------------------------------------------*/
void ExplorerScene::setTextColor(const QColor &color)
{
    myTextColor = color;
    if (isItemChange(DiagramTextItem::Type)) {
        DiagramTextItem *item =
            qgraphicsitem_cast<DiagramTextItem *>(selectedItems().first());
        item->setDefaultTextColor(myTextColor);
    }
}


/*---------------------------------------------------------------------------------------------
 * (function: setItemColor)
 *-------------------------------------------------------------------------------------------*/
void ExplorerScene::setItemColor(const QColor &color)
{
    myUnitColor = color;
    if (isItemChange(LogicUnit::Type)) {
        LogicUnit *item =
            qgraphicsitem_cast<LogicUnit *>(selectedItems().first());
        item->setBrush(myUnitColor);
    }
}


/*---------------------------------------------------------------------------------------------
 * (function: setFont)
 *-------------------------------------------------------------------------------------------*/
void ExplorerScene::setFont(const QFont &font)
{
    myFont = font;

    if (isItemChange(DiagramTextItem::Type)) {
        QGraphicsTextItem *item =
            qgraphicsitem_cast<DiagramTextItem *>(selectedItems().first());
        //At this point the selection can change so the first selected item might not be a DiagramTextItem
        if (item)
            item->setFont(myFont);
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: setMode)
 *-------------------------------------------------------------------------------------------*/
void ExplorerScene::setMode(Mode mode)
{
    myMode = mode;
}

/*---------------------------------------------------------------------------------------------
 * (function: setUnitType)
 *-------------------------------------------------------------------------------------------*/
void ExplorerScene::setUnitType(LogicUnit::UnitType type)
{
    myUnitType = type;
}

/*---------------------------------------------------------------------------------------------
 * (function: editorLostFocus)
 *-------------------------------------------------------------------------------------------*/
void ExplorerScene::editorLostFocus(DiagramTextItem *item)
{
    QTextCursor cursor = item->textCursor();
    cursor.clearSelection();
    item->setTextCursor(cursor);

    if (item->toPlainText().isEmpty()) {
        removeItem(item);
        item->deleteLater();
    }
}


/*---------------------------------------------------------------------------------------------
 * (function: mousePressEvent)
 *-------------------------------------------------------------------------------------------*/
void ExplorerScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if (mouseEvent->button() != Qt::LeftButton)
        return;

    LogicUnit *item;
    switch (myMode) {
        case InsertItem:
            item = new LogicUnit("MouseAdd",myUnitType, myUnitMenu);
            item->setBrush(myUnitColor);
            addItem(item);
            item->setPos(mouseEvent->scenePos());
            emit itemInserted(item);
            break;

        case InsertLine:
            line = new QGraphicsLineItem(QLineF(mouseEvent->scenePos(),
                                        mouseEvent->scenePos()));
            line->setPen(QPen(myLineColor, 2));
            addItem(line);
            break;

        case InsertText:
            textItem = new DiagramTextItem();
            textItem->setFont(myFont);
            textItem->setTextInteractionFlags(Qt::TextEditorInteraction);
            textItem->setZValue(1000.0);
            connect(textItem, SIGNAL(lostFocus(DiagramTextItem*)),
                    this, SLOT(editorLostFocus(DiagramTextItem*)));
            connect(textItem, SIGNAL(selectedChange(QGraphicsItem*)),
                    this, SIGNAL(itemSelected(QGraphicsItem*)));
            addItem(textItem);
            textItem->setDefaultTextColor(myTextColor);
            textItem->setPos(mouseEvent->scenePos());
            emit textInserted(textItem);

    default:
        ;
    }
    QGraphicsScene::mousePressEvent(mouseEvent);
}

/*---------------------------------------------------------------------------------------------
 * (function: mouseMoveEvent)
 *-------------------------------------------------------------------------------------------*/
void ExplorerScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if (myMode == InsertLine && line != 0) {
        QLineF newLine(line->line().p1(), mouseEvent->scenePos());
        line->setLine(newLine);
    } else if (myMode == MoveItem) {
        QGraphicsScene::mouseMoveEvent(mouseEvent);
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: mouseReleaseEvent)
 *-------------------------------------------------------------------------------------------*/
void ExplorerScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if (line != 0 && myMode == InsertLine) {
        QList<QGraphicsItem *> startItems = items(line->line().p1());
        if (startItems.count() && startItems.first() == line)
            startItems.removeFirst();
        QList<QGraphicsItem *> endItems = items(line->line().p2());
        if (endItems.count() && endItems.first() == line)
            endItems.removeFirst();

        removeItem(line);
        delete line;


        if (startItems.count() > 0 && endItems.count() > 0 &&
            startItems.first()->type() == LogicUnit::Type &&
            endItems.first()->type() == LogicUnit::Type &&
            startItems.first() != endItems.first()) {
            LogicUnit *startItem =
                qgraphicsitem_cast<LogicUnit *>(startItems.first());
            LogicUnit *endItem =
                qgraphicsitem_cast<LogicUnit *>(endItems.first());
            Wire *wire = new Wire(startItem, endItem);
            wire->setColor(myLineColor);
            startItem->addConnection(wire);
            endItem->addConnection(wire);
            wire->setZValue(-1000.0);
            addItem(wire);
            wire->updatePosition();
        }
    }

    line = 0;
    QGraphicsScene::mouseReleaseEvent(mouseEvent);
}

/*---------------------------------------------------------------------------------------------
 * (function: isItemChange)
 *-------------------------------------------------------------------------------------------*/
bool ExplorerScene::isItemChange(int type)
{
    foreach (QGraphicsItem *item, selectedItems()) {
        if (item->type() == type)
            return true;
    }
    return false;
}

/*---------------------------------------------------------------------------------------------
 * (function: addLogicUnit)
 *-------------------------------------------------------------------------------------------*/
LogicUnit *ExplorerScene::addLogicUnit(QString name, LogicUnit::UnitType type,QPointF position)
{

    LogicUnit *unit;
    QString nm;
    nm=name;
    unit = new LogicUnit(nm,type, myUnitMenu);
    unit->setBrush(myUnitColor);
    addItem(unit);
    unit->setPos(position);
    emit itemInserted(unit);

    return unit;
}

/*---------------------------------------------------------------------------------------------
 * (function: addConnection)
 *-------------------------------------------------------------------------------------------*/
bool ExplorerScene::addConnection(LogicUnit *startUnit, LogicUnit *endUnit)
{

    if(startUnit->getName().compare(endUnit->getName())==0)
        return false;

    Wire *wire = new Wire(startUnit, endUnit);
    wire->setColor(myLineColor);
    startUnit->addConnection(wire);
    endUnit->addConnection(wire);
    wire->setZValue(-1000.0);
    addItem(wire);
    wire->updatePosition();

    return true;
}



