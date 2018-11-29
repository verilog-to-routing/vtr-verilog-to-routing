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
#include "container.h"
#include <time.h>

Container::Container(ExplorerScene* scene)
{
    myScene = scene;
    maxlayer = 0;
    maxcountPerLayer = 0;
    myOdin = new OdinInterface();
    odinStarted = false;
    simOffset = 0;
    maxSimStep = 0;
    actSimStep = 0;
}

/*---------------------------------------------------------------------------------------------
 * (function: setFilename)
 *-------------------------------------------------------------------------------------------*/
void Container::setFilename(QString filename)
{
    myFilename = filename;
    myOdin->setFilename(filename);
}

/*---------------------------------------------------------------------------------------------
 * (function: addUnit)
 *-------------------------------------------------------------------------------------------*/
LogicUnit * Container::addUnit(QString name,LogicUnit::UnitType type, QPointF position,
                               nnode_t* odinRef)
{
    LogicUnit* newUnit;
    //create in scene
    newUnit = myScene->addLogicUnit(name,type,position);
    newUnit->setPos(position);
    newUnit->setOdinRef(odinRef);
    unithashtable[name] = newUnit;
    unitcontainer.append(newUnit);
    return newUnit;
}

/*---------------------------------------------------------------------------------------------
 * (function: addModule)
 *-------------------------------------------------------------------------------------------*/
LogicUnit * Container::addModule(QString name)
{
    LogicUnit* newUnit;
    //create in scene
    newUnit = myScene->addLogicUnit(name,LogicUnit::Module,QPointF(100,100));
    newUnit->setPos(QPointF(100,100));
    unithashtable[name] = newUnit;
    unitcontainer.append(newUnit);
    return newUnit;
}

/*---------------------------------------------------------------------------------------------
 * (function: addConnectionHash)
 *-------------------------------------------------------------------------------------------*/
bool Container::addConnectionHash(QString start, QString end)
{
    //do not connect modules with themself
    if(start.compare(end) == 0)
        return false;

    bool startFound = false;
    bool endFound = false;
    bool success = false;
    LogicUnit *startUnit, *endUnit;
    if(unithashtable.contains(start)){
        startUnit = unithashtable.value(start);
        startFound = true;
    }
    if(unithashtable.contains(end)){
        endUnit = unithashtable.value(end);
        endFound = true;
    }
    if(startFound && endFound){
        myScene->addConnection(startUnit, endUnit);
        success = true;
    }
    return success;

}

/*---------------------------------------------------------------------------------------------
 * (function: arrangeContainer)
 *-------------------------------------------------------------------------------------------*/
//arrange the modules according to their layer.
//at the moment wire crossing is not considered
void Container::arrangeContainer()
{
    computeLayers();
    spreadLayers();
    myScene->setSceneRect(0,0,1000.0+400*maxlayer+maxcountPerLayer*50,1000.0+400*maxcountPerLayer);
}

/*---------------------------------------------------------------------------------------------
 * (function: deleteModule)
 *-------------------------------------------------------------------------------------------*/
void Container::deleteModule(QString name)
{
    for(int i = 0;i<unitcontainer.count();i++){
        LogicUnit *temp = unitcontainer.at(i);
        if(temp->getName().compare(name)==0){
            unitcontainer.removeAt(i);
        }
    }
    if(unithashtable.contains(name))
        unithashtable.remove(name);
}

/*---------------------------------------------------------------------------------------------
 * (function: clearContainer)
 *-------------------------------------------------------------------------------------------*/
//delete everything in the container
void Container::clearContainer()
{
    while(!unitcontainer.empty()){
        unitcontainer.removeFirst();
    }
    unithashtable.clear();
}

/*---------------------------------------------------------------------------------------------
 * (function: setLayer)
 *-------------------------------------------------------------------------------------------*/
void Container::setLayer(QString unitName, int layercount){
    QString actName;

    foreach (LogicUnit *actUnit, unitcontainer) {
        actName = actUnit->getName();
        if(actName.compare(unitName)==0){
            if(layercount > actUnit->getLayer()){
                   actUnit->setLayer(layercount);
            }
        }
    }

}

