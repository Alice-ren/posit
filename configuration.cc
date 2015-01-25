/*****
      configuration.cc
      Configuration struct and related functions to walk a tree defining a configuration
******/

#include "configuration.hh"

//Internal storage for a configuration
class config_node {
public:
  config_node() { sub_given = NULL; sub_excluded = NULL; cond_prob = 0.0; cond_quality = 0.0; }
  void init_subs(event e, double cond_prob, double cond_quality) { 
    sub_given = new config_node;
    sub_excluded = new config_node; 
    this->cond_prob = cond_prob;
    this->cond_quality = cond_quality;
    this->sub_e = e; 
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
  }
  event sub_e;
  double cond_prob; //probability in context; conditional probability
  double cond_quality; //Really this is going to be a confidence measure; I don't know how confidence commutes though
  config_node* sub_given;
  config_node* sub_excluded;
};

//Member functions of the configuration wrapper function
configuration::configuration(double prob, double quality) {
  this->prob = prob;
  this->quality = quality;
}

configuration configuration::sub_given() const {
  configuration ret;
  if(node->sub_given == NULL) return ret; //with prob=0.0
  ret.node = node->sub_given;
  ret.prob = prob*node->cond_prob;
  ret.quality = quality*node->cond_quality;
  ret.given = get_union(get_occurrence(node->sub_e), given);
  ret.excluded = excluded;
  return ret;
}

configuration configuration::sub_excluded() const {
  configuration ret;
  if(node->sub_excluded == NULL) return ret; //with prob=0.0
  ret.node = node->sub_excluded;
  ret.prob = prob*(1.0 - node->cond_prob);
  ret.quality = quality*node->cond_quality;
  ret.given = given;
  ret.excluded = get_union(get_occurrence(node->sub_e), excluded); 
  //FIXME: account for constraints; ie if we have a given at the same point in time as an excluded and a single valued constraint, 
  //we don't need to store the excluded anymore
  return ret;
}
bool configuration::expanded() const {
  return (node->sub_given != NULL || node->sub_excluded != NULL);
}

//Initialize and allocate on the heap
void configuration::expand(const completion& cmp) {
  free_subs(); //Just to make sure we don't leave any dangling pointers, but please check to see if this is already expanded before you do this
  node->init_subs(cmp.e, cmp.prob, cmp.quality);
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
  print_occurrence(given);
  cout << "Excld: " << endl;
  print_occurrence(excluded);
  
  if(expanded()) {
    sub_given().print(tab_layers + 1);
    sub_excluded().print(tab_layers + 1);
  }	
}

void configuration::enumerate_subconfigs(list<configuration> &subconfigs) const {
  if(expanded()) {
    sub_given().enumerate_subconfigs(subconfigs);
    sub_excluded().enumerate_subconfigs(subconfigs);
  } else {
    subconfigs.push_back(*this);
  }
}

configuration configuration::highest_quality_subconfig(configuration current_best) const {
  if(quality > current_best.quality) {
    if(expanded()) {
      current_best = sub_given().highest_quality_subconfig(current_best);
      current_best = sub_excluded().highest_quality_subconfig(current_best);
    } else {
      current_best = (*this);
    }
  }
  
  return current_best;
}

//Free memory from the heap.  Assuming no aliasing here.  If we are in the habit of making copies of configurations.. this becomes dangerous.
//To Do: write copy constructor, destructor, etc.  The way I did this is kind of wacky.
void configuration::free_subs() {
  node->free_subs();
}


