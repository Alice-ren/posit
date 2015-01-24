/*****
      bitfield.cc
      A little helper class for reading, writing binary data
******/

#include "bitfield.hh"

bitfield_reader::bitfield_reader(string filename) {
  current_bit_index = 0;
	
  //Read from the file
  ifstream inp_file;
  inp_file.open(filename.c_str());
  while(inp_file.good()) {
    unsigned c = unsigned(inp_file.get());
    if(c != EOF)
      f.data.push_back(c);
  }
  inp_file.close();
}

bool bitfield_reader::pop_bit() {
  bool val = f.get_bit(current_bit_index);
  current_bit_index++;
  return val;
}

bitfield_writer::bitfield_writer(string inp_filename) {
  current_bit_index = 0;
  filename = inp_filename;
}

void bitfield_writer::push_bit(bool val) {
  f.set_bit(current_bit_index, val);
  current_bit_index++;	
}

void bitfield_writer::write_file() {
  ofstream outp_file;
  outp_file.open(filename.c_str());
  for(unsigned i=0;i < f.data.size();i++)
    outp_file.put(char(f.data[i]));
  outp_file.close();
}

bool bitfield::get_bit(unsigned bit_index) {
  unsigned local_bit_index = bit_index & 7;
  unsigned index = (bit_index >> 3);
  unsigned char c;
  if(index < int(data.size()))
    c = data[index];
  else
    c = 0;
  if((c >> local_bit_index) & 1)
    return true;
  else
    return false;
}

void bitfield::set_bit(unsigned bit_index, bool val) {
  unsigned local_bit_index = 7 - (bit_index & 7);
  unsigned index = (bit_index >> 3);
  if(data.size() <= index)
    data.resize(index + 1);
	
  unsigned char c = data[index];
	
  if(val)
    c |=  ((unsigned char)(128) >> local_bit_index); //logical shift
  else
    c &= ~((unsigned char)(128) >> local_bit_index); //logical shift
	
  data[index] = c;
}

void bitfield::clear() {
  data.clear();
}

