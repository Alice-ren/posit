/****
     file_io.hh
     A couple of helper functions to read / write characters from a file to a vector of unsigned ints.
****/

#ifndef FILE_IO
#define FILE_IO

#include <string>
#include <fstream>
#include <vector>

//I'm not sure why I wrote these to play fast and loose with char / unsigned.
//Don't use these as written.
void read_file(string filename, vector<unsigned> &inp_data) {
  ifstream inp_file;
  inp_file.open(filename.c_str());
  while(inp_file.good()) {
    unsigned c = unsigned(inp_file.get());
    if(c == EOF)
      c = 256; //Why did I do this.
    inp_data.push_back(c);
  }
  inp_file.close();
}

void write_file(string filename, const vector<unsigned>& outp_data) {
  ofstream outp_file;
  outp_file.open(filename.c_str());
	
  for(unsigned i=0;i < outp_data.size();i++) {
    outp_file.put(char(outp_data[i]));
  }
	
  outp_file.close();
}

#endif
