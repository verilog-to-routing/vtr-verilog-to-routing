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
#include "logicunit.h"
#include "wire.h"




LogicUnit::LogicUnit(QString name, UnitType unitType, QMenu *contextMenu,
             QGraphicsItem *parent, QGraphicsScene *scene)
    : QGraphicsPolygonItem(parent, scene)
{
    myUnitType = unitType;
    myContextMenu = contextMenu;
    myName=name;
    myIncount = 0;
    myLayer = 0;
    QList<int> list;
    myOutVal.append(list);
    myOutVal[0].append(0);
    hasOdinNode = false;
    highlighted = false;
    hasModule = false;
    myCurrentCycle = 0;

//different types and shapes are possible here.
    // if the shape includes a graphic it can be added in function image()
    switch (myUnitType) {
        case Input:
            myPolygon << QPointF(-50, 0) << QPointF(0, 50)
                      << QPointF(50, 0) << QPointF(0, -50)
                      << QPointF(-50, 0);
            break;
    case Latch:
        myPolygon << QPointF(-50, -50) << QPointF(50, -50)
              << QPointF(50, 50) << QPointF(-50, 50)
              << QPointF(-50, -50) << QPointF(-50, 24)
              << QPointF(-43, 17)<< QPointF(-50, 10);

        break;
    case Output:
        myPolygon << QPointF(-50, -50) << QPointF(50, -50)
              << QPointF(0, 10) << QPointF(50, 0)
              << QPointF(-50, -50);

        break;
    case Clock:
        myPolygon << QPointF(-50, 0) << QPointF(0, 50)
                  << QPointF(50, 0) << QPointF(0, -50)
                  << QPointF(-50, 0)<< QPointF(-50, 30) << QPointF(-30, 50)
                  << QPointF(30, 50) << QPointF(50, 30)
                  << QPointF(50, -30)<< QPointF(30, -50)
                  << QPointF(-30, -50)<< QPointF(-50, -30)
                  << QPointF(-50, 30);
        break;
    case And:
    case Or:
    case Nor:
    case Not:
    case Nand:
    case Xor:
    case Xnor:
    case ADDER_FUNC:
    case MUX:
    case CARRY_FUNC:
    case ADD:
    case MINUS:
    case MULTIPLY:
    case MEMORY:
    case Module:
        myPolygon << QPointF(-50, -50) << QPointF(50, -50)
              << QPointF(50, 50) << QPointF(-50, 50)
              << QPointF(-50, -50);
        break;
    default:
            myPolygon << QPointF(-50, 30) << QPointF(-30, 50)
                  << QPointF(30, 50) << QPointF(50, 30)
                  << QPointF(50, -30)<< QPointF(30, -50)
                  << QPointF(-30, -50)<< QPointF(-50, -30)
                  << QPointF(-50, 30);
            break;

    }
    setPolygon(myPolygon);

    //Object properties
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
}


/*---------------------------------------------------------------------------------------------
 * (function: removeConnection)
 *-------------------------------------------------------------------------------------------*/
void LogicUnit::removeConnection(Wire *wire)
{
    int index = wires.indexOf(wire);

    if (index != -1)
        wires.removeAt(index);
}

/*---------------------------------------------------------------------------------------------
 * (function: removeConnections)
 *-------------------------------------------------------------------------------------------*/
void LogicUnit::removeConnections()
{
    foreach (Wire *wire, wires) {
        wire->startUnit()->removeConnection(wire);
        wire->endUnit()->removeConnection(wire);
        scene()->removeItem(wire);
        delete wire;
    }
    myIncount=0;
}


/*---------------------------------------------------------------------------------------------
 * (function: addConnection)
 *-------------------------------------------------------------------------------------------*/
void LogicUnit::addConnection(Wire *wire)
{
    if(myUnitType != LogicUnit::Module){
        //increase the input count if this instance is the ending point
        if(wire->endUnit()->getName().compare(myName) == 0){
            myIncount++;
            foreach (Wire *oldwire, wires) {
                oldwire->setMaxNumber(myIncount);
            }
            wire->setNumber(myIncount);
            wire->setMaxNumber(myIncount);
        }
        wires.append(wire);
        itemChange(QGraphicsPolygonItem::ItemPositionChange,QVariant(0));
    }else{//I am a Module
        if(wire->startUnit()->hasModule){
            //I am the start module
            if(wire->startUnit()->getModule()->getName().compare(myName)==0){
                LogicUnit* startNode = wire->startUnit();
                if(!outNodes.contains(startNode)){
                       outNodes.append(startNode);
                }
                wires.append(wire);
                wire->setMyOutputPinNumberModule(outNodes.indexOf(startNode)+1);
            }
        }else if(wire->endUnit()->hasModule){
            //I am the end Module
            if(wire->endUnit()->getModule()->getName().compare(myName)==0){
                myIncount++;
                wires.append(wire);
                wire->setNumberModule(myIncount);
            }
        }
    }

}

