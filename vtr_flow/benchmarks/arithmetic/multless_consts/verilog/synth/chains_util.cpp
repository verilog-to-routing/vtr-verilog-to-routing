#include "chains.h"
#include <string>

void myabort() { abort(); }

/* returns number of CSD bits minus one.
   in other workds the CSD cost */
int csd_bits(coeff_t c) {
    c = (c << 1); /* append 0 to the right */
    int res = 0, mode = 0, newmode;
    while(c) {
        if(mode==0) {
            newmode = 0;
            if((c&3)==1) ++res;
            if((c&3)==3) { ++res; newmode=1; }
        }
        else { /* mode==1 */
            newmode = 1;
            if((c&3)==0) { ++res; newmode = 0; }
            if((c&3)==2) ++res;
        }
        mode = newmode;
        c = (c>>1);
    }
    if(mode==1) ++res;
    return res -1;
}

void gen_add_coeffs(cfset_t &src1, cfset_t &src2, cfset_t &dest) {
    FORALL(src1, s1)
        FORALL(src2, s2)
            compute_fast_sumdiffs(dest, *s1, *s2);
}

void gen_mul_coeffs(cfset_t &src1, cfset_t &src2, cfset_t &dest) {
    FORALL(src1, s1) {
        FORALL(src2, s2) {
            /* prevent multiplication overflow */
            if(clog2(*s1) + clog2(*s2) > clog2(MAX_NUM)-1 ) continue;
            else {
                coeff_t prod = (*s1) * (*s2);
                assert((prod >= *s1) && (prod >= *s2) && "an overflow has occurred");
                if(prod < MAX_NUM && prod > 0)
                    ADD(dest, prod);
            }
        }
    }
}

/* result of set_product is all possible outputs of an adder with
   inputs from 2 different sets */
void set_product(cfset_t &dest, cfset_t &src1, cfset_t &src2) {
    FORALL(src1, s1)
        FORALL(src2, s2)
            compute_fast_sumdiffs(dest, *s1, *s2);
}

void set_product(cfset_t &dest, full_addmap_t &src1, cfset_t &src2) {
    FORALL_FAM(src1, s1)
        FORALL(src2, s2)
            compute_fast_sumdiffs(dest, (*s1).first, *s2);
}
void set_product(cfset_t &dest, full_addmap_t &src1, full_addmap_t &src2) {
    FORALL_FAM(src1, s1)
        FORALL_FAM(src2, s2)
            compute_fast_sumdiffs(dest, (*s1).first, (*s2).first);
}
void set_product(cfset_t &dest, coeff_t c, cfset_t &src) {
    FORALL(src, s)
        compute_fast_sumdiffs(dest, *s, c);
}

void set_product(cfset_t &dest, coeff_t c, full_addmap_t &src) {
    FORALL_FAM(src, s)
        compute_fast_sumdiffs(dest, (*s).first, c);
}

void set_ratio(cfset_t &dest, coeff_t c, cfset_t &denom) {
    FORALL(denom, q)
      if ( c % (*q) == 0 ) ADD(dest, c/(*q));
}
void set_ratio(cfset_t &dest, cfset_t &nom, cfset_t &denom) {
    FORALL(nom, c)
        FORALL(denom, q)
            if ( (*c) % (*q) == 0 ) ADD(dest, (*c)/(*q));
}

/* dest = dest \ sub */
void subtract_set(cfset_t &dest, cfset_t &sub) {
    FORALL(sub, s) REMOVE(dest, *s);
}

void unite_set(cfset_t &dest, cfset_t &add) {
    FORALL(add, s) ADD(dest, *s);
}

void _compute_all_sumdiffs(bool is_left, addmap_t &result, coeff_t num1, coeff_t num2) {
    /* we store shift+1 to make possible differentiating +/- of unshifted number.
       so shift 0 is actually 1, and shift n is actually n+1 */
    int max_shift = MAX_SHIFT - clog2(num2);
    if(max_shift < 0) max_shift = 0;
    for(int shift = MIN_SHIFT; shift <= max_shift; ++shift) {
        coeff_t sum, diff;
        diff = num1 - (num2<<shift);
        if(diff != 0) {
            sum = num1 + (num2<<shift);
            if(sum < MAX_NUM) result[fundamental(sum)]   = SUM(is_left, shift);
            if(diff > 0 && diff < MAX_NUM)
                result[fundamental(diff)] = DIFF(is_left, shift);
            else if(diff < 0 && diff > -MAX_NUM)
                result[fundamental(-diff)] = RDIFF(is_left, shift);
        }
    }
}

