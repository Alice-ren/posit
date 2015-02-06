/*****
      model.h
      Defines probability model over events and sets of events
******/

#include "occurrence.hh"
#include "pattern.hh"
#include <vector>
#include <list>
#include <map>
#include <iostream>
#include <algorithm>
#include <math.h>
#include <cfloat>
#include <chrono>
#include <ctime>
using namespace std;

#ifndef MODEL
#define MODEL

/*
  The model consists of a first order mapping of a current state onto a number of future states, along with a probability of each.
  The critical assumption here is that the probability of any sequence of events is time shift independent - constant with respect to time.
  In the case of image processing or other applications the probability of sequences of events might be independent with respect to both the horizontal and vertical axis of the image, giving essentially 2 "time" coordinates.
*/

/*
  The relationship between the model and the configuration space is complex.  It's not complicated, but it is complex.
  Here is why:  In a sense they both generate the same thing- a list of events with a probability of each.  So why have both, and why separate them this way?
  The model contains a set of first-order predictions or probabilities that assume a finite amount of context.  Now some sequences of events are conditionally independent of others, and some are not.  For those which only depend on other events within our chosen context, we can correctly generate extended probabilities by chaining the first order possibilities together transitively.  Doing this we can generate a tree of possibilities.  Some possibilities are mutually exclusive, so after eliminating those we can walk back up the tree and generate an "effective" probability which will be different than the first-order probability obtained from the model.
  Now if two event sequences are not conditionally independent, then we will obtain the wrong result by doing this and we need to incorporate more context for those sequences into the first-order model.  But the amount of context can never be infinite, so by chaining probabilites we always get some benefit.
  The process of chaining probabilities can therefore be separated from the process of generating a first-order model, so that we could drop in an alternative first-order model based on neural nets or some other architecture.
*/

/*
  Any model is subject to a few constraints:  First, that it has the public functions below.  Second, the completions that the model returns cannot ever have exactly zero or exactly 100% probability.  In fact if a certain event can occur at all, the model must give some nonzero probability for it to occur at any time.
*/

class model;

//A completion is an event combined with the probability of that event (in some context)
typedef struct {
  event e;
  double prob;
} completion;

//A completion set is a convenient but efficient way to access the results of a probability query against the model.
class completion_set {
public:
  completion_set(model* p_model, unsigned t_abs) { this->p_model = p_model; this->t_abs = t_abs; }
  completion operator[] (unsigned i) const;
  unsigned size() const;
  double total_prob() const;
  void add_completion(const event &e, double prob);
private:
  unsigned t_abs;
  map<unsigned, completion> explicit_completions;
  vector<completion> prior_completions;
  double explicit_completions_total_prob;
  model* p_model; //Used to find prior_count
};

class model {
 public:
  model(unsigned memory_constraint);
  void train(const occurrence &givens);
  double prob(const occurrence& occ);
  double conditional_prob(const occurrence& occ, const occurrence& givens);
  void get_first_order_completions(occurrence& occ, unsigned t_abs, completion_set& result_set)
 private:
  void add_pattern(pattern p, double count);
  void add_sample(pattern* p, double count);
  double prior_count(unsigned pattern_length) const; //Assume an even prior distribution of events and patterns
  double sample_size(const pattern& p);
  bool is_match(const occurrence& occ, const pattern& p, unsigned t_abs) const;
  double local_prob(const pattern& p) const;
  unsigned get_new_visit_id() const;
  void find_terms(const occurrence& occ, list<term> &terms, pattern& patt, unsigned t_abs, unsigned visit_id) const;
  
  list<pattern> top_level_patterns;
  pattern base_level_pattern;
  unsigned memory_constraint;
  double total_num_events;
  unsigned current_visit_id;
  event_bounds p_bounds;
  double PRIOR_EVENT_DENSITY = 1.0;
  double PRIOR_INTERVAL = 1.0;
};

#endif
