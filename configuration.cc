/*****
      configuration.cc
      Configuration struct and related functions to walk a tree defining a configuration
******/

#include "configuration.hh"

//Internal storage for a configuration space
class config_node {
public:
  config_node(config_node* super, double prob, event e) {
    if(super == NULL && e.t != 0)
      cout << "ERROR: first event in configuration space not at t=0";      
      
    this->super = super;
    sub_given = NULL;
    sub_complement = NULL;
    this->prob = prob;
    prob_needs_update = false;
    this->e = e;
  }

  void expand(int t, double given_prob, double complement_prob) {
    free_subs(); //just in case we are trying to do something wierd
    sub_given = new config_node(this, given_prob, event(t, 1));
    sub_complement = new config_node(this, complement_prob, event(t, 0));
    invalidate_prob();
  }

  bool expanded() const {
    return (node->sub_given != NULL && node->sub_complement != NULL);
  }

  invalidate_prob() {
    prob_needs_update = true;
    if(super != NULL)
      super->invalidate_prob();
  }
  
  void free_subs() {
    if(sub_given != NULL) {
      sub_given->free_subs();
      delete sub_given;
    }
    if(sub_complement != NULL) {
      sub_complement->free_subs();
      delete sub_complement;
    }

    sub_given = NULL;
    sub_complement = NULL;
  }
  
  //Note the assumption that the first event is at t=0 and all other events have t_offset relative to that
  occurrence givens() const { //Not the most efficient way to write this
    if(super != NULL)
      return get_union(super.givens(), e.t, pattern(e.p));
    else
      return pattern(e.p);
  }

  double prob() {
    if(prob_needs_update) {
      prob = 0.0; //so there is no path where prob is undefined
      if(expanded())
	prob = sub_given->prob() + sub_complement->prob();
      
      prob_needs_update = false;
    }
  
    return prob;
  }

  double sub_event_cond_prob() { //returns the conditional probability of the topmost event in the tree.
    double p = prob();

    if(p == 0.0)
      return p;
    
    if(expanded())
      return sub_given->prob()/p;

    return 0.0;
  }

  event sub_event() const {
    if(expanded())
      return sub_given->e;
    else {
      event bogus;
      return bogus;
    }
  }
  
  //mark the event as probability zero
  config_node* choose_complement() {
    if(expanded()) {
      sub_given->free_subs();
      sub_given->prob = 0.0;
      sub_given->prob_needs_update = false;
      return sub_complement;
    }
    return NULL;
  }

  //Mark the complement as probability zero
  config_node* choose_event() {
    if(expanded()) {
      sub_complement->free_subs();
      sub_complement->prob = 0.0;
      sub_complement->prob_needs_update = false;
      return sub_given;
    }
    return NULL;
  }

  config_node* highest_probability_leaf_node(double &current_best = 0.0) {
    if(prob() < current_best)
      return NULL;
    else {
      config_node* best_node = NULL;
      if(expanded()) {
	config_node* given_best_node = sub_given->highest_probability_leaf_node(current_best);
	if(given_best_node != NULL)
	  best_node = given_best_node;
	config_node* excl_best_node = sub_given->highest_probability_leaf_node(current_best);
	if(excl_best_node != NULL)
	  best_node = excl_best_node;
	
      } else {
	best_node = this;
	current_best = prob();
      }
      
      return best_node;
    }
  }

  void configuration::print(unsigned tab_layers) {
    if(expanded())
      cout << "-->";
    else
      cout << "   ";
  
    cout << setw(10) << prob() << " " << setw(10) << quality << "\t";
    for(unsigned i = 0;i < tab_layers;i++)
      cout << "\t";
    cout << "Given: " << endl;
    print_pattern(givens());
  
    if(expanded()) {
      sub_given->print(tab_layers + 1);
      sub_complement->print(tab_layers + 1);
    }	
  }
  
private:
  double prob;
  bool prob_needs_update;
  config_node* sub_given;
  config_node* sub_complement;
  config_node* super;
  event e;
};


//Member functions of the configuration space
configuration_space::configuration_space() {
  root_node = new config_node(NULL, 1.0, event(1, 0));
  fulcrum_node = root_node;
}

configuration_space::~configuration_space() {
  root_node->free_subs();
  delete root_node;
}

void configuration_space::choose_event() {  //Make the given node the root node
  if(fulcrum_node->expanded())
    fulcrum_node = fulcrum_node->choose_event();
}

void configuration_space::choose_complement() { //Mark the given as prob=0.00 and the complement as 1.00 without invalidating the whole tree
  if(fulcrum_node->expanded())
    fulcrum_node = fulcrum_node->choose_complement();
}

double configuration_space::next_event_cond_prob() const { //conditional probability
  return fulcrum_node->sub_event_cond_prob();
}

event configuration_space::next_event() const {
  if(!fulcrum_node->expanded())
    cout << "ERROR: fulcrum node is not expanded; no next event to return.\n";
  
  return fulcrum_node->sub_event();
}

void configuration_space::predict(unsigned max_events, double predict_time_limit, function<completion_set(pattern)> get_first_order_completions) {
  for(unsigned i=0;i<max_events;i++) { //FIXME: Insert timing code.
    config_node* best_node = root.highest_probability_leaf_node();
    completion_set cmp = get_first_order_completions(best_node->givens());
    best_node->expand(cmp.t_offset, cmp.event_prob, cmp.complement_prob);
  }
}