/*---------------------------------------------------------------------------------------------
 * (function: image)
 *-------------------------------------------------------------------------------------------*/
QPixmap* LogicUnit::image() const
{
    QSize size(250,250);
    QPixmap* pixmap = new QPixmap(size);
    switch (myUnitType) {
    case And:
        pixmap->convertFromImage(QImage(":/images/nodeTypes/AND.png"));
        pixmap->scaled(size);
        break;
    case Nand:
        pixmap->convertFromImage(QImage(":/images/nodeTypes/NAND.png"));
        pixmap->scaled(size);
        break;
    case Or:
        pixmap->convertFromImage(QImage(":/images/nodeTypes/OR.png"));
        pixmap->scaled(size);
        break;
    case Nor:
        pixmap->convertFromImage(QImage(":/images/nodeTypes/NOR.png"));
        pixmap->scaled(size);
        break;
    case Xor:
        pixmap->convertFromImage(QImage(":/images/nodeTypes/XOR.png"));
        pixmap->scaled(size);
        break;
    case Xnor:
        pixmap->convertFromImage(QImage(":/images/nodeTypes/XNOR.png"));
        pixmap->scaled(size);
        break;
    case Not:
        pixmap->convertFromImage(QImage(":/images/nodeTypes/NOT.png"));
        pixmap->scaled(size);
        break;
    case MUX:
        pixmap->convertFromImage(QImage(":/images/nodeTypes/MUX.png"));
        pixmap->scaled(size);
        break;
    case ADDER_FUNC:
        pixmap->convertFromImage(QImage(":/images/nodeTypes/ADDER_FUNC.png"));
        pixmap->scaled(size);
        break;
    case CARRY_FUNC:
        pixmap->convertFromImage(QImage(":/images/nodeTypes/CARRY_FUNC.png"));
        pixmap->scaled(size);
        break;
    case MULTIPLY:
        pixmap->convertFromImage(QImage(":/images/nodeTypes/Hmult.png"));
        pixmap->scaled(size);
        break;
    case ADD:
        pixmap->convertFromImage(QImage(":/images/nodeTypes/Hadd.png"));
        pixmap->scaled(size);
        break;
    case MINUS:
        pixmap->convertFromImage(QImage(":/images/nodeTypes/Hminus.png"));
        pixmap->scaled(size);
        break;
    case MEMORY:
        pixmap->convertFromImage(QImage(":/images/nodeTypes/Hmemory.png"));
        pixmap->scaled(size);
        break;
    case Module:
        pixmap->convertFromImage(QImage(":/images/nodeTypes/module.png"));
        pixmap->scaled(size);
        break;
    default:
        pixmap->fill(Qt::transparent);
        QPainter painter(pixmap);
        painter.setPen(QPen(Qt::black, 8));
        painter.translate(125, 125);
        painter.drawPolyline(myPolygon);
    }



    return pixmap;
}


/*---------------------------------------------------------------------------------------------
 * (function: contextMenuEvent)
 *-------------------------------------------------------------------------------------------*/
void LogicUnit::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    scene()->clearSelection();
    setSelected(true);
    myContextMenu->exec(event->screenPos());
}

/*---------------------------------------------------------------------------------------------
 * (function: itemChange)
 *-------------------------------------------------------------------------------------------*/
QVariant LogicUnit::itemChange(GraphicsItemChange change,
                     const QVariant &value)
{
    int i = 0;
    if (change == QGraphicsItem::ItemPositionChange) {
        foreach (Wire *wire, wires) {
            wire->updatePosition();
            i++;
        }
    }

    return value;
}

/*---------------------------------------------------------------------------------------------
 * (function: paint)
 *-------------------------------------------------------------------------------------------*/
void LogicUnit::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
     QGraphicsPolygonItem::paint(painter,option,widget);
QImage image;