/*---------------------------------------------------------------------------------------------
 * (function: computeLayersHash)
 *-------------------------------------------------------------------------------------------*/
//A smarter function to compute the layers.
void Container::computeLayersHash()
{
    QHash<QString, LogicUnit *> currentStep = arrHashtable;
    QHash<QString, LogicUnit *> nextStep;
    QHash<QString, LogicUnit *>::const_iterator blockIterator = currentStep.constBegin();
    LogicUnit *actUnit;

    for(int i = unitcontainer.count(); i>=0;i--){
        while(blockIterator != currentStep.constEnd()){
            actUnit = blockIterator.value();
            //adjust the layers of all "children"
            QList<Wire *> cons = actUnit->getOutCons();
            foreach(Wire *wire, cons){
                //go through all of the ends and set the layer to layer+1
                int newLayer = actUnit->getLayer()+1;
                wire->endUnit()->setLayer(newLayer);
                nextStep.insert(wire->endUnit()->getName(),wire->endUnit());
                if(newLayer > maxlayer){
                    maxlayer = newLayer;
                }
            }
            //get next Item
            ++blockIterator;
        }
        currentStep.clear();
        currentStep = nextStep;
        nextStep.clear();
        blockIterator = currentStep.constBegin();
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: getMaxParentLayer)
 *-------------------------------------------------------------------------------------------*/
int Container::getMaxParentLayer(LogicUnit* node){
    QList<LogicUnit*> parents = node->getParents();
    int result = -1;
    foreach(LogicUnit* parent, parents){
        if(parent->getLayer() > result){
            result = parent->getLayer();
        }
    }
    return result;
}

/*---------------------------------------------------------------------------------------------
 * (function: copySimCyclesintoNodes)
 *-------------------------------------------------------------------------------------------*/
void Container::copySimCyclesIntoNodes()
{   //for each cycle which was simulated         //int value;
    //int i;, copy the values into the nodes
    int cycle;
    for(cycle=0+simOffset;cycle<maxSimStep;cycle++){
        QHash<QString, nnode_t *>::const_iterator blockIterator = odinTable.constBegin();

        while(blockIterator != odinTable.constEnd()){
             int value;
             int i;
             QString name = blockIterator.key();
             LogicUnit* visNode = unithashtable[name];
             nnode_t* node = blockIterator.value();
             //for each output pin of the node update the current value
             for(i=0;i<node->num_output_pins;i++){
                 value = myOdin->getOutputValue(node, i, cycle);
                 visNode->setOutValue(i, value, cycle);
                 visNode->updateWires();
             }
             ++blockIterator;
        }
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: computeLayers)
 *-------------------------------------------------------------------------------------------*/
//compute the layers of all modules inside the container
void Container::computeLayers()
{
    QQueue<LogicUnit*> nodequeue;
    QHash<QString, LogicUnit*> donehashtable;
    for (int i = 0; i < myOdin->blifexplorer_netlist->num_top_input_nodes; i++){
        QString name = myOdin->blifexplorer_netlist->top_input_nodes[i]->name;
        nodequeue.enqueue(getReferenceToUnit(name));
    }

    // Enqueue constant nodes.
    nnode_t* constant_nodes[] = {myOdin->blifexplorer_netlist->gnd_node,
                                 myOdin->blifexplorer_netlist->vcc_node,
                                 myOdin->blifexplorer_netlist->pad_node};
    int num_constant_nodes = 3;
    for (int i = 0; i < num_constant_nodes; i++){
        QString name = constant_nodes[i]->name;
        nodequeue.enqueue(getReferenceToUnit(name));
    }

    // go through the netlist. While doing so
    // remove nodes from the queue and add followup nodes
    LogicUnit* node;
    int maxparent;
    while(!nodequeue.isEmpty()){
        node = nodequeue.dequeue();
        //remember name of the node so it is not processed again
        QString nodeName(node->getName());
        //assign layer
        maxparent = getMaxParentLayer(node);

        //only if the node is visible it is considered for a layer
        if(node->isVisible()){
            node->setLayer(maxparent+1);
        }
        if(node->getLayer()>maxlayer){
            maxlayer = node->getLayer();
        }
        //save node in hash, so it will be processed only once
        donehashtable[nodeName] = node;
        //Enqueue child nodes which are ready
        int num_children = 0;
        nnode_t** children = get_children_of(node->getOdinRef(), &num_children);

        // connect node to all children
        //enqueue children if not already done or in queue
        for(int i=0; i< num_children; i++){
            nnode_t* nodeKid = children[i];
            QString kidName(nodeKid->name);
            LogicUnit* kidUnit = getReferenceToUnit(kidName);

            bool inQueue = nodequeue.contains(kidUnit);
            bool done = donehashtable.contains(kidName);

            if(!inQueue && !done && parentsDone(kidUnit,donehashtable)){
                nodequeue.enqueue(kidUnit);
            }
        }
    }
    maxlayer++;
    /*Locate all outputs at the very end of the graph*/
    QHash<QString, LogicUnit*>::const_iterator blockIterator = unithashtable.constBegin();
    for(int i = 0; i<=maxlayer;i++){
        blockIterator = unithashtable.constBegin();
        while(blockIterator != unithashtable.constEnd()){
            LogicUnit* actUnit = blockIterator.value();
            if(actUnit->getName().contains("top^out")){
                actUnit->setLayer(maxlayer);
            }

            ++blockIterator;
        }
    }

}

/*---------------------------------------------------------------------------------------------
 * (function: spreadLayers)
 *-------------------------------------------------------------------------------------------*/
void Container::spreadLayers()
{
    QHash<QString, LogicUnit*>::const_iterator blockIterator = unithashtable.constBegin();
    LogicUnit* lastUnit = NULL;
    int counter = 0;
    int offset = 200;

    for(int i = 0; i<=maxlayer;i++){
        blockIterator = unithashtable.constBegin();

        while(blockIterator != unithashtable.constEnd()){
            LogicUnit* actUnit = blockIterator.value();
            if(actUnit->isVisible()){
                if(actUnit->getLayer()== i){
                    actUnit->setPos(offset+15*counter,100.0+200*counter);
                    actUnit->updateWires();
                    lastUnit = actUnit;
                    counter++;
                }
                if(maxcountPerLayer < counter){
                    maxcountPerLayer = counter;
                }
            }
            ++blockIterator;
        }
        if(lastUnit!=NULL){
            offset = lastUnit->x()+200;
        }else{
            offset = 200;
        }

        counter = 0;

    }

}

/*---------------------------------------------------------------------------------------------
 * (function: startOdin)
 *-------------------------------------------------------------------------------------------*/
void Container::startOdin(){
    int error_code = 0;
    if(!odinStarted){
        myOdin->setFilename(myFilename);
        error_code = myOdin->startOdin();
    }
    else
    {
        std::cout << "odin is already started";
    }

    odinStarted = (error_code == 0)? true: false;
    
    if(odinStarted){
        odinTable = myOdin->getNodeTable();
    }
    else
    {
        std::cout << "odin failed to start with error:" << std::to_string(error_code);
        exit(1);
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: createNodesFromOdin)
 *-------------------------------------------------------------------------------------------*/
int Container::createNodesFromOdin(){
    QHash<QString, nnode_t *>::const_iterator blockIterator = odinTable.constBegin();
    int items = 0;
    int odinNodeCount = odinTable.count();
     while(blockIterator != odinTable.constEnd()){
         QString actName = blockIterator.key();
         nnode_t* actOdinNode = blockIterator.value();
         operation_list odinType = actOdinNode->type;
         LogicUnit::UnitType type;

         switch(odinType){
         case CLOCK_NODE:
             type = LogicUnit::Clock;
             break;
         case INPUT_NODE:
             type = LogicUnit::Input;
            break;
         case FF_NODE:
             type = LogicUnit::Latch;
             break;
         case OUTPUT_NODE:
             type = LogicUnit::Output;
             break;
         case LOGICAL_AND:
             type = LogicUnit::And;
             break;
         case LOGICAL_OR:
             type = LogicUnit::Or;
             break;
         case LOGICAL_NOT:
             type = LogicUnit::Not;
             break;
         case LOGICAL_NAND:
             type = LogicUnit::Nand;
             break;
         case LOGICAL_XOR:
             type = LogicUnit::Xor;
             break;
         case LOGICAL_XNOR:
             type = LogicUnit::Xnor;
             break;
         case LOGICAL_NOR:
             type = LogicUnit::Nor;
             break;
         case MUX_2:
             type = LogicUnit::MUX;
             break;
         case ADDER_FUNC:
             type = LogicUnit::ADDER_FUNC;
             break;
         case CARRY_FUNC:
             type = LogicUnit::CARRY_FUNC;
             break;
         case MULTIPLY:
             type = LogicUnit::MULTIPLY;
             break;
         case ADD:
             type = LogicUnit::ADD;
             break;
         case MINUS:
             type = LogicUnit::MINUS;
             break;
         case MEMORY:
             type = LogicUnit::MEMORY;
             break;
         default:
             type = LogicUnit::LogicGate;
             break;
         }
        //add Unit with name and type
         addUnit(actName, type,QPointF(100,100),actOdinNode);
         if(actName.contains("+")){
             assignToModule(actName);
             unithashtable[actName]->setVisible(false);
         }
         items++;
         if((100*items/odinNodeCount)%5==0)
         {
             fprintf(stdout, "VISUALIZATION: Node progress: %d\n", 100*items/odinNodeCount);
         }
         ++blockIterator;
     }
     fprintf(stderr, "Created Nodes from Netlist: %d\n", items);
     return items;
}

/*---------------------------------------------------------------------------------------------
 * (function: parentsDone)
 *-------------------------------------------------------------------------------------------*/
bool Container::parentsDone(LogicUnit *unit, QHash<QString, LogicUnit *> donehashlist)
{
    //in case it is a flipFlop the parents don't have to be done
    //FlipFlops can be parts of cycles.
    if(unit->unitType() == LogicUnit::Latch ||
            unit->unitType() == LogicUnit::MEMORY){
        return true;
    }

    bool result = true;
    QList<LogicUnit*> parents = unit->getParents();
    foreach(LogicUnit* parent,parents){
        if(!donehashlist.contains(parent->getName())){
            result = false;
        }
    }
    return result;
}

/*---------------------------------------------------------------------------------------------
 * (function: parentsDone)
 *-------------------------------------------------------------------------------------------*/
bool Container::parentsDone(LogicUnit *unit)
{
    bool result = true;
    QList<LogicUnit*> parents = unit->getParents();
    foreach(LogicUnit* parent,parents){
        if(!completeNodes.contains(parent->getName())){
            result = false;
        }
    }
    return result;
}


int Container::createConnectionsFromOdinIterate()
{
    //only create connections if nodes are created
    if(myItemcount <= 0)
        return -1;
    //create all connections while iterating through all nodes
    //leave out the clocks
    //(visual reasons. The clock should be added last to be at the right port)
    int conCount = 0;
    int itemNr = 0;
    int j,i, itemTotal;
    bool clocksFound = false;

    QHash<QString, LogicUnit *> hash =unithashtable;
    QHash<QString, LogicUnit *>::const_iterator blockIterator = hash.constBegin();
    itemTotal = hash.count();
     while(blockIterator != hash.constEnd()){
         QString actName = blockIterator.key();
         LogicUnit* actNode = blockIterator.value();

         if(actNode->unitType() == LogicUnit::Module){
             ++blockIterator;
             continue;
         }

         if(actNode->getOdinRef()->type == CLOCK_NODE){
             if(!clocks.contains(actNode)){
                clocks.append(actNode);
                clocksFound = true;
             }
         }else{//if the node is not a clock node, create all output connections
             int output_number = actNode->getOdinRef()->num_output_pins;
             for(j=0; j<output_number;j++)
             {
                 int num_children = 0;
                 nnode_t** children = get_children_of_nodepin(actNode->getOdinRef(), &num_children,j);
                 QHash<QString, nnode_t*> childrenDone;
                 for(i=0; i< num_children; i++){
                     nnode_t* nodeKid = children[i];
                     QString kidName(nodeKid->name);
                     //LogicUnit* kidUnit = getReferenceToUnit(kidName);

                     if(!childrenDone.contains(kidName)){
                         //bool success = addConnectionHashByRef(actNode,kidUnit);
                         bool success = addConnectionHash(actName,kidName);
                         if(!success){
                             /*FIXME: This error is sometimes created by Odin as
                                         the method get_children_of(node->getOdinRef(), &num_children)
                                         sometimes returns a node itself as its child.*/
                             fprintf(stderr, "CONTAINER:  Warning: Node has itself in childlist in Odin II:");
                             fprintf(stderr,"%s", ((char*)actName.toLocal8Bit().data()));
                             fprintf(stderr,"\n");
                         }else{
                         //for hard blocks add the output pin of the created connection
                             Wire* newWire = getConnectionBetween(actName, kidName);
                             newWire->setMaxOutputPinNumber(
                                         actNode->getOdinRef()->num_output_pins);
                             newWire->setMyOutputPinNumber(j+1);
                             //add modules to connection if needed
                             LogicUnit* startNode = unithashtable[actName];
                             LogicUnit* startModule;
                             LogicUnit* endNode = unithashtable[kidName];
                             LogicUnit* endModule;
                            if(startNode->hasModule && endNode->hasModule){
                                startModule = startNode->getModule();
                                endModule = endNode->getModule();
                                if(startModule->getName().compare(endModule->getName())!=0){
                                //both connections are in modules
                                    startModule->addConnection(newWire);
                                    endModule->addConnection(newWire);
                                    newWire->setEndModule(endModule);
                                    newWire->setStartModule(startModule);
                                }
                            }else if(startNode->hasModule){
                                startModule = startNode->getModule();
                                startModule->addConnection(newWire);
                                newWire->setStartModule(startModule);
                            }else if(endNode->hasModule){
                                endModule = endNode->getModule();
                                endModule->addConnection(newWire);
                                newWire->setEndModule(endModule);
                            }

                             childrenDone[kidName] = nodeKid;
                             conCount++;
                             if(conCount%500 == 0){
                                fprintf(stdout, "CONTAINER:  Connection progress: %d Cons, %d.%d Percent.\n", conCount, 100*itemNr/itemTotal,
                                        (10000*itemNr/itemTotal)%100);
                                fflush(stdout);
                             }
                         }
                     }
                 }
             }
         }

         //go through each output pin of the node and create all connections
         itemNr++;
         ++blockIterator;
     }

     //create all clock connections if clocks exist
     if(clocksFound){
         //for all clocks create the connections
         foreach(LogicUnit* clock, clocks){
             int num_children = 0;
             nnode_t** children = get_children_of(clock->getOdinRef(), &num_children);
             QString clockName = clock->getName();

             // connect clock to all children
             for(i=0; i< num_children; i++){
                 nnode_t* nodeKid = children[i];
                 QString kidName(nodeKid->name);
                 addConnectionHash(clockName,kidName);

                 Wire* newWire = getConnectionBetween(clockName, kidName);
                 LogicUnit* startNode = unithashtable[clockName];
                 LogicUnit* startModule;
                 LogicUnit* endNode = unithashtable[kidName];
                 LogicUnit* endModule;

                 if(startNode->hasModule && endNode->hasModule){
                     startModule = startNode->getModule();
                     endModule = endNode->getModule();
                     if(startModule->getName().compare(endModule->getName())!=0){
                    //both connections are in modules
                         startModule->addConnection(newWire);
                         endModule->addConnection(newWire);
                         newWire->setEndModule(endModule);
                         newWire->setStartModule(startModule);
                     }
                 }else if(startNode->hasModule){
                     startModule = startNode->getModule();
                     startModule->addConnection(newWire);
                     newWire->setStartModule(startModule);
                 }else if(endNode->hasModule){
                    endModule = endNode->getModule();
                    endModule->addConnection(newWire);
                    newWire->setEndModule(endModule);
                 }

                 conCount++;
             }
         }
     }
     return conCount;


}


/*---------------------------------------------------------------------------------------------
 * (function: createConnectionsFromOdin)
 *-------------------------------------------------------------------------------------------*/
int Container::createConnectionsFromOdin()
{
    //only create connections if nodes are created
    if(myItemcount <= 0)
        return -1;
    //create all connections while traversing through the netlist
    //leave out the clocks
    //(visual reasons. The clock should be added last to be at the right port)
    int i, j, conCount;
    conCount = 0;
    QQueue<LogicUnit*> nodequeue;
    QHash<QString, LogicUnit*> donehashtable;

    for (i = 0; i < myOdin->blifexplorer_netlist->num_top_input_nodes; i++){
        QString name = myOdin->blifexplorer_netlist->top_input_nodes[i]->name;
        nodequeue.enqueue(getReferenceToUnit(name));
    }

    // Enqueue constant nodes.
    nnode_t* constant_nodes[] = {myOdin->blifexplorer_netlist->gnd_node,
                                 myOdin->blifexplorer_netlist->vcc_node,
                                 myOdin->blifexplorer_netlist->pad_node};
    int num_constant_nodes = 3;
    for (i = 0; i < num_constant_nodes; i++){
        QString name = constant_nodes[i]->name;
        nodequeue.enqueue(getReferenceToUnit(name));
    }

    // go through the netlist. While doing so
    // remove nodes from the queue and add followup nodes
    LogicUnit* node;
    bool clockFound = false;
    while(!nodequeue.isEmpty()){
        node = nodequeue.dequeue();
        //remember name of the node so it is not processed again
        QString nodeName(node->getName());
        //save node in hash, so it will be processed only once
        donehashtable[nodeName] = node;
        //check each output pin for children.
        int output_number = node->getOdinRef()->num_output_pins;
        for(j=0; j<output_number;j++)
        {
            //Enqueue child nodes which are ready
            int num_children = 0;
            nnode_t** children = get_children_of_nodepin(node->getOdinRef(), &num_children,j);
            QHash<QString, nnode_t*> childrenDone;//used to not create a connection multiple times
            // connect node to all children
            //enqueue children if not already done or in queue
            for(i=0; i< num_children; i++){
                nnode_t* nodeKid = children[i];
                QString kidName(nodeKid->name);
                LogicUnit* kidUnit = getReferenceToUnit(kidName);

                bool inQueue = nodequeue.contains(getReferenceToUnit(kidName));
                bool done = donehashtable.contains(kidName);

                if(node->getOdinRef()->type != CLOCK_NODE)
                {
                    if(!childrenDone.contains(kidName)){
                        bool success = addConnectionHash(nodeName,kidName);
                        if(!success){
                            /*FIXME: This error is sometimes created by Odin as
                                        the method get_children_of(node->getOdinRef(), &num_children)
                                        sometimes returns a node itself as its child.*/
                            fprintf(stderr, "CONTAINER:  Warning: Node has itself in childlist in Odin II.\n");
                        }else{
                        //for hard blocks add the output pin of the created connection
                            Wire* newWire = getConnectionBetween(nodeName, kidName);
                            newWire->setMaxOutputPinNumber(
                                        getReferenceToUnit(nodeName)->getOdinRef()->num_output_pins);
                            newWire->setMyOutputPinNumber(j+1);
                            childrenDone[kidName] = nodeKid;
                            conCount++;
                        }
                    }
                }else if(!clocks.contains(node)){
                    clocks.append(node);
                    clockFound = true;
                }

                if(!inQueue && !done && parentsDone(kidUnit,donehashtable)){
                    nodequeue.enqueue(kidUnit);
                }else if(!inQueue && !done && kidUnit->getOdinRef()->type == FF_NODE){
                    nodequeue.enqueue(kidUnit);
                }
            }
        }
    }

    //create clock connections
    int num_children = 0;
    if(clockFound){
        //for all clocks create the connections
        foreach(LogicUnit* clock, clocks){
            nnode_t** children = get_children_of(clock->getOdinRef(), &num_children);
            QString clockName = clock->getName();

            // connect clock to all children
            for(i=0; i< num_children; i++){
                nnode_t* nodeKid = children[i];
                QString kidName(nodeKid->name);
                addConnectionHash(clockName,kidName);
                conCount++;
            }
        }
    }
    //return connection count
    return conCount;
}

/*---------------------------------------------------------------------------------------------
 * (function: readInFileOdinOnly)
 *-------------------------------------------------------------------------------------------*/
/**
  * Reads in the file in Odin and creates the nodes based on the netlist provided by odin.
  */
int Container::readInFileOdinOnly(){
    //used to measure the time, which is needed to create a graph
    clock_t start, finish;
    int cons;
    start = clock();
    //let odin ii parse in the file and return a hashtable of all nodes in the netlist
    startOdin();
    fprintf(stdout, "VISUALIZATION: Creating nodes...\n");
    //iterate through the hashtable and create all nodes based on the type
    myItemcount = createNodesFromOdin();
    fprintf(stdout, "VISUALIZATION: Creating Node connections...\n");
     //create connections
    cons = createConnectionsFromOdinIterate();
     if(myItemcount <= 0)
         return -1;

     finish = clock();
     fprintf(stderr, "Nodes: %d, Cons: %d, Time: %f\n", myItemcount, cons, (double)(finish - start)/CLOCKS_PER_SEC);
     return 0;

}

/*---------------------------------------------------------------------------------------------
 * (function: findByName)
 *-------------------------------------------------------------------------------------------*/
QHash<QString, LogicUnit *> Container::findByName(QString name)
{
    QHash<QString, LogicUnit *> result;
    QHash<QString, LogicUnit *> hash =unithashtable;
    QHash<QString, LogicUnit *>::const_iterator blockIterator = hash.constBegin();

     while(blockIterator != unithashtable.constEnd()){
         QString actName = blockIterator.key();
         if(actName.contains(name, Qt::CaseInsensitive)){
            result.insert(actName, blockIterator.value());
            blockIterator.value()->setBrush(QColor(23,255,255,255));
         }
         ++blockIterator;
     }
     return result;
}

/*---------------------------------------------------------------------------------------------
 * (function: getReferenceToUnit)
 *-------------------------------------------------------------------------------------------*/
LogicUnit * Container::getReferenceToUnit(QString name)
{
    if(!unithashtable.contains(name)){
        fprintf(stderr, "Container: Not all nodes could be connected!\n");
    }
    return unithashtable[name];
}

/*---------------------------------------------------------------------------------------------
 * (function: getMaxSimStep)
 *-------------------------------------------------------------------------------------------*/
int Container::getMaxSimStep()
{
    return maxSimStep;
}

/*---------------------------------------------------------------------------------------------
 * (function: getActSimiStep)
 *-------------------------------------------------------------------------------------------*/
int Container::getActSimStep()
{
    return actSimStep;
}

/*---------------------------------------------------------------------------------------------
 * (function: startSimulator)
 *-------------------------------------------------------------------------------------------*/
int Container::startSimulator()
{
    myOdin->setUpSimulation();
    maxSimStep = myOdin->simulateNextWave();
    copySimCyclesIntoNodes();
    /*
    myOdin->simulateNextWave();
    myOdin->endSimulation();
    copySimCyclesIntoNodes();*/
    return 0;
}

int Container::simulateNextWave()
{
    int success = -1;
    simOffset = maxSimStep;
    maxSimStep = myOdin->simulateNextWave();
    copySimCyclesIntoNodes();

    return success;
}

void Container::resetAllHighlights()
{
    QHash<QString, LogicUnit *> hash =unithashtable;
    QHash<QString, LogicUnit *>::const_iterator blockIterator = hash.constBegin();

     while(blockIterator != unithashtable.constEnd()){
        LogicUnit *unit = blockIterator.value();
        unit->setBrush(QColor(255,255,255,255));
        QList<Wire *> list = unit->getAllCons();
        foreach (Wire *wire, list) {
            wire->setColor(QColor(0,0,0,255));
            wire->setZValue(-1000);
            wire->update();
        }
         ++blockIterator;
     }
}

void Container::setVisibilityForAll(bool value)
{
    QHash<QString, LogicUnit *> hash =unithashtable;
    QHash<QString, LogicUnit *>::const_iterator blockIterator = hash.constBegin();

     while(blockIterator != unithashtable.constEnd()){
        LogicUnit *unit = blockIterator.value();
        unit->setShown(value);
        QList<Wire *> list = unit->getAllCons();
        foreach (Wire *wire, list) {
            wire->updatePosition();
        }
         ++blockIterator;
     }
}

void Container::showActivity()
{
    QHash<QString, nnode_t *>::const_iterator blockIterator = odinTable.constBegin();

    while(blockIterator != odinTable.constEnd()){
         QString name = blockIterator.key();
         LogicUnit* visNode = unithashtable[name];
         //get all connections outgoing and advise them to
         //represent the activity by color
         QList<Wire*> outgoingWires = visNode->getOutCons();
         foreach(Wire* wire, outgoingWires){
             wire->showActivity();
             wire->update();
         }
         visNode->showActivity();

         ++blockIterator;
    }

}

void Container::getActivityInformation()
{
    //right now it is a dummy function. the activity values are
    //generated randomly
    QHash<QString, nnode_t *>::const_iterator blockIterator = odinTable.constBegin();

    while(blockIterator != odinTable.constEnd()){
         QString name = blockIterator.key();
         LogicUnit* visNode = unithashtable[name];
         //get all connections outgoing and advise them to
         //represent the activity by color
         QList<Wire*> outgoingWires = visNode->getOutCons();
         foreach(Wire* wire, outgoingWires){
             int act = qrand() % 255;
             wire->setActivity(act);

         }
        ++blockIterator;
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: showSimulationStep)
 *-------------------------------------------------------------------------------------------*/
void Container::showSimulationStep(int cycle)
{
    actSimStep = cycle;

    //if current step is bigger than the last cycle, create new values
    if(actSimStep>maxSimStep){
        maxSimStep = myOdin->simulateNextWave();
        copySimCyclesIntoNodes();
    }

    //visit each node and update the output status of each one of them
    QHash<QString, nnode_t *>::const_iterator blockIterator = odinTable.constBegin();

    while(blockIterator != odinTable.constEnd()){
         QString name = blockIterator.key();
         LogicUnit* visNode = unithashtable[name];
         visNode->setCurrentCycle(cycle);
         ++blockIterator;
    }
    //actSimStep = (simOffset+1)%64;
}

/*---------------------------------------------------------------------------------------------
 * (function: getConnectionBetween)
 *-------------------------------------------------------------------------------------------*/
Wire *Container::getConnectionBetween(QString nodeName, QString kidName)
{
    Wire* result = NULL;
    QHash<QString, Wire *> wires;
    QHash<QString, LogicUnit *> hash =unithashtable;
    QHash<QString, LogicUnit *>::const_iterator blockIterator = hash.constBegin();

     while(blockIterator != unithashtable.constEnd()){
         QString actName = blockIterator.key();
         if(actName.compare(nodeName)==0){
             LogicUnit* actUnit = blockIterator.value();
             wires = actUnit->getOutConsHash();
         }
         ++blockIterator;
     }

     if(wires.contains(kidName)){
        result = wires.value(kidName);
     }
     return result;
}

QList<LogicUnit *> Container::getClocks()
{
    return clocks;
}

QString Container::extractModuleFromName(QString name){
    if(!name.contains("+"))
        return QString("");

    QString result = name.left(name.lastIndexOf("+"));
    result = result.right(result.length()-name.lastIndexOf(".")-1);
    return result;
}

void Container::assignNodeToModule(QString nodeName,
                                   QString moduleName){
    LogicUnit* node = unithashtable[nodeName];
    LogicUnit* module = unithashtable[moduleName];

    node->addPartner(module);
    module->addPartner(node);
}

void Container::assignToModule(QString actName)
{
    //extract module
    QString moduleName = extractModuleFromName(actName);
    //check if module exist
    if(!unithashtable.contains(moduleName)){
        //create module
        addModule(moduleName);
    }
    //assign node to module
    assignNodeToModule(actName, moduleName);
}

void Container::expandCollapse(QString modulename)
{
    bool makeAllVisible;
    LogicUnit* module = unithashtable[modulename];

    makeAllVisible = module->isVisible();

    //toggle module visibility
    module->setVisible(!module->isVisible());

    QHash<QString, LogicUnit *> hash =unithashtable;
    QHash<QString, LogicUnit *>::const_iterator blockIterator = hash.constBegin();

     while(blockIterator != unithashtable.constEnd()){
         LogicUnit* actNode = blockIterator.value();
         if(actNode->hasModule &&
                 actNode->getModule()->getName().compare(modulename)==0){
            actNode->setVisible(makeAllVisible);
         }
         ++blockIterator;
     }
     arrangeContainer();


}



