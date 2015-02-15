/*****
      configuration.hh
      Configuration struct and related functions to walk a tree defining a configuration space
******/

#include "occurrence.hh"
#include "model.h"
#include <list>
#include <iostream>
#include <iomanip>

using namespace std;

#ifndef CONFIGURATION
#define CONFIGURATION

class config_node;

class configuration_space {
public:
  configuration_space();
  ~configuration_space();
  void choose_event();
  void choose_complement();
  double next_event_cond_prob();
  event next_event();
  void predict(const model &m, event_bounds t_bounds, unsigned max_events, double predict_time_limit);
private:
  config_node* root_node;
  config_node* fulcrum_node;
};




#endif
