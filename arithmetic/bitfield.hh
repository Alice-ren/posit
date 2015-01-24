/****
     bitfield.hh
     A little helper class for reading, writing binary data

     Use the bitfield class to read and write bits to a vector of bits in memory.  The vector resizes itself automatically.
     Use the bitfield_reader class to read one bit at a time from a file.
     Use the bitfield_writer class to write one bit at a time to a file.

     Limitations:
     --This class has a maximum size of 2^32 bits, and depending on system constraints possibly much less.
     In the future therefore it will be advantageous to have the bitfield define a smaller working vector
     and write in and out automatically when that buffer gets used up.
     --Please don't try to read and write from the same file at the same time.
     --The reader does not throw any kind of exception when you read past the end of the file; it just returns a zero.
****/

#ifndef BITFIELD_H
#define BITFIELD_H

#include <string>
#include <fstream>
#include <vector>

using namespace std;

//A bitfield is a big array of bits that we can read from and write to.  It is dynamically resizing.
class bitfield {
public:
  bool get_bit(unsigned bit_index);  //Returns zero for an undefined bit.  FIXME: should throw an exception.
  void set_bit(unsigned bit_index, bool val);  //Allows for automatic resizing to the length required by bit_index
  void clear();
private:
  vector<unsigned char> data;
  friend class bitfield_reader;
  friend class bitfield_writer;
};

//Read bits from a file, one at a time.
class bitfield_reader {
public:
  bitfield_reader(string filename);
  bool pop_bit(); //pop from the *front*.  FIXME: throw an exception or something for end of file.
private:
  bitfield f;
  unsigned current_bit_index;
};

//Write bits to a file, one at a time.
class bitfield_writer {
public:
  bitfield_writer(string inp_filename);
  void push_bit(bool val); //push onto the back
  void write_file();
private:
  bitfield f;
  string filename;
  unsigned current_bit_index;
};

#endif
