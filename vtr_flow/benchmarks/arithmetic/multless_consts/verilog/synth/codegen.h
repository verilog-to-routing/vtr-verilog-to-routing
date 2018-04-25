struct op {
    /* add: res = r1 + r2
       sub: res = r1 - r2
       shl: res = r1 << sh
       shr: res = r1 >> sh
       cmul: res = c * r1
       addshl: res = r1 + (r2 << sh)
       subshl: res = r1 - (r2 << sh)
       shlsub: res = (r1 << sh) - r2
    */
    typedef enum { ADD=1, SUB=2, SHL=3, SHR=4, CMUL=5, NEG, ADDSHL, SUBSHL, SHLSUB, OPLAST } type_t;
  static const char* ID[]; // = { "+", "-", "<<", ">>", "*", "-", "+ <<", "- <<", "<< -" };
  static const char* DOTATTRS[];

    type_t type;
    coeff_t c;
    reg_t r1, r2, res;
    int sh;

    static op* add(reg_t res, reg_t r1, reg_t r2, coeff_t c=0) {
        return new op(ADD,  c, r1, r2, res,  0); }
    static op* sub(reg_t res, reg_t r1, reg_t r2, coeff_t c=0) {
        return new op(SUB,  c, r1, r2, res,  0); }
    static op* shl(reg_t res, reg_t r1, int sh, coeff_t c=0) {
        return new op(SHL,  c, r1,  0, res, sh); }
    static op* shr(reg_t res, reg_t r1, int sh, coeff_t c=0) {
        return new op(SHR,  c, r1,  0, res, sh); }
    static op* cmul(reg_t res, reg_t r1, coeff_t c) {
        return new op(CMUL, c, r1,  0, res,  0); }
    static op* neg(reg_t res, reg_t r1, coeff_t c=0) {
        return new op(NEG,  c, r1,  0, res,  0); }

    static op* addshl(reg_t res, reg_t r1, reg_t r2, int sh, coeff_t c=0) {
        return new op(ADDSHL, c, r1, r2, res, sh); }
    static op* subshl(reg_t res, reg_t r1, reg_t r2, int sh, coeff_t c=0) {
        return new op(SUBSHL, c, r1, r2, res, sh); }
    static op* shlsub(reg_t res, reg_t r1, reg_t r2, int sh, coeff_t c=0) {
        return new op(SHLSUB, c, r1, r2, res, sh); }

    void output(ostream &os) const;
    void output_gap(ostream &os) const;
    void output_dot(ostream &os) const;

private:
    op(type_t tt, coeff_t cc, reg_t rr1, reg_t rr2, reg_t rres, int ssh) :
      type(tt), c(cc), r1(rr1), r2(rr2), res(rres), sh(ssh) {}
};
