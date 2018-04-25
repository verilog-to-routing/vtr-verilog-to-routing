/*
 * Copyright (c) 2006 Peter Tummeltshammer for the Spiral project (www.spiral.net)
 * Copyright (c) 2006 Carnegie Mellon University
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "chains.h"
#include "table.h"
#include <string>
#include <map>
#include <fstream>

typedef map<coeff_t, chain*> chainmap_t;
typedef chainmap_t::const_iterator chainiter_t ;
chainmap_t * CHAINS;

void   init_table(const char *fname) {
    CHAINS = new chainmap_t;
    ifstream fin(fname);
    assert(fin);

    chain * ch = read_chain(fin);
    while(fin && ch!=NULL) {
    (*CHAINS)[ch->c] = ch;
    ch = read_chain(fin);
    }
}

chain *get_chain_table(coeff_t c) {
    chainiter_t i = CHAINS->find(c);
    if(i==CHAINS->end())
    return NULL;
    else return (*i).second;
}

void   delete_table() {
    delete CHAINS;
}


chain * check_chain(coeff_t c, chain *ch) {
    if(c != ch->c) {
    cerr << "Error: reading coefficient " << c << ", chain produces "
         << ch->c << " instead" << endl;
    assert(0);
    }
    else return ch;
}

chain * read_chain(istream &fin) {
    coeff_t c;
    int cost;
    std::string type;

    fin >> c >> cost >> type;

    if(!fin) return NULL;
    if(type == "add") {
    coeff_t c1, c2;
    int sp11, sp12;
    fin >> c1 >> c2 >> sp11 >> sp12;
    return check_chain(c, new add_chain(cost, c1, c2, sp_t(sp11, sp12)));
    }
    else if(type == "mult") {
    coeff_t c1, c2;
    fin >> c1 >> c2;
    return check_chain(c, new mult_chain(cost, c1, c2));
    }
    else if(type == "lpfr2") {
    coeff_t c1, c2;
    int sp11, sp12, sp21, sp22;
    fin >> c1 >> c2 >> sp11 >> sp12 >> sp21 >> sp22;
    return check_chain(c, new leapfrog2_chain(cost, c1, c2,
                   sp_t(sp11, sp12), sp_t(sp21, sp22)));
    }
    else if(type == "lpfr3") {
    coeff_t c1, c2;
    int sp11, sp12, sp21, sp22, sp31, sp32;
    fin >> c1 >> c2 >> sp11 >> sp12 >> sp21 >> sp22 >> sp31 >> sp32;
    return check_chain(c, new leapfrog3_chain(cost, c1, c2,
                  sp_t(sp11, sp12), sp_t(sp21, sp22),sp_t(sp31, sp32)));
    }
    else {
    cerr << "'" << type << "' ";
    cerr << "is not a valid chain type (allowed = add, mult, lpfr2, lpfr3)" << endl;
    assert(0);
    }

}
