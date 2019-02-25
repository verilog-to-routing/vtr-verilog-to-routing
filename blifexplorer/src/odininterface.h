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
#ifndef ODININTERFACE_H
#define ODININTERFACE_H

#include <QtWidgets>

#include <memory>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sstream>

#include "odin_globals.h"
#include "odin_types.h"

#include "simulate_blif.h"
#include "odin_ii.h"




class OdinInterface
{
public:
    OdinInterface();
    int startOdin();
    QHash<QString, nnode_t *> getNodeTable();
    void setFilename(QString filename);
    void setUpSimulation();
    int simulateNextWave();
    void endSimulation();
    int getOutputValue(nnode_t* node, int pin, int actstep);
    netlist_t *blifexplorer_netlist;

private:
    QHash<QString, nnode_t *> nodehash;
    QQueue<nnode_t *> nodequeue;
    QString myFilename;
    sim_data_t *sim_data;
    char **arg_list;
    int arg_len;
    int wave;
};

#endif // ODININTERFACE_H
