/*****************************************************************************************\
*                                                                                         *
*  Array Abstraction                                                                      *
*                                                                                         *
*  Author:  Gene Myers                                                                    *
*  Date  :  October 2009                                                                  *
*                                                                                         *
*  (c) July 27, '09, Dr. Gene Myers and Howard Hughes Medical Institute                   *
*      Copyrighted as per the full copy in the associated 'README' file                   *
*                                                                                         *
\*****************************************************************************************/

#ifndef _ARRAY_LIB

#define _ARRAY_LIB

#include "mylib.h"

/****************************************************************************************
 *                                                                                      *
 * ARRAY DECLARATIONS                                                                   *
 *                                                                                      *
 ****************************************************************************************/

namespace mylib {
typedef enum		//  Some arrays may encode RGB or RGBA data (see below):
  { PLAIN_KIND    = 0,	//    The outermost dimension encodes each channel (R, G, B, or A)
    RGB_KIND      = 1,	//  Others may encode COMPLEX data (see below):
    RGBA_KIND     = 2,  //    The innermost dimension encodes the real and imaginary parts.
    COMPLEX_KIND  = 3,
    UNNAMED_ARRAY_KIND,
    UNNAMED_ARRAY_KIND2
  } Array_Kind;

typedef enum		//  Interpretation of the outermost dimension index if RGB or RGBA
  { RED_INDEX   = 0,
    GREEN_INDEX = 1,
    BLUE_INDEX  = 2,
    ALPHA_INDEX = 3
  } Color_Index;

typedef enum		//  Interpretation of the innermost dimension index if COMPLEX
  { REAL_INDEX = 0,
    IMAG_INDEX = 1,
  } Complex_Index;

typedef struct
  { Array_Kind  kind;    //  Interpreation of the array: one of the four enum constants above
    Value_Type  type;    //  Type of values, one of the eight enum constants above
    int         scale;   //  # of bits in integer values

    int         ndims;   //  Number of dimensions of the array
    Size_Type   size;    //  Total number of elements in the array (= PROD_i dims[i])
    Dimn_Type  *dims;    //  dims[i] = length of dimension i

    int         tlen;    //  Length of the text string text
    string      text;    //  An arbitrary string label

    void       *data;    //  A block of size sizeof(type) * size bytes holding the array.
  } Array;

typedef Array  Matrix;  		//  Descriptive declarations for 2D and 1D arrays
typedef Array  Vector;
typedef Vector Coordinate;	        //  A coordinate into an array's integer lattice
                                        //    The type is Dimn_Type

typedef Array  Pixel_Array; 	  	//  Designates UINT8_TYPE or UINT16_TYPE
typedef Matrix Pixel_Matrix;
typedef Vector Pixel_Vector;

typedef Array  Integer_Array;    	//  Designates INT32
typedef Matrix Integer_Matrix;
typedef Vector Integer_Vector;

typedef Array  Float_Array;    		//  Designates FLOAT32
typedef Matrix Float_Matrix;
typedef Vector Float_Vector;

typedef Array  Double_Array;   		//  Designates FLOAT64
typedef Matrix Double_Matrix;
typedef Vector Double_Vector;

typedef Array  Numeric_Array;  		//  Designates FLOAT32 or FLOAT64
typedef Matrix Numeric_Matrix;
typedef Vector Numeric_Vector;

typedef Array  Complex_Array;   	//  Designates FLOAT32 or FLOAT64,
typedef Matrix Complex_Matrix;          //     PLAIN or COMPLEX array
typedef Vector Complex_Vector;

Array *_G(Copy_Array)(Array *array);	//  Per-convention object primitives
Array *Pack_Array(Array *_R(_M(array)));
Array *Inc_Array(Array *_R(_I(array)));
void   Free_Array(Array *_F(array));
void   Kill_Array(Array *_K(array));
void   Reset_Array();
int    Array_Usage();
void   Array_List(void (*handler)(Array *));
int    Array_Refcount(Array *array);
Array *_G(Read_Array)(FILE *input);
void   Write_Array(Array *array, FILE *output);

#define AUINT8(a)    ((mylib::uint8    *) (a)->data)  //  Coerce array data to its type
#define AUINT16(a)   ((mylib::uint16   *) (a)->data)
#define AUINT32(a)   ((mylib::uint32   *) (a)->data)
#define AUINT64(a)   ((mylib::uint64   *) (a)->data)
#define AINT8(a)     ((mylib::int8     *) (a)->data)
#define AINT16(a)    ((mylib::int16    *) (a)->data)
#define AINT32(a)    ((mylib::int32    *) (a)->data)
#define AINT64(a)    ((mylib::int64    *) (a)->data)
#define AFLOAT32(a)  ((mylib::float32  *) (a)->data)
#define AFLOAT64(a)  ((mylib::float64  *) (a)->data)

#define ADIMN(a)     ((mylib::Dimn_Type *) (a)->data)
#define AINDX(a)     ((mylib::Indx_Type *) (a)->data)
#define ASIZE(a)     ((mylib::Size_Type *) (a)->data)
#define AOFFS(a)     ((mylib::Offs_Type *) (a)->data)

//  Generic bundle for holding extents, either real or integral

typedef struct
  { Vector   *min;
    Vector   *max;
  } Extent_Bundle;


/****************************************************************************************
 *                                                                                      *
 * ARRAY CREATION AND ACCESS                                                            *
 *                                                                                      *
 ****************************************************************************************/

Array *_G(Make_Array)(Array_Kind kind, Value_Type type, int ndims, Dimn_Type *dims);

Array *_G(Make_Array_With_Shape)(Array_Kind kind, Value_Type type, Coordinate *_F(shape));

Array *_G(Make_Array_Of_Data)(Array_Kind kind, Value_Type type, int ndims, Dimn_Type *dims,
                             void *data);

Array *_G(Make_Array_From_Arrays)(Array_Kind kind, int n, Array **arrays);

void Set_Array_Text(Array *_M(array), string text);
void Append_To_Array_Text(Array *_M(array), string text);

Value Get_Array_Value(Array *array, Coordinate *_F(coord));
void  Set_Array_Value(Array *_M(array), Coordinate *_F(coord), Value v);

typedef Array Array_Bundle;

Array_Bundle *Get_Array_Plane(Array_Bundle *_R(_M(a_bundle)), Dimn_Type plane);


/****************************************************************************************
 *                                                                                      *
 * COORDINATE ROUTINES                                                                  *
 *                                                                                      *
 ****************************************************************************************/

Coordinate *_G(Coord)(string list);
void        Print_Coord(FILE *file, Coordinate *_F(coord));

Coordinate *_G(Coord1)(Dimn_Type d1);
Coordinate *_G(Coord2)(Dimn_Type d2, Dimn_Type d1);
Coordinate *_G(Coord3)(Dimn_Type d3, Dimn_Type d2, Dimn_Type d1);
Coordinate *_G(Coord4)(Dimn_Type d4, Dimn_Type d3, Dimn_Type d2, Dimn_Type d1);

Coordinate *AppendCoord(Dimn_Type d, Coordinate *_R(_M(coord)));
Coordinate *PrependCoord(Coordinate *_R(_M(coord)), Dimn_Type d);

Coordinate *_G(Idx2CoordA)(Array *array, Indx_Type idx);
Coordinate *_G(Idx2CoreA)(Array *array, Indx_Type idx);
Indx_Type   Coord2IdxA(Array *array, Coordinate *_F(coord));

void        Set_Coord_Basis(Coordinate *_F(shape), Array_Kind kind);
void        Use_Array_Basis(Array *array);
Coordinate *_G(Get_Coord_Basis)(Array_Kind *_O(kind));

Coordinate *_G(Idx2Coord)(Indx_Type idx);
Coordinate *_G(Idx2Core)(Indx_Type idx);
Indx_Type   Coord2Idx(Coordinate *_F(coord));

Coordinate *_G(Floor_Coord)(Double_Vector *point);
Coordinate *_G(Ceiling_Coord)(Double_Vector *point);
Coordinate *_G(Nearest_Coord)(Double_Vector *point);


/****************************************************************************************
 *                                                                                      *
 * ARRAY SLICES                                                                         *
 *                                                                                      *
 ****************************************************************************************/

typedef void Slice;

Slice      *_G(Copy_Slice)(Slice *slice);	//  Per-convention object primitives
Slice      *Pack_Slice(Slice *_R(_M(slice)));
Slice      *Inc_Slice(Slice *_R(_I(slice)));
void        Free_Slice(Slice *_F(slice));
void        Kill_Slice(Slice *_K(slice));
void        Reset_Slice();
int         Slice_Usage();
void        Slice_List(void (*handler)(Slice *));
int         Slice_Refcount(Slice *slice);

Slice      *_G(Make_Slice)(Array *_I(target), Coordinate *_S(beg), Coordinate *_S(end));

Coordinate *Slice_First(Slice *slice);
Coordinate *Slice_Last(Slice *slice);
Indx_Type   Slice_Index(Slice *slice);
Coordinate *Slice_Coordinate(Slice *slice);

boolean     Set_Slice_To_Index(Slice *_M(slice), Indx_Type idx);
boolean     Inc_Slice_Index(Slice *_M(slice));
boolean     Dec_Slice_Index(Slice *_M(slice));
boolean     Inside_Slice(Slice *slice);

Indx_Type   Set_Slice_To_First(Slice *_M(slice));
Indx_Type   Set_Slice_To_Last(Slice *_M(slice));
Indx_Type   Next_Slice_Index(Slice *_M(slice));
Indx_Type   Prev_Slice_Index(Slice *_M(slice));

Array      *_G(Make_Array_From_Slice)(Slice *slice);


/****************************************************************************************
 *                                                                                      *
 * ARRAY FRAMES                                                                         *
 *                                                                                      *
 ****************************************************************************************/

typedef void Frame;

Frame      *_G(Copy_Frame)(Frame *frame);	//  Per-convention object primitives
Frame      *Pack_Frame(Frame *_R(_M(frame)));
Frame      *Inc_Frame(Frame *_R(_I(frame)));
void        Free_Frame(Frame *_F(frame));
void        Kill_Frame(Frame *_K(frame));
void        Reset_Frame();
int         Frame_Usage();
void        Frame_List(void (*handler)(Frame *));
int         Frame_Refcount(Frame *frame);

Frame      *_G(Make_Frame)(Array *_I(target), Coordinate *_S(shape), Coordinate *_S(anchor));

Coordinate *Frame_Shape(Frame *frame);
Coordinate *Frame_Anchor(Frame *frame);
Indx_Type   Frame_Index(Frame *frame);
Coordinate *Frame_Coordinate(Frame *frame);

boolean     Place_Frame(Frame *_M(frame), Indx_Type p);
boolean     Move_Frame_Forward(Frame *_M(frame));
boolean     Move_Frame_Backward(Frame *_M(frame));
boolean     Frame_Within_Array(Frame *_M(frame));

void       *Frame_Values(Frame *frame);
Offs_Type  *Frame_Offsets(Frame *frame);

Array        *_G(Make_Array_From_Frame)(Frame *frame);
Array_Bundle *Frame_Array(Array_Bundle *_R(_O(bundle)), Frame *frame);


/****************************************************************************************
 *                                                                                      *
 * ARRAY FORMS                                                                          *
 *                                                                                      *
 ****************************************************************************************/

typedef enum		//  Enum constants for the 3 classes of AForms
  { ARRAY_CLASS  = 0,
    SLICE_CLASS  = 1,
    FRAME_CLASS  = 2,
  } Form_Class;

typedef void   AForm;               //  An Array, Slice, or Frame
typedef void   APart;               //  An Array or Slice, but not a Frame
typedef APart  Pixel_APart;         //  A UINT8_TYPE or UINT16_TYPE apart

Size_Type   AForm_Size(AForm *form);
Array      *AForm_Array(AForm *form);
Array_Kind  AForm_Kind(AForm *form);
Coordinate *_G(AForm_Shape)(AForm *form);

boolean     Same_Shape(AForm *a, AForm *b);     // Do a and b have the same shape?
boolean     Same_Type(AForm *a, AForm *b);      // Do a and b have the same shape and type?

Form_Class  AForm_Class(AForm *form);
boolean     Is_Slice(AForm *form);
boolean     Is_Frame(AForm *form);
boolean     Is_Array(AForm *form);

Array      *_G(Make_Array_From_AForm)(AForm *form);

AForm      *_G(Copy_AForm)(AForm *form);    //  Object-specific versions of the class primitives
AForm      *Pack_AForm(AForm *_R(_M(form)));
AForm      *Inc_AForm(AForm *_R(_I(form)));
void        Free_AForm(AForm *_F(form));
void        Kill_AForm(AForm *_K(form));
void        Reset_AForm();
int         AForm_Usage();
void        AForm_List(void (*handler)(AForm *));
int         AForm_Refcount(AForm *form);


/****************************************************************************************
 *                                                                                      *
 * GENERAL ARRAY OPERATORS                                                              *
 *                                                                                      *
 ****************************************************************************************/

void   Print_Array(AForm *a, FILE *output, int indent, string format);
void   Print_Inuse_List(FILE *output, int indent);

void printArrayInfo(AForm *o, FILE *output = stdout);

typedef struct      //  A return bundle (not an object)
  { Value maxval;
    Value minval;
  } Range_Bundle;

Range_Bundle *Array_Range(Range_Bundle *_R(_O(range)), AForm *array);
APart        *Scale_Array(APart *_R(_M(array)), double factor, double offset);
APart        *Scale_Array_To_Range(APart *_R(_M(array)), Value min, Value max);

APart *Array_Op_Scalar  (APart *_R(_M(a)), Operator op, Value_Type type, Value val);
APart *Complex_Op_Scalar(APart *_R(_M(a)), Operator op, Value_Type type, Value rpart, Value ipart);

APart *Array_Op_Array    (APart *_R(_M(a)), Operator op, AForm *b);
APart *Complex_Op_Array  (APart *_R(_M(a)), Operator op, AForm *b);
APart *Complex_Op_Complex(APart *_R(_M(a)), Operator op, AForm *b);

APart *Threshold_Array(APart *_R(_M(a)), Value cutoff);
APart *Array_Fct_Val  (APart *_R(_M(a)), Value (*fct)(void *valp));
APart *Array_Fct_Idx  (APart *_R(_M(a)), Value (*fct)(Coordinate *coord));

Array *Convert_Array_Inplace(Array *_R(_M(array)), Array_Kind kind, Value_Type type, int scale,
                                                  ... _X( int|double factor ) );
Array *_G(Convert_Array)(Array *array, Array_Kind kind, Value_Type type, int scale,
                                                  ... _X( int|double factor ) );

boolean Image_Check(Array *array);

Array *Convert_Image_Inplace(Array *_R(_M(array)), Array_Kind kind, Value_Type type, int scale);
Array *_G(Convert_Image)(Array *array, Array_Kind kind, Value_Type type, int scale);

Array *_G(Array_Multiply)(Array *a, Array *b);

Array *_G(Apply_Map)(Array *image, Array *map);

Array *Down_Sample_Inplace(Array *_R(_M(source)), Coordinate *_F(box));
Array *_G(Down_Sample)(AForm *source, Coordinate *_F(box));

Array *Clip_Array_Inplace(Array *_R(_M(source)), Coordinate *_F(beg), Coordinate *_F(end));
Array *_G(Clip_Array)(AForm *source, Coordinate *_F(beg), Coordinate *_F(end));

Array *Pad_Array_Inplace(Array *_R(_M(source)), Coordinate *_F(anchor), Coordinate *_F(shape));
Array *_G(Pad_Array)(AForm *source, Coordinate *_F(anchor), Coordinate *_F(shape));


/****************************************************************************************
 *                                                                                      *
 * CORRELATION, COVARIANCE, LINEAR REGRESSION                                           *
 *                                                                                      *
 ****************************************************************************************/

double A_Correlation(AForm *a1, AForm *a2);
double A_Covariance(AForm *a1, AForm *a2);
double A_Pearson_Correlation(AForm *a1, AForm *a2);

Double_Matrix *_G(Correlations)(int n, AForm *a0, ... _X(AForm *an-1));
Double_Matrix *_G(Covariances)(int n, AForm *a0, ... _X(AForm *an-1));
Double_Matrix *_G(Pearson_Correlations)(int n, AForm *a0, ... _X(AForm *an-1));

typedef struct
  { double R2;               //  R-squared, coefficient of determination
    double aR2;              //  adjusted R-squared
    double ser;              //  standard error of the regression

    double tss;              //  total sum of squares
    double mss;              //  model sum of squares
    double rss;              //  residual sum of squares

    Double_Vector *std_err;  //  standard error of each parameter
    Double_Vector *t_stat;   //  t-statistic for each parameter

  } Regression_Bundle;

Double_Vector *_G(Linear_Regression)(int n, Regression_Bundle *_O(stats),
                                    AForm *obs, AForm *a0, ... _X(AForm *an-1));

Double_Vector *Simple_Regression(Array *_R(_O(vector)), Regression_Bundle *_O(stats),
                                 AForm *obs, AForm *inp);
}
#endif
