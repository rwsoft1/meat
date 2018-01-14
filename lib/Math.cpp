/******************************************************************************
 * Auto-generated by the Grinder Compiler Library
 */

#include <meat.h>
#include <meat/datastore.h>

#include <cmath>

#define PI 3.14159265

using namespace meat;

#define null (meat::Null())

/******************************************************************************
 * Math Class
 */

static meat::vtable_entry_t MathMethods[] = {
  {0x00000782, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x000007a0, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x00019850, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x00368f3a, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x00379f78, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x20be875b, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x24ab71da, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x331152ee, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x331156ce, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x34003578, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x39a68c12, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x39a6a1d2, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x48dbf560, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x6b2d9a7a, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x7a8e569a, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x7b840562, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x7d180801, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}}
};

// class method acos:
static Reference Math_cm_acos_(Reference context) {
  Reference self = cast<Context>(context).self();
  Reference klass = cast<Context>(context).klass();
  Reference value = cast<Context>(context).parameter(0);

    return new meat::Value(std::acos(FLOAT(value)));
  }

// class method asin:
static Reference Math_cm_asin_(Reference context) {
  Reference self = cast<Context>(context).self();
  Reference klass = cast<Context>(context).klass();
  Reference value = cast<Context>(context).parameter(0);

    return new meat::Value(std::asin(FLOAT(value)));
  }

// class method atan2y:x:
static Reference Math_cm_atan2y_x_(Reference context) {
  Reference self = cast<Context>(context).self();
  Reference klass = cast<Context>(context).klass();
  Reference y = cast<Context>(context).parameter(0);
  Reference x = cast<Context>(context).parameter(1);

    return new meat::Value(std::atan2(FLOAT(y), FLOAT(x)));
  }

// class method atan:
static Reference Math_cm_atan_(Reference context) {
  Reference self = cast<Context>(context).self();
  Reference klass = cast<Context>(context).klass();
  Reference value = cast<Context>(context).parameter(0);

    return new meat::Value(std::atan(FLOAT(value)));
  }

// class method ceiling:
static Reference Math_cm_ceiling_(Reference context) {
  Reference self = cast<Context>(context).self();
  Reference klass = cast<Context>(context).klass();
  Reference value = cast<Context>(context).parameter(0);

    return new meat::Value(std::ceil(FLOAT(value)));
  }

// class method cos:
static Reference Math_cm_cos_(Reference context) {
  Reference self = cast<Context>(context).self();
  Reference klass = cast<Context>(context).klass();
  Reference value = cast<Context>(context).parameter(0);

    return new meat::Value(std::cos(FLOAT(value)));
  }

// class method cosh:
static Reference Math_cm_cosh_(Reference context) {
  Reference self = cast<Context>(context).self();
  Reference klass = cast<Context>(context).klass();
  Reference value = cast<Context>(context).parameter(0);

    return new meat::Value(std::cosh(FLOAT(value)));
  }

// class method exp:
static Reference Math_cm_exp_(Reference context) {
  Reference self = cast<Context>(context).self();
  Reference klass = cast<Context>(context).klass();
  Reference value = cast<Context>(context).parameter(0);

    double answer = (double)std::exp(FLOAT(value));

    if (answer == HUGE_VAL) throw meat::Exception("exp range error");

    return new meat::Value(answer);
  }

// class method floor:
static Reference Math_cm_floor_(Reference context) {
  Reference self = cast<Context>(context).self();
  Reference klass = cast<Context>(context).klass();
  Reference value = cast<Context>(context).parameter(0);

    return new meat::Value(std::floor(FLOAT(value)));
  }

// class method log10:
static Reference Math_cm_log10_(Reference context) {
  Reference self = cast<Context>(context).self();
  Reference klass = cast<Context>(context).klass();
  Reference value = cast<Context>(context).parameter(0);

    if (FLOAT(value) <= 0.0)
      throw meat::Exception("Log10 parameter out of range");

    return new meat::Value(std::log10(FLOAT(value)));
  }

// class method log:
static Reference Math_cm_log_(Reference context) {
  Reference self = cast<Context>(context).self();
  Reference klass = cast<Context>(context).klass();
  Reference value = cast<Context>(context).parameter(0);

    if (FLOAT(value) <= 0.0)
      throw meat::Exception("Log parameter out of range");

    return new meat::Value(std::log(FLOAT(value)));
  }

// class method round:
static Reference Math_cm_round_(Reference context) {
  Reference self = cast<Context>(context).self();
  Reference klass = cast<Context>(context).klass();
  Reference value = cast<Context>(context).parameter(0);

    return new meat::Value((double)round(FLOAT(value)));
  }

// class method sin:
static Reference Math_cm_sin_(Reference context) {
  Reference self = cast<Context>(context).self();
  Reference klass = cast<Context>(context).klass();
  Reference value = cast<Context>(context).parameter(0);

    return new meat::Value(std::sin(FLOAT(value)));
  }

