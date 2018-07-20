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
#ifndef LOGICUNIT_H
#define LOGICUNIT_H

#include <QGraphicsPixmapItem>
#include <QList>
#include <QDebug>
#include <QtWidgets>





QT_BEGIN_NAMESPACE
class QPixmap;
class QGraphicsItem;
class QGraphicsScene;
class QTextEdit;
class QGraphicsSceneMouseEvent;
class QMenu;
class QGraphicsSceneContextMenuEvent;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;
class QPolygonF;
QT_END_NAMESPACE

class Wire;


//we need to know odin to connect to it
#include "odininterface.h"


class LogicUnit : public QGraphicsPolygonItem
{
public:
    enum { Type = UserType + 15 };
    enum UnitType { LogicGate, Input, Output, Latch,
                    Clock, And, Or, Nand, Nor, Not,
                    Xor,Xnor, MUX, ADDER_FUNC, CARRY_FUNC,MINUS, MEMORY, MULTIPLY, ADD, Module};

    LogicUnit(QString name, UnitType unitType, QMenu *contextMenu,
        QGraphicsItem *parent = 0, QGraphicsScene *scene = 0);

    void removeConnection(Wire *wire);
    void removeConnections();
    UnitType unitType() const
        { return myUnitType; }
    QPolygonF polygon() const
        { return myPolygon; }
    void addConnection(Wire *wire);
    QPixmap* image() const;
    int type() const
        { return Type;}
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    QString getName();
    QList<Wire *> getOutCons();
    QHash<QString, Wire *> getOutConsHash();
    QList<Wire *> getAllCons();
    void setLayer(int layer);
    int getLayer();
    void updateWires();
    void setName(QString name);
    int getOutValue(int pinNumber);
    int setOutValue(int pinNumber, int value, int cycle);
    void setOdinRef(nnode_t* odinNode);
    nnode_t* getOdinRef();
    bool hasOdinReference();
    QList<LogicUnit*> getParents();
    QList<LogicUnit*> getChildren();
    bool isHighlighted();
    void setHighlighted(bool value);
    void setCurrentCycle(int cycle);
    bool isShown();
    void setShown(bool value);
    void addPartner(LogicUnit* partner);
    bool hasModule;
    LogicUnit* getModule();
    int getMaxOutNumber();
    int getMaxNumber();
    void showActivity();
protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

private:
    UnitType myUnitType;
    QPolygonF myPolygon;
    QPolygonF fullPolygon;
    QMenu *myContextMenu;
    QList<Wire *> wires;
    QString myName;
    int myIncount;
    int myLayer;
    QList< QList<int> > myOutVal;
    //a nodes partner is its module
    //a modules partner are the containing nodes
    QList<LogicUnit *> moduleViewPartners;
    nnode_t *myOdinNode;
    bool hasOdinNode;
    bool highlighted;
    bool shown;
    int myCurrentCycle;

    //module specific vars
    QList<LogicUnit*> outNodes;
};

#endif