void compute_all_sumdiffs(addmap_t &result, coeff_t num1, coeff_t num2) {
    _compute_all_sumdiffs(false, result, num1, num2);
    _compute_all_sumdiffs(true, result, num2, num1);
}

void _compute_fast_sumdiffs(cfset_t &result, coeff_t num1, coeff_t num2) {
    int max_shift = MAX_SHIFT - clog2(num2);
    if(max_shift < 0) max_shift = 0;
    for(int shift = MIN_SHIFT; shift <= max_shift; ++shift) {
        coeff_t sum, diff;
        diff = cfabs(num1 - (num2<<shift));
        if(diff != 0) {
            sum = num1 + (num2<<shift);
            if(sum < MAX_NUM) result.insert(fundamental(sum));
            if(diff < MAX_NUM) result.insert(fundamental(diff));
        }
    }
}

/* does not store shift amount pairs, so usable only for cost estimation
   [but not code generation] */
void compute_fast_sumdiffs(cfset_t &result, coeff_t num1, coeff_t num2) {
    _compute_fast_sumdiffs(result, num1, num2);
    _compute_fast_sumdiffs(result, num2, num1);
}


void op::output(ostream &os) const {
     os << "t" << res << " = ";
     switch(type) {
     case ADD: os << "add(t" << r1 << ", t" << r2 << ");"; break;
     case SUB: os << "sub(t" << r1 << ", t" << r2 << ");"; break;
     case SHL: if(sh!=0) os << "shl(t" << r1 << ", " << sh << ");";
               else os << "t" << r1 << ";"; break;
     case SHR: os << "shr(t" << r1 << ", " << sh << ");";  break;
     case CMUL: os << "cmul(t" << r1 << ", " << c << ");"; break;
     case NEG: os << "neg(t" << r1 << ");"; break;
     case ADDSHL: os << "addshl(t" << r1 << ", t" << r2 << ", " << sh << ");"; break;
     case SUBSHL: os << "subshl(t" << r1 << ", t" << r2 << ", " << sh << ");"; break;
     case SHLSUB: os << "shlsub(t" << r1 << ", t" << r2 << ", " << sh << ");"; break;
     default: ASSERT(0);
     }
     os << "   /* " << c << "*/";
     os << endl;
}

void op::output_gap(ostream &os) const {
    os << "  [" << res << ", ";
    switch(type) {
    case ADD: case SUB:
      os << "[" << type << ", " << r1 << ", " << r2 << "]"; break;
    case SHL: case SHR:
      if(sh!=0) os << "[" << type << ", " << r1 << ", " << sh << "]";
      else      os << r1; break;
    case CMUL:
      os << "[" << type << ", " << r1 << ", " << c << "]"; break;
    case NEG:
      os << "[" << type << ", " << r1 << "]"; break;
    case ADDSHL: case SUBSHL: case SHLSUB:
      os << "[" << type << ", " << r1 << ", " << r2 << ", " << sh << "]"; break;
    default:
      ASSERT(0);
    }
    os << " ], " << endl;
}

const char* op::ID[]       = { "undef", "+",      "-",      "<<",     ">>",     "*",      "-1",        "+ <<",   "- <<",   "<< -" };
const char* op::DOTATTRS[] = { "undef",
  "style=filled fontname=\"Arial\" shape=\"box\" fillcolor=\"#990000\" fontcolor=\"white\" fontsize=26 ",
  "style=filled fontname=\"Arial\" shape=\"box\" fillcolor=\"#990000\" fontcolor=\"white\" fontsize=26 ",
  "style=filled fontname=\"Arial\" shape=\"box\" fillcolor=\"#cccccc\" fontcolor=\"#444444\"",
  "style=filled fontname=\"Arial\" shape=\"box\" fillcolor=\"#cccccc\" fontcolor=\"#444444\"",
  "style=filled fontname=\"Arial\" shape=\"box\" fillcolor=\"#990000\" fontcolor=\"white\"",
  "style=filled fontname=\"Arial\" shape=\"box\" fillcolor=\"#444444\" fontcolor=\"white\"",
  "style=filled fontname=\"Arial\" shape=\"box\" fillcolor=\"#990000\" fontcolor=\"white\"",
  "style=filled fontname=\"Arial\" shape=\"box\" fillcolor=\"#990000\" fontcolor=\"white\"",
  "style=filled fontname=\"Arial\" shape=\"box\" fillcolor=\"#990000\" fontcolor=\"white\""
};


