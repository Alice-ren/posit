/*****
      model.cc
      Defines probability spaces over events and sets of events
******/

#include "model.hh"

/*
  A word on the storage strategy of patterns.
  Whenever we have some pattern ABC, if we have one count of ABC then we correspondingly have one count of AB, BC and A_C.
  We may still store a separate pattern AB, however, because we have found counts of AB which are not part of the larger pattern ABC.
  In that case, the raw count for AB will represent the count of occurrences of AB&(~C).  Therefore when we want the total count of AB*, we must take the sum of the count for AB, and add in the counts for ABC, ABQ and any other superpattern.  So the raw count for AB will represent the counts of AB which were not part of *any* superpattern.
  The subdivide() function is not written yet, but when it is, we may do things like subdivide 1x"QABC" into 1x"QAB" and 1x"ABC".  In that case we would also want to generate or update the pattern "AB" with a negative count.  So if there were no previously existing examples of "AB" by itself we would have -1x"AB".  That seems wierd but it is consistent-because we add in the counts for superpatterns, if we ask for the count of ABC, we get one, QAB, we get one, and if we ask for the count of AB we get one (1+1-1).
*/

//The member functions of the completion_set class
completion_set::completion_set(model* p_model, unsigned t_abs) {
  this->p_model = p_model;
  this->t_abs = t_abs;
}

//There are actually two different kinds of completions we might return - calculated ones and pure prior counts
//completions.  This function abstracts them both into one type so the caller doesn't have to worry about the
//difference.
completion completion_set::operator[] (unsigned i) const {
  if(i < explicit_completions.size()) //The easy case
    return *(explicit_completions.begin() + i);
  else {
    //Add to the list of prior completions as necessary
    unsigned prior_i = i - explicit_completions.size(); //prior_i is the index into the explicit completions vector
    if(prior_i >= prior_completions.size()) { //We don't have that one yet
      unsigned prior_p; //prior_p is the position corresponding to this prior_i
      if(prior_completions.empty())
	prior_p = p_model->p_bounds.lb;
      else
	prior_p = prior_completions.back().e.p + 1;
      
      while(prior_i >= prior_completions.size()) { //until the list of prior completions includes the one we want,
	if(explicit_completions.find(prior_p) == explicit_completions.end()) {
	  //if this is not an explicit completion,
	  //add it to the list of prior completions
	  completion cmp; cmp.e.p = prior_p; cmp.e.t = t_abs; cmp.prob = prior_counts(something);
	  prior_completions.push_back(cmp);  
	}
	
	prior_p++;
	if(prior_p > p_model->p_bounds.ub) //Ran out of possibilities
	  break;
      }
    }
    
    //Pull the appropriate completion and return it
    if(prior_i < prior_completions.size()) {
      return prior_completions[prior_i];
    } else {
      //We were asked for one that does not exist - return nothing
      completion cmp;
      cmp.prob = 0.0;
      return cmp;
    }
  }
}

unsigned completion_set::size() const {
  return p_model->p_bounds.ub - p_model->p_bounds.lb;
}

double completion_set::total_prob() const {
  return explicit_completions_total_prob + p_model->prior_counts(something);
}

void completion_set::add_completion(const event &e, double prob) { //We don't check for duplicates
  if(e.t != t_abs) { cout << "Bad value of e.t in completion_set::add_completion\n"; return; }
  completion cmp; cmp.e = e;cmp.prob = prob;
  explicit_completions_total_prob += prob;
  explicit_completions[e.p] = cmp;
}

//Member functions for the model class
model::model(unsigned memory_constraint) {
  this->memory_constraint = memory_constraint;
  total_num_events = 0.0;

  PRIOR_EVENT_DENSITY = 1.0;
  PRIOR_INTERVAL = 1.0;
	
  //Load the counts for the base case N=1 with even prior distribution
  /*const double PRIOR_COUNTS = 0.02;  //We don't ever want to assume zero probability for something, so we initialize to a small prior count.
    for(unsigned i=0;i < (bounds.event_ub - bounds.event_lb);i++) {
    pattern patt;
    patt.p.push_back(i);
    add_pattern(patt, PRIOR_COUNTS);
    }*/
}

