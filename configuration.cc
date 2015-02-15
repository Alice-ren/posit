/*****
      configuration.cc
      Configuration struct and related functions to walk a tree defining a configuration
******/

#include "configuration.hh"

typedef enum {EVENT, COMPLEMENT} t_node_type;

//Internal storage for a configuration space
class config_node {
public:
  config_node(config_node* super, double prob, event e) {
    this->super = super;
    sub_given = NULL;
    sub_excluded = NULL;
    this->remaining_completions = NULL;
    this->prob = prob;
    prob_current = true;
    this->e = e;
    node_type = EVENT;
  }

  config_node(config_node* super, double prob, completion_set* remaining_completions) {
    this->super = super;
    sub_given = NULL;
    sub_excluded = NULL;
    this->remaining_completions = remaining_completions;
    this->prob = prob;
    prob_current = true;
    node_type = COMPLEMENT;
  }
  
  void expand(const model& m, event_bounds t_bounds) { //FIXME: requiring a model here is not elegant
    free_subs(); //just in case there are some already
    if(node_type == EVENT || (remaining_completions == NULL && prob != 0.0)) {
      remaining_completions = new completion_set;
      *remaining_completions = m.get_first_order_completions(givens(), t_bounds); //FIXME: this is awkward.
    }
    
    completion cmp = remaining_completions->get_next_completion(m);
    
    if(remaining_completions->remaining_prob() == 0.0) {
      delete remaining_completions;
      sub_excluded = new config_node(this, 0.0, NULL);
    } else
      sub_excluded = new config_node(this, remaining_completions->remaining_prob(), remaining_completions);
    
    sub_given = new config_node(this, cmp.prob, cmp.e);
    
    remaining_completions = NULL; //We only want one pointer to this, otherwise we could double-delete
    node->invalidate_prob();
  }

  bool expanded() const {
    return (node->sub_given != NULL && node->sub_excluded != NULL);
  }

  invalidate_prob() {
    prob_current = false;
    if(super != NULL)
      super->invalidate_prob();
  }
  
  void free_subs() {
    if(sub_given != NULL) {
      sub_given->free_subs();
      delete sub_given;
    }
    if(sub_excluded != NULL) {
      sub_excluded->free_subs();
      delete sub_excluded;
    }
    if(remaining_completions != NULL)
      delete remaining_completions;

    sub_given = NULL;
    sub_excluded = NULL;
    remaining_completions = NULL;
  }

  occurrence givens() { //Not the most efficient way to write this
    occurrence occ;
    if(node_type == EVENT)
      occ = get_occurrence(e);

    if(super != NULL)
      return get_union(occ, super->givens());
    else
      return occ;
  }

  double prob() {
    if(!prob_current) {
      prob = 0.0; //so there is no path where prob is undefined
      if(expanded())
	prob = sub_given->prob() + sub_excluded->prob();
      
      prob_current = true;
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

  event sub_event() {
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
      sub_given->prob_current = true;
      return sub_exclusion;
    }
    return NULL;
  }

  //Mark the complement as probability zero
  config_node* choose_event() {
    if(expanded()) {
      sub_exclusion->free_subs();
      sub_exclusion->prob = 0.0;
      sub_exclusion->prob_current = true;
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

  void configuration::print(unsigned tab_layers) const {
    if(expanded())
      cout << "-->";
    else
      cout << "   ";
  
    cout << setw(10) << prob << " " << setw(10) << quality << "\t";
    for(unsigned i = 0;i < tab_layers;i++)
      cout << "\t";
    cout << "Given: " << endl;
    print_occurrence(givens());
  
    if(expanded()) {
      sub_given->print(tab_layers + 1);
      sub_excluded->print(tab_layers + 1);
    }	
  }
  
  t_node_type get_node_type() const {
    return node_type;
  }
  
private:
  double prob;
  bool prob_current;
  config_node* sub_given;
  config_node* sub_excluded;
  config_node* super;
  t_node_type node_type;
  event e;
  completion_set* remaining_completions;
};


//Member functions of the configuration space
configuration_space::configuration_space() {
  root_node = new config_node(NULL, 1.0, NULL);
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

void configuration_space::choose_complement() { //Mark the given as prob=0.00 and the exclusion as 1.00 without invalidating the whole tree
  if(fulcrum_node->expanded())
    fulcrum_node = fulcrum_node->choose_complement();
}

double configuration_space::next_event_cond_prob() { //conditional probability
  return fulcrum_node->sub_event_cond_prob();
}

//FIXME: what if the user did not do any predicting beforehand?
event configuration_space::next_event() {
  return fulcrum_node->sub_event();
}

void configuration_space::predict(const model &m, event_bounds t_bounds, unsigned max_events, double predict_time_limit) {
  for(unsigned i=0;i<max_events;i++) { //FIXME: Insert timing code.
    config_node* best_node = root.highest_probability_leaf_node();
    best_node->expand(m);
  }
}