void op::output_dot(ostream &os) const {
    os << "t" << res << " [label=\"" << ID[type];
    if(sh!=0) os << sh;
    //os << "\\n \\N=" << c << "x"; /* 'variable = Cx'  node annotation */
    os << "\\n \\" << c << "x";  /* 'Cx'  node annotation */
    os << "\" " << DOTATTRS[type] << "]\n";

    switch(type) {
    case ADD: case SUB:
        if(!r1) os << "X"; else os << "t" << r1;  os << " -> t" << res << "\n";
        if(!r2) os << "X"; else os << "t" << r2;  os << " -> t" << res << "\n"; break;
    case SHL: case SHR:
        if(!r1) os << "X"; else os << "t" << r1;  os << " -> t" << res << "\n"; break;
    case CMUL:
        if(!r1) os << "X"; else os << "t" << r1;  os << " -> t" << res << "\n"; break;
    case NEG:
        if(!r1) os << "X"; else os << "t" << r1;  os << " -> t" << res << "\n"; break;
    case ADDSHL: case SUBSHL: case SHLSUB:
        if(!r1) os << "X"; else os << "t" << r1;  os << " -> t" << res << "\n";
        if(!r2) os << "X"; else os << "t" << r2;  os << " -> t" << res << "\n"; break;
    default:
      ASSERT(0);
    }
}

coeff_t chain::get_intermediates_sum(chain *(get_ch)(coeff_t)) {
    cfset_t interm;
    get_intermediates(interm, get_ch);
    coeff_t res = 0;
    FORALL(interm, i) res += (*i);
    return res;
}

mult_chain::mult_chain(cost_t cost, coeff_t c1, coeff_t c2) {
    this->cost = cost;
    this->c1 = c1;
    this->c2 = c2;
    c = c1*c2;
}

void mult_chain::output(ostream &os) const {
    os << c << " " << cost;
    os << " mult " << c1 << " " << c2 << endl;
}

void mult_chain::output_c(ostream &os) const {
    os << "new mult_chain(" << cost << ", " << c1 << ", " << c2 << ")";
}

void mult_chain::get_ops(oplist_t &l, reg_t dest, reg_t src, reg_t (tmpreg)()) {
    reg_t t1 = tmpreg();
    l.push_back( op::cmul(t1, src, c1) );
    l.push_back( op::cmul(dest, t1, c2) );
}


void mult_chain::get_intermediates(cfset_t &dest, chain *(get_ch)(coeff_t)) {
    cfset_t tmp;
    get_ch(c1)->get_intermediates(dest, get_ch);
    get_ch(c2)->get_intermediates(tmp, get_ch);
    FORALL(tmp, i) {
        ADD(dest, (*i) * c1);
    }
}

void mult_chain::compute_depth(chain *(get_ch)(coeff_t)) {
    _depth = get_ch(c1)->depth(get_ch) +  get_ch(c2)->depth(get_ch);
}

add_chain::add_chain(cost_t cost, coeff_t c1, coeff_t c2, spref_t sp) {
    this->cost = cost;
    this->c1 = c1;
    this->c2 = c2;
    this->sp = sp;
    c = compute_sp(c1, c2, sp);
}

void add_chain::compute_depth(chain *(get_ch)(coeff_t)) {
    _depth = 1 + max(get_ch(c1)->depth(get_ch), get_ch(c2)->depth(get_ch));
}

void add_chain::output(ostream &os) const {
    os << c << " " << cost;
    os << " add " << c1 << " " << c2 << " " << sp.first
       << " " << sp.second << endl;
}

