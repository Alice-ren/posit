/*****
      configuration_space.cc
      Configuration struct and related functions to walk a tree defining a configuration space
******/

#include "configuration_space.hh"

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