void model::add_sample(pattern* p, double count) {
  p->count += count;
  update_base_case(*p, count);
}
 
void model::add_pattern(pattern p, double count) {
  p.count = count;
  patterns.push_back(p);
  update_base_case(p, count);
}

//FIXME: This function does not depend on any element of model.  Move the constants into the model.
double model::prior_count(unsigned pattern_length) const {
  double prior_position_density = PRIOR_EVENT_DENSITY/(double(p_bounds.ub - p_bounds.lb));
  return pow(prior_position_density, pattern_length)*PRIOR_INTERVAL;
}

//This function is a stub.  But it will work in a limited way.  Ignores mem_constraint.
//FIXME: Add the ability to subdivide existing patterns as events are added until we are within some memory constraint.
/*-Need to determine which patterns should be purged by looking at the older ones (?) and
  a) asking which ones can be written as a product of conditionally independent factors
  b) asking which ones have only occurred once ever and are large.
  Maybe there is some more elegant way of choosing thresholds for "old" and "large".
*/
void model::train(const occurrence &givens) {
  static occurrence prev_givens; //state variablee - last set of givens, so we can determine which are new
  occurrence dup_givens = get_intersection(prev_givens, givens);
  add_pattern(get_pattern(givens), 1.0);
  add_pattern(get_pattern(dup_givens), -1.0); //So that we don't double count
  prev_givens = givens;
}

unsigned model::get_new_visit_id() const {
  return ++current_visit_id;
}

//Returns the size of the sample space containing p
//TODO: This probably should be frequency wrt time not frequency wrt events
double model::sample_size(const pattern& p) {
  return total_num_events - p.total_events_at_creation + 1.0;  //might be wrong - time based not event based?
}

//Returns probability of getting this pattern but not any super-pattern
double model::local_prob(const pattern& p) const {
  return (p.count + prior_count(p.p.size()))/(sample_size(p) + prior_count(0));  //this could be made faster but this is elegant
}

bool model::is_match(const occurrence& occ, const pattern& p, unsigned t_abs) const {
  return is_single_valued(get_union(occ, get_occurrence(p, t_abs)));
}

class term {
public:
  term(const occurrence& occ, double prob) { this->occ = occ; this->prob = prob; }
  term() { prob = 1.0; }
  occurrence occ;
  double prob;
};

//Find the top level terms necessary to find P(occ)
void model::find_terms(const occurrence& occ, list<term> &terms, pattern& patt, unsigned t_abs, unsigned visit_id) const {
  if(already_visited(patt, visit_id, t_abs) || !match(occ, patt, t_abs))
    return;
  
  mark_visited(patt, visit_id, t_abs);
  
  list<patt_link> super_patterns;
  get_super_patterns(occ, patt, supers);
  
  //Based on the super patterns, get the super terms.
  list<term> new_terms;
  //Add onto the list this pattern, locally.
  new_terms.push_back(term(get_occurrence(patt, t_abs), local_prob(occ, patt, t_abs)));

  //Get the terms corresponding to super patterns of patt.
  for(auto p_link = super_patterns.begin();p_link != super_patterns.end();p_link++) {
    find_terms(occ, new_terms, *(p_link.p_patt), t_abs + p_link.t_offset, visit_id);
  }
  
  //For each pair of new terms:
  for(auto p_term_a = new_terms.begin();p_term_a != new_terms.end();p_term++) {
    auto p_term_b = p_term_a;
    p_term_b++;
    while(p_term_b != new_terms.end()) {
      bool remove_term_b = false;
      occurrence term_a_intersection = get_intersection(p_term_a->occ, occ);
      occurrence term_b_intersection = get_intersection(p_term_b->occ, occ);
      
      if(term_a_intersection == term_b_intersection) {
	//Take the sum of probability for terms which are the same in the occ
	//This is because the patterns are mutually exclusive and both contribute part of
	//the total probability for the intersection occurrence.
	p_term_a->prob += p_term_b->prob;
	remove_term_b = true;
      } else if(is_sub_occurrence(p_term_a->occ, p_term_b->occ)) {
	//Remove terms which are a strict sub of another term (not equal) in the occ.
	//This is because the smaller one's probability would be divided out of the
	//total probability for the multiplied terms anyway.
	remove_term_b = true;
      }
      
      if(remove_term_b) {
	p_term_b = new_terms.erase(p_term_b);
      } else
	p_term_b++;
      }
    }
  }
  
  //Append new_terms to terms
  terms.splice(terms.end(), new_terms);
}

