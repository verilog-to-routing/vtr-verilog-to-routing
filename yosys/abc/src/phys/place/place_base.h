/*===================================================================*/
//  
//     place_base.h
//
//        Aaron P. Hurst, 2003-2007
//              ahurst@eecs.berkeley.edu
//
/*===================================================================*/

#if !defined(PLACE_BASE_H_)
#define ABC__phys__place__place_base_h


ABC_NAMESPACE_HEADER_START


// --------------------------------------------------------------------
// Data structures
//
// --------------------------------------------------------------------

// --- a C++ bool-like type
//typedef char bool;
#ifndef ABC__phys__place__place_base_h
#define bool int
#endif

#define true 1
#define false 0


// --- Rect - rectangle

typedef struct Rect {
  float x, y;
  float w, h;
} Rect;


// --- AbstractCell - a definition of a cell type

typedef struct AbstractCell {
  char *m_label;            // string description

  float m_width, m_height;  // dimensions

  bool  m_pad;              // a pad (external I/O) cell?
} AbstractCell;


// --- ConcreteCell - a design object

typedef struct ConcreteCell {
  int           m_id;       // a unique ID (see below)
  char         *m_label;    // string description

  AbstractCell *m_parent;   // cell type

  bool          m_fixed;    // position is fixed?
  float         m_x, m_y;   // center of cell

  int           m_data;
} ConcreteCell;


// --- ConcreteNet - a design net

typedef struct ConcreteNet {
  int            m_id;       // a unique ID (see below)

  int            m_numTerms; // num. of connected cells
  ConcreteCell **m_terms;    // connected cells

  float          m_weight;   // relative weight

  int            m_data;
} ConcreteNet;


// A note about IDs - the IDs are non-nonegative integers. They need not
// be contiguous, but this is certainly a good idea, as they are stored
// in a non-sparse array.
// Cells and nets have separate ID spaces.

// --------------------------------------------------------------------
// Global variable prototypes
//
// --------------------------------------------------------------------

// NOTE: None of these need to be managed externally.

extern int   g_place_numCells;   // number of cells
extern int   g_place_numNets;    // number of nets
extern float g_place_rowHeight;  // height of placement row
extern Rect  g_place_coreBounds; // border of placeable area
                                 // (x,y) = corner
extern Rect  g_place_padBounds;  // border of total die area
                                 // (x,y) = corner

extern ConcreteCell **g_place_concreteCells; // all concrete cells
extern ConcreteNet  **g_place_concreteNets;  // all concrete nets


// --------------------------------------------------------------------
// Function prototypes
//
// --------------------------------------------------------------------

void   addConcreteNet(ConcreteNet *net);
void   addConcreteCell(ConcreteCell *cell);
void   delConcreteNet(ConcreteNet *net);
void   delConcreteCell(ConcreteCell *cell);

void   globalPreplace(float utilization);
void   globalPlace();
void   globalIncremental();
void   globalFixDensity(int numBins, float maxMovement);

float fastEstimate(ConcreteCell *cell,
                   int numNets, ConcreteNet *nets[]);
float fastTopoPlace(int numCells, ConcreteCell *cells[], 
                    int numNets, ConcreteNet *nets[]);

Rect   getNetBBox(const ConcreteNet *net);
float  getNetWirelength(const ConcreteNet *net);
float  getTotalWirelength();
float  getCellArea(const ConcreteCell *cell);

void   writeBookshelf(const char *filename);

// comparative qsort-style functions
int    netSortByL(const void *a, const void *b);
int    netSortByR(const void *a, const void *b);
int    netSortByB(const void *a, const void *b);
int    netSortByT(const void *a, const void *b);
int    netSortByID(const void *a, const void *b);
int    cellSortByX(const void *a, const void *b);
int    cellSortByY(const void *a, const void *b);
int    cellSortByID(const void *a, const void *b);



ABC_NAMESPACE_HEADER_END

#endif