// class method sinh:
static Reference Math_cm_sinh_(Reference context) {
  Reference self = cast<Context>(context).self();
  Reference klass = cast<Context>(context).klass();
  Reference value = cast<Context>(context).parameter(0);

    return new meat::Value(std::sinh(FLOAT(value)));
  }

// class method sqrt:
static Reference Math_cm_sqrt_(Reference context) {
  Reference self = cast<Context>(context).self();
  Reference klass = cast<Context>(context).klass();
  Reference value = cast<Context>(context).parameter(0);

    if (FLOAT(value) < 0.0)
      throw meat::Exception("Square root parameter out of range");

    return new meat::Value(std::sqrt(FLOAT(value)));
  }

// class method tan:
static Reference Math_cm_tan_(Reference context) {
  Reference self = cast<Context>(context).self();
  Reference klass = cast<Context>(context).klass();
  Reference value = cast<Context>(context).parameter(0);

    return new meat::Value(std::tan(FLOAT(value)));
  }

// class method tanh:
static Reference Math_cm_tanh_(Reference context) {
  Reference self = cast<Context>(context).self();
  Reference klass = cast<Context>(context).klass();
  Reference value = cast<Context>(context).parameter(0);

    return new meat::Value(std::tanh(FLOAT(value)));
  }

// class method truncate:
static Reference Math_cm_truncate_(Reference context) {
  Reference self = cast<Context>(context).self();
  Reference klass = cast<Context>(context).klass();
  Reference value = cast<Context>(context).parameter(0);

   return new meat::Value((double)trunc(FLOAT(value)));
  }

static meat::vtable_entry_t MathCMethods[] = {
  {0x00000782, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x000007a0, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x00019850, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x002eaf93, 0x00247b28, VTM_NATIVE  , 1, Math_cm_cos_},
  {0x002fb9bd, 0x00247b28, VTM_NATIVE  , 1, Math_cm_exp_},
  {0x0032c576, 0x00247b28, VTM_NATIVE  , 1, Math_cm_log_},
  {0x0035de62, 0x00247b28, VTM_NATIVE  , 1, Math_cm_sin_},
  {0x003634b9, 0x00247b28, VTM_NATIVE  , 1, Math_cm_tan_},
  {0x00368f3a, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x05859774, 0x00247b28, VTM_NATIVE  , 1, Math_cm_acos_},
  {0x058cc643, 0x00247b28, VTM_NATIVE  , 1, Math_cm_asin_},
  {0x058d1c9a, 0x00247b28, VTM_NATIVE  , 1, Math_cm_atan_},
  {0x05a74899, 0x00247b28, VTM_NATIVE  , 1, Math_cm_cosh_},
  {0x0685f3aa, 0x00247b28, VTM_NATIVE  , 1, Math_cm_sinh_},
  {0x0689a71a, 0x00247b28, VTM_NATIVE  , 1, Math_cm_sqrt_},
  {0x068b6f7b, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x06906833, 0x00247b28, VTM_NATIVE  , 1, Math_cm_tanh_},
  {0x09e10969, 0x00247b28, VTM_NATIVE  , 2, Math_cm_atan2y_x_},
  {0x20be875b, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x24ab71da, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x34345daf, 0x00247b28, VTM_NATIVE  , 1, Math_cm_floor_},
  {0x39a68c12, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x39a6a1d2, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x3e972438, 0x00247b28, VTM_NATIVE  , 1, Math_cm_log10_},
  {0x4439e83c, 0x00247b28, VTM_NATIVE  , 1, Math_cm_ceiling_},
  {0x48db806d, 0x00247b28, VTM_NATIVE  , 1, Math_cm_round_},
  {0x54aa30e6, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x5fd81970, 0x00247b28, VTM_NATIVE  , 1, Math_cm_truncate_},
  {0x6b2d9a7a, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x7a8e569a, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x7b840562, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}},
  {0x7d180801, 0x00000000, VTM_SUPER   , 0, {(meat::method_ptr_t)0}}
};

static std::uint8_t Symbols[] = {
  "Math\0acos:\0asin:\0atan2y:x:\0atan:\0ceiling:\0cos:\0cosh:\0exp:\0floor:\0"
  "log10:\0log:\0round:\0sin:\0sinh:\0sqrt:\0tan:\0tanh:\0truncate:\0\0"
};

// We need to declare init_Math as a C function.
extern "C" {
  DECLSPEC void init_Math(meat::data::Library &library);
}

void init_Math(meat::data::Library &library) {
  meat::Class *cls;

  // Import required libraries.

  cls = new meat::Class(meat::Class::resolve(0x0c658f60), 0, 0);
  cls->set_vtable(17, MathMethods, meat::STATIC);
  cls->set_class_vtable(32, MathCMethods, meat::STATIC);
  library.add(cls, "Math");

  library.set_symbols(Symbols, meat::STATIC);
}
