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
#include <QDebug>
#include <QtGui>

#include "wire.h"
#include <math.h>

const qreal Pi = 3.14;

//constructor. A start and an endpoint is needed for each wire
Wire::Wire(LogicUnit *startUnit, LogicUnit *endUnit,
          QGraphicsItem *parent, QGraphicsScene *scene):QGraphicsLineItem(parent, scene)
{
    myStartUnit = startUnit;
    myEndUnit = endUnit;
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    //set default color to black, can later be used to highlight selected items
    myColor = Qt::black;
    mySafeColor = Qt::black;

    //The values for an turned on and turned off wire
   // wireOff = QColor(0,0,255);
    wireOn = QColor(255,0,0);

    //penwidth can be used to show "active" wires after simulation
    myPenwidth = 3;

    setPen(QPen(myColor,myPenwidth,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    //myNumber is always set by the logic block wich creates it. 0 is default until setNumber is called
    myNumber=0;
    //output position which is needed for hord blocks
    myOutPinNumber = 1;
    myOutPinMax = 1;
}

/*---------------------------------------------------------------------------------------------
 * (function: type)
 *-------------------------------------------------------------------------------------------*/
int Wire::type() const
{
    return Type;
}

/*---------------------------------------------------------------------------------------------
 * (function: boundingRect)
 *-------------------------------------------------------------------------------------------*/
//calculate the bounding rect to tell the graphics view where to update
QRectF Wire::boundingRect() const
{
    qreal extra = (pen().width()+20)/2.0;

    return QRectF(line().p1(), QSizeF(line().p2().x() - line().p1().x(),
                                      line().p2().y() - line().p1().y()))
        .normalized()
        .adjusted(-extra, -extra, extra, extra);
}
/*---------------------------------------------------------------------------------------------
 * (function: shape)
 *-------------------------------------------------------------------------------------------*/
QPainterPath Wire::shape() const
{
//    QPainterPath path = QGraphicsLineItem::shape();
//    return path;

    QLineF myLine = line();
    QPainterPath path;
    qreal x1, x2, y1,y2;
    x1 = myLine.p1().x();
    x2=myLine.p2().x();
    y1=myLine.p1().y();
    y2=myLine.p2().y();

    //define corner points for rectangular connections
    path.moveTo(x1,y1);
    path.lineTo(x1+(x2-x1)/2,y1);
    path.moveTo(x1+(x2-x1)/2,y1);
    path.lineTo(x1+(x2-x1)/2,y2);
    path.moveTo(x1+(x2-x1)/2,y2);
    path.lineTo(x2,y2);

    return path;
}

/*---------------------------------------------------------------------------------------------
 * (function: setColor)
 *-------------------------------------------------------------------------------------------*/
//changes the color of the wire
void Wire::setColor(const QColor &color)
{
    myColor = color;
    mySafeColor = color;
}

/*---------------------------------------------------------------------------------------------
 * (function: startUnit)
 *-------------------------------------------------------------------------------------------*/
LogicUnit * Wire::startUnit() const
{
    return myStartUnit;
}

/*---------------------------------------------------------------------------------------------
 * (function: endUnit)
 *-------------------------------------------------------------------------------------------*/
LogicUnit * Wire::endUnit() const
{
    return myEndUnit;
}
/*---------------------------------------------------------------------------------------------

 * (function: updatePosition)
 *-------------------------------------------------------------------------------------------*/
void Wire::updatePosition()
{
    //draw line according to start and end positions
    double inputPosition = ((float)myNumber)/(myMaxNumber+1);
    inputPosition = inputPosition*100;
    double outputPosition = ((float)myOutPinNumber)/(myOutPinMax+1);
    outputPosition = outputPosition*100;
    QLineF line(mapFromItem(myStartUnit,50,outputPosition-50),mapFromItem(myEndUnit,-50,inputPosition-50));
    setLine(line);
//    QLineF line(mapFromItem(myStartUnit,50,0),mapFromItem(myEndUnit,-50,0));
//    setLine(line);
}

/*---------------------------------------------------------------------------------------------
 * (function: paint)
 *-------------------------------------------------------------------------------------------*/
//the most important function, which draws the wire
void Wire::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    //if the modules collide, do not paint a connection
    if(myStartUnit->collidesWithItem(myEndUnit))
    {
        return;
    }

    //if connection can be drawn
    QPen myPen = pen();
    myPen.setColor(myColor);
    myPen.setWidthF(myPenwidth);
    painter->setPen(myPen);
    painter->setBrush(myColor);

    //draw
    //painter->drawLine(line());
    if(isSelected())
    {
        painter->setPen(QPen(myColor,1,Qt::DashLine));
    }

    //line consists of multiple lines

    QPainterPath path;
    path = shape();


    //diagonal line
//    QLineF myLine = line();
//    painter->drawLine(myLine);

    //rectangular lines (have to be improved)
    painter->drawPath(path);


}

/*---------------------------------------------------------------------------------------------
 * (function: setNumber)
 *-------------------------------------------------------------------------------------------*/
void Wire::setNumber(int number)
{
    myNumber = number;
    updatePosition();
}

/*---------------------------------------------------------------------------------------------
 * (function: setMaxNumber)
 *-------------------------------------------------------------------------------------------*/
void Wire::setMaxNumber(int maxnumber)
{
    //if max number was adjusted, adjust your parameters and the position
    myMaxNumber = maxnumber;
    updatePosition();
}

/*---------------------------------------------------------------------------------------------
 * (function: undateWireStatus)
 *-------------------------------------------------------------------------------------------*/
/* Updates the width of the Wire according to the output the Wire is connected to*/
void Wire::updateWireStatus()
{
    int status;
    status = myStartUnit->getOutValue(myOutPinNumber);

    if(status==0){
        //myPenwidth = wireOff;
        myColor = mySafeColor;
    } else if(status == 1){
        myColor = QColor(wireOn.red(),mySafeColor.green(),mySafeColor.blue());;
        //myColor = mySafeColor;
    }else{
        //myPenwidth = wireOn;
        myColor = QColor(mySafeColor.red(),255,mySafeColor.blue());
    }
    update();

}

/*---------------------------------------------------------------------------------------------
 * (function: getMyCount)
 *-------------------------------------------------------------------------------------------*/
int Wire::getMyCount()
{
    return myNumber;
}

/*---------------------------------------------------------------------------------------------
 * (function: getMaxCount)
 *-------------------------------------------------------------------------------------------*/
int Wire::getMaxCount()
{
    return myMaxNumber;
}

/*---------------------------------------------------------------------------------------------
 * (function: getMyOutputPinNumber)
 *-------------------------------------------------------------------------------------------*/
int Wire::getMyOutputPinNumber()
{
    return myOutPinNumber;
}

/*---------------------------------------------------------------------------------------------
 * (function: getMaxOutputPinNumber)
 *-------------------------------------------------------------------------------------------*/
int Wire::getMaxOutputPinNumber()
{
    return myOutPinMax;
}

/*---------------------------------------------------------------------------------------------
 * (function: setMyOutputPinNumber)
 *-------------------------------------------------------------------------------------------*/
void Wire::setMyOutputPinNumber(int number)
{
    myOutPinNumber = number;
    updatePosition();
}

/*---------------------------------------------------------------------------------------------
 * (function: setMaxOutputPinNumber)
 *-------------------------------------------------------------------------------------------*/
void Wire::setMaxOutputPinNumber(int number)
{
    myOutPinMax = number;
    updatePosition();
}

/*---------------------------------------------------------------------------------------------
 * (function: getMyInputPosition)
 *-------------------------------------------------------------------------------------------*/
QPointF Wire::getMyInputPosition()
{
    double i = ((float)myNumber)/(myMaxNumber+1);
    i = i*100;
    //QLineF line(mapFromItem(myStartUnit,50,0),mapFromItem(myEndUnit,-50,i-50));
    return QPointF(-50,i-50);
}