double model::prob(const occurrence& occ) {
  //Find the terms necessary to make occ
  list<term> terms;
  find_terms(occ, term, base_level_pattern, 0, get_new_visit_id());

  //Take the conditional product of terms which are different in the occ
  term current;
  current.prob = 1.0;

  //FIXME not done
  //For each term, divide out the overlap between this term and the last by calling prob recursively
  for(auto p_term = terms.begin();p_term != terms.end();p_term++) {
    current.prob *= p_term->prob;
    occurrence intersection_occ = get_intersection(current.occ, p_term->occ);
    if(!intersection_occ.empty())
      current.prob /= prob(intersection_occ);
  }

  //Return the product
  return current.prob;
}

//Find conditional probability
/*
Notes on future optimizations:
For a set of givens ABC, and we want ___X|ABC, we will find:
some patterns that have an element where X goes, and
some patterns that do not.
The ones that do not will cancel out, since they will be members of both ABX and ABC.  This applies to the cross terms too.
That means that to find P(X|ABC), we need to make a list of the biggest terms we can make that have an element where X goes.
So if the patterns are:
A_C, AB_X, BCX, BCR, B_XQ, XQR
Then the terms for ABCX are:
A_C, AB_X, BCX
And the terms for ABC are:
A_C, AB, BC
And we form the product P(X|ABC) = ( P(AB_X)P(BCX)P(A_C)/P(B_X)P(A)P(C) ) / ( P(A_C)P(AB)P(BC)/P(A)P(B)P(C) )
But again we don't need the cross terms that appear for both (the ones that don't have any elements where X goes).
So then we have P(X|ABC) = ( P(AB_X)P(BCX)/P(B_X) ) / ( P(AB)P(BC)/P(B) )
This can be simplified to
P(X|ABC) = P(X|AB_)P(X|BC)/P(X|B)
Now if you look at the original list for ABCX, you can see that the terms to find P(ABCX), counting only terms that
have X as an element, would be
P(ABCX) = P(AB_X)P(BCX)/P(B_X)
The one to one relationship between these last two is highly suggestive.

If we use Bayes's theorem repeatedly to find this probability, we will find that it gives
P'(X|Gn) = P(X)( P(Gn|X)/P(Gn) ) = P(X)( P(Gn & X)/P(Gn)P(X) )
So,
P(X|G) = P(X|G1)P(X|G2)P..P(X|Gn)/P(X)^(n - 1)
Which is almost the same, but it assumes that the only cross term between the givens is X.  In other words they are
all independent.

Now by because all the patterns in our term set have no supers in the givens to operate on, it doesn't matter if
we ask about P(term) in context or out of context since the answer is the same.  But that is not the case for
the cross terms.  For the cross terms if we ask in context we get a very compact answer.
P(ABC) = P(AB)P(BC)/P(B)
but in context, P(B) = P(AB) + P(BC), so
P(ABC) = P(AB)P(BC)/( P(AB) + P(BC) ) = 1/( 1/P(AB) + 1/P(BC) )
which is the same formula that you get for resistors in parallel, again highly suggestive.  One can imagine parallel
probability flows.

UPDATE: this function is actually broken, because the first order prob() function does not take into account that
two of its answers may be contradictory, and therefore if you take the sum of conditional_prob for every
answer for a given tick they might not add up to one.  That's ok; we can call the get_first_order_completions
function instead which has a cumulative_prob method that you can use to normalize the answer and adjust the
configuration space with the new information.
*/
double model::conditional_prob(const occurrence& occ, const occurrence& givens) {
  return prob(get_union(occ, givens))/prob(givens);
}

//Return a completion_set containing information about all the first order events with their probability
//at a particular absolute time.
void model::get_first_order_completions(occurrence& occ, unsigned t_abs, completion_set& result_set) {
  
}

