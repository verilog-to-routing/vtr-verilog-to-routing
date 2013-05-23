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
#ifndef CONTAINER_H
#define CONTAINER_H

#include "logicunit.h"
#include "wire.h"
#include "explorerscene.h"
#include "odininterface.h"
#include<QHash>

class Container
{

public:
    Container(ExplorerScene* scene);
    void setFilename(QString filename);
    void addUnit(QString name, LogicUnit::UnitType type, QPointF position);
    LogicUnit * addModule(QString name);
    LogicUnit * addUnit(QString name,LogicUnit::UnitType type, QPointF position, nnode_t* odinRef);
    void addConnection(QString start, QString end);
    bool addConnectionHash(QString start, QString end);
    void arrangeContainer();
    void deleteModule(QString name);
    void clearContainer();
    int readInFile();
    int readInFileOdin();
    int readInFileOdinOnly();
    QHash<QString,LogicUnit *> findByName(QString name);
    LogicUnit *getReferenceToUnit(QString name);
    int getMaxSimStep();
    int getActSimStep();
    int startSimulator();
    void showSimulationStep(int cycle);
    void setEdge(int i);
    int simulateNextWave();
    void resetAllHighlights();
    void showActivity();
    void getActivityInformation();
    void setVisibilityForAll(bool value);
    QList<LogicUnit*> getClocks();
    void expandCollapse(QString modulename);
    QString extractModuleFromName(QString name);
private:
    Wire* getConnectionBetween(QString nodeName, QString kidName);
    void computeLayers();
    void spreadLayers();
    void computeLayersHash();
    void setLayer(QString unitName, int layercount);
    int createInputs();
    int createNodes();
    int createNodesFromOdin();
    int createConnectionsFromOdin();
    int createConnectionsFromOdinIterate();
    int createLatches();
    int createConnections();
    int getNodeListFromOdin();
    void conectNodeToLogicUnit(nnode_t *node, QString name);
    void startOdin();
    bool parentsDone(LogicUnit* unit, QHash<QString,LogicUnit*> donehashlist);
    bool parentsDone(LogicUnit *unit);
    int getMaxParentLayer(LogicUnit* node);
    void copySimCyclesIntoNodes();
    void assignToModule(QString actName);
    void assignNodeToModule(QString nodeName, QString moduleName);


    QString myFilename;
    QList<LogicUnit *> unitcontainer;
    ExplorerScene* myScene;
    QHash<QString, LogicUnit *> unithashtable;
    QHash<QString, LogicUnit *> arrHashtable;
    int maxlayer;
    int maxcountPerLayer;
    int myBlockCount;
    OdinInterface* myOdin;
    bool odinStarted;
    QHash<QString, nnode_t *> odinTable;
    int myItemcount;
    QHash<QString, LogicUnit *> completeNodes;
    int simOffset, maxSimStep, actSimStep;
    QList<LogicUnit*> clocks;

    };


#endif // CONTAINER_H
