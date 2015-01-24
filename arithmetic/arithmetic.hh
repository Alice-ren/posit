/****
     arithmetic.hh
     Defines algorithms related to arithmetic coding

     To use this library, simply give the reader or the writer a filename and start pushing or extracting bits.
     But- in order to extract or write bits, you must supply a probability that the next bit will be a 1,
     assuming that you don't know what the next bit is beforehand.  This is seemingly odd when we are giving
     the actual value of the bit in the same function, but it is important to the data compression.

     Limitations: 
     --You cannot rewind or decode out of order.
     --Uses the bitfield class and the restrictions of that class carry forward.  (512mB filesize limit, currently)
     --For this function to give sane results, the probability given to the writer and the reader must be exactly
     the same for the same bits.
     --This library does not specify a stop or 'no more bits' character!  It is up to the user to decide ahead of
     time what sequence of bits (what character) means 'end of file', or code the file length uncompressed at the
     beginning.
****/

#ifndef ARITHMETIC_H
#define ARITHMETIC_H

#include "bitfield.hh"

class arithmetic_reader {
public:
  arithmetic_reader(string filename);
  bool pop_bit(double prob); //prob is the probability (supplied by the user) that the next bit will be a 1.
private:
  void flush_bits();
  //State variables.
  bitfield_reader field;
  double ub; //upper bound
  double lb; //lower bound
  double d;  //number representing our data
};

class arithmetic_writer {
public:
  arithmetic_writer(string filename);
  void push_bit(double prob, bool val); //prob is the probability (supplied by the user) that the next bit will be a 1.
  void write_file();
private:
  void flush_bits();
  //State variables.
  bitfield_writer field;
  double ub; //upper bound
  double lb; //lower bound
  unsigned carry_bits;
};

#endif
