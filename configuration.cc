/*****
      configuration.cc
      Configuration struct and related functions to walk a tree defining a configuration space
******/

#include "configuration.h"

/*
  The config_node class below is the real data structure holding information about a configuration space.
  The configuration class acts like an iterator onto that structure, providing a nice interface onto the space.
  Use it like:
  configuration root(1.0, 1,0);
  root.expand(some_event, .3, .9);
  configuration gc = root.sub_given();
  configuration ec = root.sub_excluded();
  root.print_configuration(configuration* c);  
  root.free_all();
  
 */

class config_node {
public:
  config_node() { sub_given = NULL; sub_excluded = NULL; cond_prob = 0.0; cond_quality = 0.0; }
  init_subs(event e, double cond_prob, double cond_quality) { 
    sub_given = new config_node;
    sub_excluded = new config_node; 
    this->cond_prob = cond_prob;
    this->cond_quality = cond_quality;
    this->sub_e = e; 
  }
  free_all() {
    if(sub_given != NULL) {
      sub_given->free_all();
      delete sub_given;
    }
    if(sub_excluded != NULL) {
      sub_excluded->free_all();
      delete sub_excluded;
    }
  }
  event sub_e;
  double cond_prob; //probability in context; conditional probability
  double cond_quality; //Really this is going to be a confidence measure; I don't know how confidence commutes though
  config_node* sub_given;
  config_node* sub_excluded;
};

class configuration {
public:
  occurrence given;
  occurrence excluded;
  double prob; //absolute probability
  double quality; //absolute quality
  configuration(double prob = 0.0, double quality = 0.0) { this->prob = prob; this->quality = quality; }
  configuration sub_given() const {
    configuration ret;
    if(sub_given == NULL) return ret; //with prob=0.0
    ret.node = node->sub_given;
    ret.prob = prob*node.cond_prob;
    ret.quality = quality*node.cond_quality;
    ret.given = union(get_occurrence(node.sub_e), given);
    ret.excluded = excluded;
    return ret;
  }
  configuration sub_excluded() const {
    configuration ret;
    if(sub_excluded == NULL) return ret; //with prob=0.0
    ret.node = node->sub_excluded;
    ret.prob = prob*(1.0 - node.cond_prob);
    ret.quality = quality*node.cond_quality;
    ret.given = given;
    ret.excluded = union(get_occurrence(node.sub_e), excluded); 
    //FIXME: account for constraints; ie if we have a given at the same point in time as an excluded and a single valued constraint, 
    //we don't need to store the excluded anymore
    return ret;
  }
  bool expanded() const { return (node.sub_given != NULL || node.sub_excluded != NULL); }
  //Initialize and allocate on the heap
  void expand(const completion& cmp) {
    free_all(); //Just to make sure we don't leave any dangling pointers, but please check to see if this is already expanded before you do this
    node.init_subs(cmp.e, cmp.prob, cmp.quality);
  }
  void print(unsigned tab_layers = 0) const {
    if(expanded())
      cout << "-->";
    else
      cout << "   ";
    
    cout << setw(10) << prob << " " << setw(10) << quality << "\t";
    for(unsigned i = 0;i < tab_layers;i++)
      cout << "\t";
    cout << "Given: " << print_occurrence(given);
    cout << "Excld: " << print_occurrence(excluded);
    
    if(expanded()) {
      sub_given().print(tab_layers + 1);
      sub_excluded().print(tab_layers + 1);
    }	
  }
  void enumerate_subconfigs(const list<configuration> &subconfigs) const {
    if(expanded()) {
      sub_givens().enumerate_subconfigs(subconfigs);
      sub_excluded().enumerate_subconfigs(subconfigs);
    } else {
      subconfigs.push_back(*this);
    }
  }
  configuration highest_quality_subconfig(configuration current_best = configuration()) const {
    if(quality > current_best.quality) {
      if(expanded()) {
	current_best = sub_givens().highest_quality_subconfig(current_best);
	current_best = sub_excluded().highest_quality_subconfig(current_best);
      } else {
	current_best = (*this);
      }
    }
    
    return current_best;
  }
  //Free memory from the heap.  Assuming no aliasing here.  If we are in the habit of making copies of configurations.. this becomes dangerous.
  //To Do: write copy constructor, destructor, etc.  The way I did this is kind of wacky.
  void free_all() {
    node.free_all();
  }
private:
  config_node node;
};

configuration_space::configuration_space(unsigned mem_constraint) {
  root.prob = 1.0;
  root.quality = 1.0;
  this->mem_constraint = mem_constraint;
}

configuration_space::~configuration_space() {
  root.free_all();
}

//FIXME: do this one.  I think the method, for now, to just run through every configuration in the tree, and keep a histogram of all the events in any configuration, then at the end, pick the one with the highest probability and return it.
//This is again a stub in the sense that this is by no means the most efficient way to do this.
completion configuration_space::most_probable_event() const {
  //histogram of possible events
  map<event, double> histogram;

  list<configuration> subconfigs;
  root.enumerate_subconfigs(subconfigs);
  for(auto p_config = subconfigs.begin(); p_config != subconfigs.end(); p_config++) {
    //enter the events into the histogram
    for(auto p_e = p_config.given.begin(); p_e != p_config.given.end(); p_e++) {
      histogram[(*p_e)] += p_config.prob;
    }
  }

  //Find the event with the highest total probability
  if(histogram.size() != 0) {
    completion best_cmp; best_cmp.prob = 0.0;
    for(auto p_hist = histogram.begin(); p_hist != histogram.end();p_hist++) {
      if(p_hist->second > best_cmp.prob) {
	best_cmp.e = p_hist->first;
	best_cmp.prob = p_hist->second;
      }
    }
    return best_cmp;
  }
  else {
    completion nothing_cmp;
    nothing_cmp.prob = 0.0;
    return nothing_cmp;
  }
}

//For the given event, mark the indicated event as given (probability 100%).
void configuration_space::given(const event &e) {
  root.given = union(root.given, get_occurrence(e));
  root.free_all();
}

//For the given event, mark the event as excluded (probability 0%).
void configuration_space::exclude(const event &e) {
  root.excluded = union(root.excluded, get_occurrence(e));
  root.free_all();
}

void configuration_space::predict(const model &m, event_bounds t_bounds, event_bounds p_bounds, unsigned max_depth, double predict_time_limit) {
  
  //Start search timer
  auto start = chrono::high_resolution_clock::now();
  
  int i = 0;
  do {
    if(i >= 2)
      break;
    
    chrono::duration<double> secs = chrono::high_resolution_clock::now() - timer_start;
    double elapsed_time = double(secs.count());
    if(elapsed_time > search_time) {
      cout << "elapsed:" << elapsed_time << " target: " << search_time << " test: " << (elapsed_time > search_time) << endl << flush;
      break;
    }    

    configuration config_to_expand = root.highest_quality_subconfig();
    
    //Get the completions to this occurrence in the space
    list<completion> completions;
    context ctx;
    ctx.t_bounds = t_bounds;
    ctx.p_bounds = p_bounds;
    ctx.givens = config_to_expand.givens;
    ctx.exclusions = config_to_expand.exclude;
    m.get_first_order_completions(ctx, completions);
    
    //Add the completions to the configuration space
    completions.sort(compare_quality);  //ok this is inefficient
    config_to_expand.expand(completions.front());
    
  } while(true);
  
  sec = chrono::system_clock::now() - start;
  cout << "took " << sec.count() << " seconds\n";
  
  cout << "=========config tree:=========" << endl;
  root.print();
  cout << "==============================" << endl;
}