//If a picture defines the shape, use the fullrect and draw invisible borders

    switch (myUnitType) {
    case And:
        image.load(":/images/nodeTypes/AND.png");
        painter->drawImage(boundingRect(),image);
            break;
    case Nand:
        image.load(":/images/nodeTypes/NAND.png");
        painter->drawImage(boundingRect(),image);
            break;
    case Or:
        image.load(":/images/nodeTypes/OR.png");
        painter->drawImage(boundingRect(),image);
            break;
    case Nor:
        image.load(":/images/nodeTypes/NOR.png");
        painter->drawImage(boundingRect(),image);
            break;
    case Xor:
        image.load(":/images/nodeTypes/XOR.png");
        painter->drawImage(boundingRect(),image);
            break;
    case Xnor:
        image.load(":/images/nodeTypes/XNOR.png");
        painter->drawImage(boundingRect(),image);
            break;
    case Not:
        image.load(":/images/nodeTypes/NOT.png");
        painter->drawImage(boundingRect(),image);
            break;
    case MUX:
        image.load(":/images/nodeTypes/MUX.png");
        painter->drawImage(boundingRect(),image);
            break;
    case ADDER_FUNC:
        image.load(":/images/nodeTypes/ADDER_FUNC.png");
        painter->drawImage(boundingRect(),image);
            break;
    case CARRY_FUNC:
        image.load(":/images/nodeTypes/CARRY_FUNC.png");
        painter->drawImage(boundingRect(),image);
        break;
    case MEMORY:
        image.load(":/images/nodeTypes/Hmemory.png");
        painter->drawImage(boundingRect(),image);
        break;
    case Module:
        image.load(":/images/nodeTypes/module.png");
        painter->drawImage(boundingRect(),image);
        break;
    case MINUS:
        image.load(":/images/nodeTypes/Hminus.png");
        painter->drawImage(boundingRect(),image);
        break;
    case ADD:
        image.load(":/images/nodeTypes/Hadd.png");
        painter->drawImage(boundingRect(),image);
        break;
    case MULTIPLY:
        image.load(":/images/nodeTypes/Hmult.png");
        painter->drawImage(boundingRect(),image);
        break;
    case LogicGate:
    case Input:
    case Output:
    case Latch:
    case Clock:
    default:
        break;
    }


    painter->setFont(QFont("Times", 9));

    painter->setPen(QPen(Qt::red));
    painter->drawText(boundingRect(),
                      Qt::TextWrapAnywhere |
                      Qt::AlignVCenter |
                      Qt::AlignCenter,
                      myName);
    painter->setPen(QPen(Qt::black));
}

/*---------------------------------------------------------------------------------------------
 * (function: getName)
 *-------------------------------------------------------------------------------------------*/
QString LogicUnit::getName(){
    return myName;
}

/*---------------------------------------------------------------------------------------------
 * (function: getOutCons)
 *-------------------------------------------------------------------------------------------*/
QList<Wire *> LogicUnit::getOutCons()
{
    QList<Wire *> outgoing;
    //An outgoing connection has THIS as startUnit
   foreach (Wire *wire,wires) {
        if(wire->startUnit()->getName().compare(myName)==0)
            outgoing.append(wire);
    }
   return outgoing;
}

/*---------------------------------------------------------------------------------------------
 * (function: getOutConsHash)
 *-------------------------------------------------------------------------------------------*/
QHash<QString, Wire *> LogicUnit::getOutConsHash()
{
    QHash <QString, Wire *> outgoing;
    //An outgoing connection has THIS as startUnit
   foreach (Wire *wire,wires) {
        if(wire->startUnit()->getName().compare(myName)==0)
            outgoing.insert(wire->endUnit()->getName(), wire);
    }
   return outgoing;
}

/*---------------------------------------------------------------------------------------------
 * (function: setLayer)
 *-------------------------------------------------------------------------------------------*/
void LogicUnit::setLayer(int layer)
{
    myLayer = layer;
}

/*---------------------------------------------------------------------------------------------
 * (function: getLayer)
 *-------------------------------------------------------------------------------------------*/
int LogicUnit::getLayer()
{
    return myLayer;
}

/*---------------------------------------------------------------------------------------------
 * (function: updateWires)
 *-------------------------------------------------------------------------------------------*/
