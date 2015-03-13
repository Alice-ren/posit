/*****
      configuration.hh
      Configuration struct and related functions to walk a tree defining a configuration space
******/

#include "pattern.hh"
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
  double next_event_cond_prob() const;
  event next_event() const;
  void predict(unsigned max_events, double predict_time_limit, function<completion_set(pattern)> get_first_order_completions);
private:
  config_node* root_node;
  config_node* fulcrum_node;
};




#endif
