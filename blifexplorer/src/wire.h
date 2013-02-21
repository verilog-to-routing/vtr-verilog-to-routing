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
#ifndef WIRE_H
#define WIRE_H

#include <QGraphicsLineItem>
#include "logicunit.h"

QT_BEGIN_NAMESPACE
class QGraphicsPolygonItem;
class QGraphicsLineItem;
class QGraphicsScene;
class QRectF;
class QGraphicsSceneMouseEvent;
class QPainterPath;
QT_END_NAMESPACE

class Wire:public QGraphicsLineItem
{
public:
    enum { Type = UserType + 4 };

    Wire(LogicUnit *startUnit, LogicUnit *endUnit,
      QGraphicsItem *parent = 0, QGraphicsScene *scene = 0);
    int type() const;
    QRectF boundingRect() const;
    QPainterPath shape() const;
    void setColor(const QColor &color);
    LogicUnit *startUnit() const;
    LogicUnit *endUnit() const;
    void setNumber(int number);
    void setMaxNumber(int maxnumber);
    void updateWireStatus();
    int getMyCount();
    int getMaxCount();
    int getMyOutputPinNumber();
    int getMaxOutputPinNumber();
    void setMyOutputPinNumber(int number);
    void setMaxOutputPinNumber(int number);

    void setNumberModule(int number);
    void setMaxNumberModule(int maxnumber);
    int getMyCountModule();
    int getMaxCountModule();
    int getMyOutputPinNumberModule();
    int getMaxOutputPinNumberModule();
    void setMyOutputPinNumberModule(int number);
    void setMaxOutputPinNumberModule(int number);

    QPointF getMyInputPosition();
    int getActivity();
    void setActivity(int value);
    void showActivity();
    void setStartModule(LogicUnit* start);
    void setEndModule(LogicUnit* end);
    LogicUnit* getStartModule();
    LogicUnit* getEndModule();
    bool hasStartModule, hasEndModule;


public slots:
    void updatePosition();
    void updatePosition(int input, int inputcount);
protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = 0);
private:
    LogicUnit *myStartUnit, *myEndUnit;
    LogicUnit *myStartModule, *myEndModule;
    bool startFromModule();
    bool endOnModule();
    QColor myColor;
    QColor mySafeColor;
    qreal myPenwidth;
    int myNumber;
    int myMaxNumber;
    int myOutPinNumber;
    int myOutPinMax;
    //in case modules are involved
    int myNumberModule;
    int myMaxNumberModule;
    int myOutPinNumberModule;
    int myOutPinMaxModule;
    QColor wireOff;
    QColor wireOn;
    int activity;


};

#endif // WIRE_H