void LogicUnit::updateWires()
{
    foreach(Wire *wire, wires){
        wire->updateWireStatus();
        wire->updatePosition();
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: setName)
 *-------------------------------------------------------------------------------------------*/
void LogicUnit::setName(QString name)
{
    myName = name;
}

/*---------------------------------------------------------------------------------------------
 * (function: getAllCons)
 *-------------------------------------------------------------------------------------------*/
QList<Wire *> LogicUnit::getAllCons()
{
    return wires;
}

/*---------------------------------------------------------------------------------------------
 * (function: getOutValue)
 *-------------------------------------------------------------------------------------------*/
/**
  *returns the value of pinNumber. pinNumber starts with 1.
  */
int LogicUnit::getOutValue(int pinNumber)
{
    /*During the creation of the node the
    Output count might be unknown. Connections
    know their output pin. If the connection requests a higher number
    than numbers available, add zeros.*/
    //FIXME read above
    while(pinNumber>myOutVal.size()){
        //add zeros until size is big enough in
        //case the list was not expanded yet.
        QList<int> list;
        myOutVal.append(list);
        myOutVal[myOutVal.size()-1].append(0);
    }

    //append cycles if required
    while(myCurrentCycle>=myOutVal[pinNumber-1].size()){
        //add zeros until size is big enough in
        //case the list was not expanded yet.
        myOutVal[pinNumber].append(0);
    }

    //pinNumber-1 as wire objects start numbering with 1 instead of 0
    return myOutVal[pinNumber-1].at(myCurrentCycle);
}

/*---------------------------------------------------------------------------------------------
 * (function: setOutValue)
 *-------------------------------------------------------------------------------------------*/
/*Sets the output value(starts with 0) of the logic block. Only 1 and 0 can be set.
  If the variable 'value' is greater that 1 it is set to 1. If its smaller than 0 it is 0*/
int LogicUnit::setOutValue(int pinNumber, int value, int cycle)
{
    int result = -1;

    //append outputs if needed
    while(pinNumber>=myOutVal.size()){
        //add zeros until size is big enough in
        //case the list was not expanded yet.
        QList<int> list;
        myOutVal.append(list);
        myOutVal[myOutVal.size()-1].append(0);
    }

    //append cycles if required
    while(cycle>=myOutVal[pinNumber].size()){
        //add zeros until size is big enough in
        //case the list was not expanded yet.
        myOutVal[pinNumber].append(0);
    }

    //make sure only 1, 0 and -1 are legal
    //all illegal values are made -1
    if(value == 1){
        myOutVal[pinNumber].replace(cycle, value);
        result = 1;
    }else if(value == 0){
        myOutVal[pinNumber].replace(cycle, value);
        result = 1;
    }else{
        myOutVal[pinNumber].replace(cycle, -1);
        result = 1;
    }
    return result;
}

/*---------------------------------------------------------------------------------------------
 * (function: setOdinRef)
 *-------------------------------------------------------------------------------------------*/
void LogicUnit::setOdinRef(nnode_t *odinNode)
{
    myOdinNode = odinNode;
    hasOdinNode = true;
}

/*---------------------------------------------------------------------------------------------
 * (function: getOdinRef)
 *-------------------------------------------------------------------------------------------*/
nnode_t * LogicUnit::getOdinRef()
{
    return myOdinNode;
}

/*---------------------------------------------------------------------------------------------
 * (function: hasOdinReference)
 *-------------------------------------------------------------------------------------------*/
bool LogicUnit::hasOdinReference()
{
    return hasOdinNode;
}

/*---------------------------------------------------------------------------------------------
 * (function: getParents)
 *-------------------------------------------------------------------------------------------*/
QList<LogicUnit*> LogicUnit::getParents()
{
    QList<LogicUnit*> result;
    foreach(Wire* wire, wires){
        if(wire->endUnit()->getName().compare(myName) == 0){
            result.append(wire->startUnit());
        }
    }
    return result;
}

/*---------------------------------------------------------------------------------------------
 * (function: isHighlighted)
 *-------------------------------------------------------------------------------------------*/
bool LogicUnit::isHighlighted()
{
    return highlighted;
}

/*---------------------------------------------------------------------------------------------
 * (function: setHighlighted)
 *-------------------------------------------------------------------------------------------*/
void LogicUnit::setHighlighted(bool value)
{
    highlighted = value;
}

/*---------------------------------------------------------------------------------------------
 * (function: setCurrentCycle)
 *-------------------------------------------------------------------------------------------*/
void LogicUnit::setCurrentCycle(int cycle)
{
    myCurrentCycle = cycle;
    updateWires();
}

bool LogicUnit::isShown()
{
    return shown;
}

void LogicUnit::setShown(bool value)
{
    shown = value;
    setVisible(value);
}

/*---------------------------------------------------------------------------------------------
 * (function: getChildren)
 *-------------------------------------------------------------------------------------------*/
QList<LogicUnit*> LogicUnit::getChildren(){
    QList<LogicUnit*> children;
    //An outgoing connection has THIS as startUnit
   foreach (Wire *wire,wires) {
        if(wire->startUnit()->getName().compare(myName)==0)
            children.append(wire->endUnit());
    }

   return children;
}

/*
  Connects modules with nodes and the other way round
*/
void LogicUnit::addPartner(LogicUnit *partner)
{
    moduleViewPartners.append(partner);
    if(myUnitType != LogicUnit::Module)
        hasModule = true;
}

LogicUnit * LogicUnit::getModule()
{
    return moduleViewPartners.at(0);
}

int LogicUnit::getMaxOutNumber()
{
    return outNodes.count();
}

int LogicUnit::getMaxNumber()
{
    return myIncount;
}

void LogicUnit::showActivity()
{
    int activity = 0;

    if(wires.count()>0){
        foreach(Wire* wire, wires){
            activity += wire->getActivity();
        }
        int count = wires.count();
        activity = activity / wires.count();
        setBrush(QColor(activity,255-activity,0));
    }


}