void add_chain::output_c(ostream &os) const {
    os << "new add_chain(" << cost << ", " << c1 << ", " << c2 << ", "
       << "sp_t(" << sp.first << ", " << sp.second << "))";
}

void add_chain::get_ops(oplist_t &l, reg_t dest, reg_t src, reg_t (tmpreg)()) {
    reg_t t1=tmpreg(), t2=tmpreg();
    if(c1!=1) l.push_back(op::cmul(t1, src, c1)); else t1 = src;
    if(c2!=1) l.push_back(op::cmul(t2, src, c2)); else t2 = src;
    sumdiff_ops(l, dest, t1, t2, tmpreg, c1, c2, sp);
}

void add_chain::get_intermediates(cfset_t &dest, chain *(get_ch)(coeff_t)) {
    get_ch(c1)->get_intermediates(dest, get_ch);
    get_ch(c2)->get_intermediates(dest, get_ch);
    ADD(dest, c);
}

leapfrog2_chain::leapfrog2_chain(cost_t cost, coeff_t c1, coeff_t c2,
                                 spref_t sp1, spref_t sp2) {
    this->cost = cost;
    this->c1 = c1; this->sp1 = sp1;
    this->c2 = c2; this->sp2 = sp2;
    leap1 = compute_sp(c1, 1, sp1);
    c = compute_sp(leap1*c2, c1, sp2);
}

void leapfrog2_chain::compute_depth(chain *(get_ch)(coeff_t)) {
    _depth = 2 + get_ch(c1)->depth(get_ch) + get_ch(c2)->depth(get_ch);
}

void leapfrog2_chain::get_ops(oplist_t &l, reg_t dest, reg_t src, reg_t (tmpreg)()) {
    reg_t leap1reg = tmpreg(), t1 = tmpreg(), t2 = tmpreg();
    l.push_back( op::cmul(t1, src, c1) );
    sumdiff_ops(l, leap1reg, t1, src, tmpreg, c1, 1, sp1);
    l.push_back( op::cmul(t2, leap1reg, c2) );
    sumdiff_ops(l, dest, t2, t1, tmpreg, c2*leap1, c1, sp2);
}

void leapfrog2_chain::get_intermediates(cfset_t &dest, chain *(get_ch)(coeff_t)) {
    cfset_t tmp;
    get_ch(c1)->get_intermediates(dest, get_ch);
    get_ch(c2)->get_intermediates(tmp, get_ch);
    ADD(dest, leap1);
    FORALL(tmp, i) {
        ADD(dest, (*i) * leap1);
    }
    ADD(dest, c);
}

void leapfrog2_chain::output(ostream &os) const {
    os << c << " " << cost;
    os << " lpfr2 " << c1 << " " << c2 << " "
       << sp1.first << " " << sp1.second << " "
       << sp2.first << " " << sp2.second << " "
       << endl;
}

void leapfrog2_chain::output_c(ostream &os) const {
    os << "new leapfrog2_chain(" << cost << ", " << c1 << ", " << c2 << ", "
       << "sp_t(" << sp1.first << ", " << sp1.second << "), "
       << "sp_t(" << sp2.first << ", " << sp2.second << "))";
}


leapfrog3_chain::leapfrog3_chain(cost_t cost, coeff_t c1, coeff_t c2,
                                 spref_t sp1, spref_t sp2, spref_t sp3) {
    this->cost = cost;
    this->c1 = c1;   this->c2 = c2;
    this->sp1 = sp1; this->sp2 = sp2; this->sp3 = sp3;
    leap1 = compute_sp(c1, 1, sp1);
    leap2 = compute_sp(leap1, c1, sp2);
    c = compute_sp(leap2*c2, leap1, sp3);
}

void leapfrog3_chain::get_ops(oplist_t &l, reg_t dest, reg_t src, reg_t (tmpreg)()) {
    reg_t leap1reg = tmpreg(), leap2reg = tmpreg(), t1 = tmpreg(), t2 = tmpreg();
    l.push_back( op::cmul(t1, src, c1) );
    sumdiff_ops(l, leap1reg, t1,       src, tmpreg, c1,    1, sp1);
    sumdiff_ops(l, leap2reg, leap1reg, t1,  tmpreg, leap1, c1, sp2);
    l.push_back( op::cmul(t2, leap2reg, c2) );
    sumdiff_ops(l, dest, t2, leap1reg, tmpreg, c2*leap2, leap1, sp3);
}

