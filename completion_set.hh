/*****
      completion_set.hh
      A completion set is a set of mutually exclusive events at the same point in time.
******/

#ifndef COMPLETION_SET
#define COMPLETION_SET

#include <map>

class model;

//A completion is an event combined with the probability of that event (in some context)
typedef struct {
  event e;
  double prob;
} completion;

//A completion set is a convenient but efficient way to access the results of a probability query against the model.
//FIXME: not sorted in order of probability (to get the most probable one first)
class completion_set {
public:
  completion_set(int t_abs) { this->p_model = p_model; this->t_abs = t_abs; }
  completion get_next_completion(const model& m);
  double remaining_prob() const;
  void add_completion(const model& m, const event &e, double prob);
private:
  int t_abs;
  unsigned current_cmp;
  unsigned current_prior_p;
  map<unsigned, completion> explicit_completions;
  double remaining_total_prob;
};

#endif
