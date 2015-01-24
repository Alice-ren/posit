/*****
czip.cc
czip stands for 'Carter Zip'.
File zipper which compresses data based on the probabilities and structure within the data.
This program is designed to achieve the maximum compression possible with little regard for execution speed.
*****/

#include <iostream>
#include <string>
#include <fstream>
#include "table.h"
#include "arithmetic.h"
#include "file_io.h"

using namespace std;

void compress_using_static_markov_chain(const vector<unsigned> &data, unsigned chain_len, arithmetic_writer &w) {
	if(data.size() == 0) return;
	
	//Calculate table dimensions
	unsigned min = 0;
	unsigned max = 256;
	
	vector<dim_type> table_dims;
	table_dims.resize(chain_len);
	for(unsigned i=0;i < chain_len;i++) {
		table_dims[i].ub = max;
		table_dims[i].lb = min;
	}
	
	//Create and fill table
	table T(table_dims);

	for(unsigned i = 0;i < data.size();i++) {
		constraint_set initial_constraints;
		if(i < chain_len - 1)
			gen_even_distribution(min, max, initial_constraints);
		else
			gen_table_distribution(T, data, i, chain_len, min, max, initial_constraints);
		
		while(initial_constraints.size() != 0 && data[i] != initial_constraints[0].p[initial_constraints[0].p.size() - 1]) {
			w.push_bit(initial_constraints[0].prob, false);
			remove_distribution_element(initial_constraints, 0);
		}
		if(initial_constraints.size() == 0) { cout << "EMPTY CONSTRAINT SET" << endl; return; }
		w.push_bit(initial_constraints[0].prob, true);
		if(i >= chain_len - 1)
			T.add_sample(initial_constraints[0].p);
	}
}

int main(int argc, char* argv[]) {
	//Grab filename
	if(argc < 2) {cout << "Please provide a filename." << endl; return 0;}
	string filename = argv[1];
	cout << "Compressing " << filename << " into " << filename << ".cz   ..." << endl;
	
	//Read file into event stream
	vector<unsigned> inp_data;
	read_file(filename, inp_data);
	
	//Get compressed data stream
	string outp_filename = filename + ".cz";
	arithmetic_writer w(outp_filename);
	compress_using_static_markov_chain(inp_data, 1, w);

	//Write compressed file
	w.write_file();
	
	//Remove original file.  FIXME: this is kind of dangerous without some better error reporting above.
	remove(filename.c_str());
	
	cout << "done." << endl;
	return 0;
}
