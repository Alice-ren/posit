/*****
      configuration.hh
      Configuration struct and related functions to walk a tree defining a configuration space
******/

#include "occurrence.hh"
#include <list>
#include <iostream>
#include <iomanip>

using namespace std;

#ifndef CONFIGURATION
#define CONFIGURATION

/*
  The configuration class is a wrapper around the real data structure.
  Use it like:
  configuration root(1.0, 1,0);
  root.expand(cmpl);
  configuration gc = root.sub_given();
  configuration ec = root.sub_excluded();
  ec.print_configuration();  
  root.free_all();
  To Do: try to figure out how I can make this cleaner / more obvious.
*/

class config_node;

class configuration {
public:
  occurrence given;
  occurrence excluded;
  double prob; //absolute probability
  double quality; //absolute quality
  configuration(double prob = 0.0, double quality = 0.0);
  configuration sub_given() const;
  configuration sub_excluded() const;
  bool expanded() const;
  void expand(const completion& cmp);
  void print(unsigned tab_layers = 0) const;
  void enumerate_subconfigs(list<configuration> &subconfigs) const;
  configuration highest_quality_subconfig(configuration current_best = configuration()) const;
  //Free memory from the heap.  Assuming no aliasing here.  If we are in the habit of making copies of configurations.. this becomes dangerous.
  //To Do: write copy constructor, destructor, etc.  The way I did this is kind of wacky.
  void free_subs();
private:
  config_node *node;
};





#endif
