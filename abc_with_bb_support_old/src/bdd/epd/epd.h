/**CHeaderFile*****************************************************************

  FileName    [epd.h]

  PackageName [epd]

  Synopsis    [The University of Colorado extended double precision package.]

  Description [arithmetic functions with extended double precision.]

  SeeAlso     []

  Author      [In-Ho Moon]

  Copyright [This file was created at the University of Colorado at
  Boulder.  The University of Colorado at Boulder makes no warranty
  about the suitability of this software for any purpose.  It is
  presented on an AS IS basis.]

  Revision    [$Id: epd.h,v 1.1.1.1 2003/02/24 22:23:57 wjiang Exp $]

******************************************************************************/

#ifndef _EPD
#define _EPD


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define	EPD_MAX_BIN	1023
#define	EPD_MAX_DEC	308
#define	EPD_EXP_INF	0x7ff

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

/**Struct**********************************************************************

  Synopsis    [IEEE double struct.]

  Description [IEEE double struct.]

  SeeAlso     []

******************************************************************************/
#ifdef	EPD_BIG_ENDIAN
struct IeeeDoubleStruct {	/* BIG_ENDIAN */
  unsigned int sign: 1;
  unsigned int exponent: 11;
  unsigned int mantissa0: 20;
  unsigned int mantissa1: 32;
};
#else
struct IeeeDoubleStruct {	/* LITTLE_ENDIAN */
  unsigned int mantissa1: 32;
  unsigned int mantissa0: 20;
  unsigned int exponent: 11;
  unsigned int sign: 1;
};
#endif

/**Struct**********************************************************************

  Synopsis    [IEEE double NaN struct.]

  Description [IEEE double NaN struct.]

  SeeAlso     []

******************************************************************************/
#ifdef	EPD_BIG_ENDIAN
struct IeeeNanStruct {	/* BIG_ENDIAN */
  unsigned int sign: 1;
  unsigned int exponent: 11;
  unsigned int quiet_bit: 1;
  unsigned int mantissa0: 19;
  unsigned int mantissa1: 32;
};
#else
struct IeeeNanStruct {	/* LITTLE_ENDIAN */
  unsigned int mantissa1: 32;
  unsigned int mantissa0: 19;
  unsigned int quiet_bit: 1;
  unsigned int exponent: 11;
  unsigned int sign: 1;
};
#endif

/**Struct**********************************************************************

  Synopsis    [Extended precision double to keep very large value.]

  Description [Extended precision double to keep very large value.]

  SeeAlso     []

******************************************************************************/
struct EpDoubleStruct {
  union {
    double			value;
    struct IeeeDoubleStruct	bits;
    struct IeeeNanStruct	nan;
  } type;
  int		exponent;
};

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/
typedef struct EpDoubleStruct EpDouble;
typedef struct IeeeDoubleStruct IeeeDouble;
typedef struct IeeeNanStruct IeeeNan;


/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

EpDouble *EpdAlloc();
int EpdCmp(const char *key1, const char *key2);
void EpdFree(EpDouble *epd);
void EpdGetString(EpDouble *epd, char *str);
void EpdConvert(double value, EpDouble *epd);
void EpdMultiply(EpDouble *epd1, double value);
void EpdMultiply2(EpDouble *epd1, EpDouble *epd2);
void EpdMultiply2Decimal(EpDouble *epd1, EpDouble *epd2);
void EpdMultiply3(EpDouble *epd1, EpDouble *epd2, EpDouble *epd3);
void EpdMultiply3Decimal(EpDouble *epd1, EpDouble *epd2, EpDouble *epd3);
void EpdDivide(EpDouble *epd1, double value);
void EpdDivide2(EpDouble *epd1, EpDouble *epd2);
void EpdDivide3(EpDouble *epd1, EpDouble *epd2, EpDouble *epd3);
void EpdAdd(EpDouble *epd1, double value);
void EpdAdd2(EpDouble *epd1, EpDouble *epd2);
void EpdAdd3(EpDouble *epd1, EpDouble *epd2, EpDouble *epd3);
void EpdSubtract(EpDouble *epd1, double value);
void EpdSubtract2(EpDouble *epd1, EpDouble *epd2);
void EpdSubtract3(EpDouble *epd1, EpDouble *epd2, EpDouble *epd3);
void EpdPow2(int n, EpDouble *epd);
void EpdPow2Decimal(int n, EpDouble *epd);
void EpdNormalize(EpDouble *epd);
void EpdNormalizeDecimal(EpDouble *epd);
void EpdGetValueAndDecimalExponent(EpDouble *epd, double *value, int *exponent);
int EpdGetExponent(double value);
int EpdGetExponentDecimal(double value);
void EpdMakeInf(EpDouble *epd, int sign);
void EpdMakeZero(EpDouble *epd, int sign);
void EpdMakeNan(EpDouble *epd);
void EpdCopy(EpDouble *from, EpDouble *to);
int EpdIsInf(EpDouble *epd);
int EpdIsZero(EpDouble *epd);
int EpdIsNan(EpDouble *epd);
int EpdIsNanOrInf(EpDouble *epd);
int IsInfDouble(double value);
int IsNanDouble(double value);
int IsNanOrInfDouble(double value);

#endif /* _EPD */