void leapfrog3_chain::get_intermediates(cfset_t &dest, chain *(get_ch)(coeff_t)) {
    cfset_t tmp;
    get_ch(c1)->get_intermediates(dest, get_ch);
    get_ch(c2)->get_intermediates(tmp, get_ch);
    ADD(dest, leap1);
    ADD(dest, leap2);
    FORALL(tmp, i) {
        ADD(dest, (*i) * leap2);
    }
    ADD(dest, c);
}

void leapfrog3_chain::compute_depth(chain *(get_ch)(coeff_t)) {
    _depth = 3 + get_ch(c1)->depth(get_ch) + get_ch(c2)->depth(get_ch);
}

void leapfrog3_chain::output(ostream &os) const {
    os << c << " " << cost;
    os << " lpfr3 " << c1 << " " << c2 << " "
       << sp1.first << " " << sp1.second << " "
       << sp2.first << " " << sp2.second << " "
       << sp3.first << " " << sp3.second << " "
       << endl;
}

void leapfrog3_chain::output_c(ostream &os) const {
    os << "new leapfrog3_chain(" << cost << ", " << c1 << ", " << c2 << ", "
       << "sp_t(" << sp1.first << ", " << sp1.second << "), "
       << "sp_t(" << sp2.first << ", " << sp2.second << "), "
       << "sp_t(" << sp3.first << ", " << sp3.second << "))";
}


void sumdiff_ops(oplist_t &l, reg_t dest, reg_t src1, reg_t src2, reg_t (tmpreg)(),
                 coeff_t c1, coeff_t c2, spref_t sp) {
    int sh1, sh2, sign=+1, orient;
    /* shifts are offset by 1, see compute_all_sumdiffs in chains.cpp */
    if(sp.first < 0) { sh1 = -sp.first-1; sign = -1; orient = 1; }
    else             { sh1 =  sp.first-1;  }

    if(sp.second < 0) { sh2 = -sp.second-1; sign = -1; orient = 2; }
    else              { sh2 =  sp.second-1; }

    if(sh1 == sh2) {
        /* shifts are allowed only on one side, so sh1==sh2 means both are 0 */
        ASSERT(sh1==0);
        ASSERT(sign==+1 || c1!=c2);
        reg_t t1=tmpreg();
        coeff_t c = (sign==+1) ? (c1+c2) : cfabs(c1-c2);
        if(sign==+1)            l.push_back( op::add(t1, src1, src2, c) );
        else if(orient==1)      l.push_back( op::sub(t1, src2, src1, c) );
        else                    l.push_back( op::sub(t1, src1, src2, c) );
        l.push_back( op::shr(dest, t1, compute_shr(c), c >> (compute_shr(c))) );
    }
    else {
        ASSERT(sh1==0 || sh2==0);
        reg_t t1=tmpreg();

        if(sh1==0) {
            l.push_back( op::shl(t1, src2, sh2, c2<<sh2) );
            if(sign==+1)        l.push_back( op::add(dest, src1, t1, c1+(c2<<sh2)) );
            else if(orient==1)  l.push_back( op::sub(dest, t1, src1, (c2<<sh2)-c1) );
            else                l.push_back( op::sub(dest, src1, t1, c1-(c2<<sh2)) );
        }
        else { /* sh2==0 */
            l.push_back( op::shl(t1, src1, sh1, c1<<sh1) );
            if(sign==+1)        l.push_back( op::add(dest, src2, t1, c2+(c1<<sh1)) );
            else if(orient==1)  l.push_back( op::sub(dest, src2, t1, c2-(c1<<sh1)) );
            else                l.push_back( op::sub(dest, t1, src2, (c1<<sh1)-c2) );
        }
    }
}

chain * check_chain(coeff_t c, chain *ch) {
    if(c != ch->c) {
        cerr << "Error: reading coefficient " << c << ", chain produces "
             << ch->c << " instead" << endl;
        ASSERT(0);
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
        ASSERT(0);
    }

}
