/*****
      completion_set.cc
      A completion set is a set of mutually exclusive events at the same point in time.
******/

#include "completion_set.hh"

//The member functions of the completion_set class
completion_set::completion_set(int t_abs) {
  this->t_abs = t_abs;
  current_prior_p = 0;
  current_cmp = 0;
}

//There are actually two different kinds of completions we might return - calculated ones and pure prior counts
//completions.  This function abstracts them both into one type so the caller doesn't have to worry about the
//difference.
completion completion_set::get_next_completion (const model& m) {
  if(current_cmp < explicit_completions.size()) { //The easy case
    current_cmp++;
    completion cmp = (*(explicit_completions.begin() + current_cmp));
    remaining_total_prob -= cmp.prob;
    return cmp;
  } else {
    //Add to the list of prior completions as necessary
    if(current_prior_p < p_model->p_bounds.ub) {
      while(explicit_completions.find(current_prior_p) != explicit_completions.end())
	current_prior_p++;
      
      completion cmp; cmp.e.p = current_prior_p; cmp.e.t = t_abs; cmp.prob = prior_counts(something);
      remaining_total_prob -= cmp.prob;
      return cmp;
    } else {
      //We were asked for one that does not exist - return nothing
      if(remaining_total_prob != 0.0)
	cout << "ERROR: remaining total prob is " << remaining_total_prob << " with no elements left.\n";
      
      completion cmp;
      cmp.prob = 0.0;
      return cmp;
    }
  }
}

double completion_set::remaining_prob() const {
  return remaining_total_prob;
}

void completion_set::add_completion(const model& m, const event &e, double prob) { //We don't check for duplicates
  if(e.t != t_abs) { cout << "Bad value of e.t in completion_set::add_completion\n"; return; }
  completion cmp; cmp.e = e;cmp.prob = prob;
  remaining_total_prob += prob;
  remaining_total_prob -= m.prior_counts(something);  //FIXME: Could be optimized away
  explicit_completions[e.p] = cmp;
}
