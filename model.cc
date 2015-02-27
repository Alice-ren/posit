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

//Model node and associated functions
class model_node;
class mn_link {
public:
  mn_link(model_node* p_node, int t_offset) { this->p_node = p_node; this->t_offset = t_offset; }
  model_node* p_node;
  int t_offset;
};

class model_node {
public:
  
  bool already_visited(unsigned visit_id, int t_abs);
  void mark_visited(unsigned visit_id, int t_abs);
  void get_super_patterns(const occurrence& target_occ, list<mn_link> &supers);
  void reset_super_link(pattern* link_patt, int t_offset);
  void reset_sub_link(pattern* link_patt, int t_offset);
  
  pattern patt;
  double count;
  unsigned last_visit_id;
  list<unsigned> visited_t_abs; //for this visit id.  We only want to visit each pattern one time with a particular t_abs.
  
  //sub, super patterns
  list<patt_link> super_links;
  list<patt_link> sub_links;
};

bool model_node::already_visited(unsigned visit_id, int t_abs) {
  return (last_visit_id == visit_id && find(visited_t_abs.begin(), visited_t_abs.end(), t_abs) != visited_t_abs.end());
}

void model_node::mark_visited(unsigned visit_id, int t_abs) {
  if(last_visit_id != visit_id)
    visited_t_abs.clear();

  last_visit_id = visit_id;
  visited_t_abs.push_back(t_abs);
}

void model_node::get_super_patterns(const occurrence& target_occ, list<mn_link> &supers) {
  if(patt.p.empty()) { //base case pattern, contains a super for the givens at every possible t
    supers.clear();
    for(auto p_e = target_occ.cbegin();p_e != target_occ.cend();p_e++) {
      for(auto p_link = super_links.cbegin(); p_link != super_links.cend();p_link++) {
	supers.push_back(mn_link(p_link->p_node, p_e->t));
      }
    }
  } else {
    supers = super_links;
  }
}

void model_node::reset_super_link(model_node* link_node, int t_offset) {
  for(auto p_link = super_links.begin();p_link != super_links.end();p_link++) {
    if(p_link->p_patt == link_patt && p_link->t_offset == t_offset)
      return; //already here
  }

  super_links.push_back(mn_link(link_node, t_offset));
}

void model_node::reset_sub_link(model_node* link_node, int t_offset) {
  for(auto p_link = sub_links.begin();p_link != sub_links.end();p_link++) {
    if(p_link->p_node == link_node && p_link->t_offset == t_offset)
      return; //already here
  }

  sub_links.push_back(mn_link(link_node, t_offset));
}

//Member functions for the model class
model::model(unsigned memory_constraint) {
  this->memory_constraint = memory_constraint;
  total_num_events = 0.0;

  PRIOR_EVENT_DENSITY = 1.0;
  PRIOR_INTERVAL = 1.0;
}

void model::relink_common_subsections(const occurrence& occ1, const occurrence& occ2) {
  list<pattern> new_patts;
  convolute(occ1, occ2, new_patts);
  for(auto pn = new_patts.begin();pn != new_patts.end();pn++) {
    relink(root, get_occurrence(*pn, 0));
  }
}

//Performs a no-touch relink (in other words calling this function does not affect returned statistics)
void model::relink(const occurrence& occ, bool only_new = true) {
  list<mn_link> supers;
  list<mn_link> subs;
  list<model_node*> siblings;
  model_node* p_node;
  find_context(root, occ, supers, subs, siblings, p_node, get_next_visit_id());

  if(p_node == NULL) {
    p_node = new model_node;
    p_node->patt = get_pattern(occ);
    p_node->count = 0.0;
    p_node->last_visit_id = MAX_UINT;
  } else if(only_new)
    return; //This is an existing pattern and we are only relinking new ones

  //Set the super, sub links
  p_patt->super_links = supers;
  p_patt->sub_links = subs;

  //Make sure links in subs, supers back to this pattern are set correctly
  for(auto p_link = supers.begin();p_link != supers.end();p_link++)
    reset_sub_link(p_link->p_node, p_node, -(p_link->t_offset));
  
  for(auto p_link = subs.begin();p_link != subs.end();p_link++)
    reset_super_link(p_link->p_node, p_node, -(p_link->t_offset));

  //Check self and sibling patterns for common subsections
  relink_common_subsections(occ, occ);
  for(auto p_sib = siblings.begin();p_sib != siblings.end();p_sib++)
    relink_common_subsections(get_occurrence(p_sib->patt, 0), occ);
}

//p_match is a pointer to the pattern representing occ, unless occ would be a new pattern in which it is NULL.
//FIXME: not done, does not find offsets
typedef enum {SUB, SUPER, SIBLING, IDENTITY, NONE} patt_relation;
patt_relation model::find_context(model_node& p_current,
				  const occurrence& target_occ,
				  list<mn_link> &supers,
				  list<mn_link> &subs,
				  list<model_node*> &siblings,
				  model_node* &p_match,
				  int t_abs,
				  unsigned visit_id) {
  if(target_occ.empty())
    return NONE;
  
  if(p_current.already_visited(visit_id, t_abs) || !is_compatible(target_occ, p_current, t_abs))
    return NONE;
  
  p_current.mark_visited(visit_id, t_abs);

  occurrence current_occ = get_occurrence(p_current.patt, t_abs);

  //if this pattern is equal to the target, then set the p_match reference and call on the supers
  if(current_occ == target_occ) {
    list<mn_link> super_patterns;
    p_current.get_super_patterns(target_occ, super_patterns);
    for(auto p_link = super_patterns.begin();p_link != super_patterns.end();p_link++) {
      find_context(*(p_link->p_node), target_occ, supers, subs, siblings, p_match, t_abs + p_link->t_offset, visit_id);
    }

    p_match = &p_current;
    return IDENTITY;
  }

  //if this pattern is a strict super of the target, then add it to the supers list and return
  if(is_sub_occurrence(current_occ, target_occ)) {
    supers.push_back(mn_link(&p_current, t_abs - target_occ[0].t));
    return SUPER;
  }
  
  //if this pattern is a sub of the target, then call on the supers and add to the subs list if not the sub of a sub
  if(is_sub_occurrence(target_occ, current_occ)) {
    bool sub_of_a_sub = false;
    list<mn_link> super_patterns;
    p_current.get_super_patterns(target_occ, super_patterns);
    for(auto p_link = super_patterns.begin();p_link != super_patterns.end();p_link++) {
      if(find_context(*(p_link->p_node), target_occ, supers, subs, siblings, p_match, t_abs + p_link->t_offset, visit_id) == SUB)
	sub_of_a_sub = true;
    }
    if(!sub_of_a_sub)
      subs.push_back(mn_link(&p_current, t_abs - target_occ[0].t));
    
    return SUB;
  }
  
  //if this pattern is compatible with the target, with a non-empty intersection, but neither a sub or super,
  //nor the pattern itself, then add to the list of siblings.
  if(!get_intersection(target_occ, current_occ).empty()) {
    //Each super of a sibling could be: another sibling, or a super of the target, or nothing.
    //If the super is a sup of the target, or a sib of the target, we would not want to add this pattern to the sibling list.
    bool sub_of_a_sib_or_super = false;
    list<mn_link> super_patterns;
    p_current.get_super_patterns(target_occ, super_patterns);
    for(auto p_link = super_patterns.begin();p_link != super_patterns.end();p_link++) {
      patt_relation sup_relation = find_context(*(p_link->p_node), target_occ, supers, subs, siblings, p_match, t_abs + p_link->t_offset, visit_id);
      if(sup_relation == SIBLING || sup_relation == SUPER)
	sub_of_a_sib_or_super = true;
    }
    
    if(!sub_of_a_sib_or_super)
      siblings.push_back(&p_current);
    
    return SIBLING;
  }
}

//Severs a link and abstracts away the sub pattern.
//Removes events unique to the sub from the super.
//Preserves counts for queries that do not cross the link boundary.
//This DOES affect returned statistics.
void model::sever_link(pattern &patt, sub_link_index) {
  //FIXME finish this
}

//optimize tree to within memory constraints.
//FIXME: figure out how to balance between memory spent on the apex pattern vs memory spent on the rest
void model::optimize(const pattern& root, unsigned memory_constraint) {
  //FIXME finish this
}

//Train the pattern tree given some new data
//This DOES affect returned statistics.
void model::train(const event& e) {
  //Manually insert an event to the end of the training set and link it to a base case pattern
  
  //make sure common subsections exist
  relink_common_subsections(apex, apex);

  //relink the apex pattern (without modifying counts anywhere)
  //FIXME: this does lots of extra work -really only the portions involving the new event need to be relinked.
  relink(apex.occ, false);
  
  //optimize to within memory_constraint.  This DOES affect returned statistics.
  optimize(memory_constraint);
}
 
double model::prior_count(unsigned pattern_length) const {
  double prior_position_density = PRIOR_EVENT_DENSITY/(double(p_bounds.ub - p_bounds.lb));
  return pow(prior_position_density, pattern_length)*PRIOR_INTERVAL;
}

//FIXME: if we ever run this algorithm in parallel there will have to be some more
//sophisticated method than this for visiting each node once.  It would be once
//per thread not just once.  This also means we cannot change the visit id inside
//any recursive function that is crawling the tree.
unsigned model::get_new_visit_id() {
  return ++current_visit_id;
}

//FIXME: for all below functions, pattern& has been replaced by model_node&
//Returns true if the pattern and the occurrence are not mutually exclusive
bool model::is_compatible(const occurrence& occ, const pattern& p, int t_abs) const {
  return is_single_valued(get_union(occ, get_occurrence(p, t_abs)));
}

//Returns the size of the sample space containing p
//FIXME: sample size is now the same for all patterns, rewrite this
double model::sample_size(const pattern& p) {
  return total_num_events - p.total_events_at_creation + 1.0;  //might be wrong - time based not event based?
}

//Returns probability of getting this pattern but not any super-pattern
double model::local_prob(const pattern& p) const {
  return (p.count + prior_count(p.p.size()))/(sample_size(p) + prior_count(0));  //this could be made faster but this is elegant
}

//Returns the total probability of getting this occ - but only works out of context
//FIXME: explain why this works.  Negative counts and diamond subpatterns and all.
double model::global_prob(const occurrence& occ, const pattern& patt, int t_abs, unsigned visit_id) {
  if(visit_id == UINT_MAX)
    visit_id = get_new_visit_id();
  
  if(already_visited(patt, visit_id, t_abs) || !is_compatible(occ, patt, t_abs))
    return;
  
  mark_visited(patt, visit_id, t_abs);

  double prob = 0.0;
  if(is_sub_occurrence(get_occurrence(patt, t_abs), occ)) //This pattern is equal or a superset of occ, add the probability
    prob += local_prob(patt);

  //Add the probability of the super patterns
  list<patt_link> super_patterns;
  get_super_patterns(occ, patt, supers);
  for(auto p_link = super_patterns.begin();p_link != super_patterns.end();p_link++)
    prob += global_prob(occ, *(p_link.p_patt), t_abs + p_link->t_offset, visit_id);

  return prob;
}

//Find the top level terms necessary to find P(occ)
void model::find_terms(const occurrence& occ, list<occurrence> &terms, pattern& patt, int t_abs, unsigned visit_id) const {
  if(visit_id == UINT_MAX)
    visit_id = get_new_visit_id();
  
  if(already_visited(patt, visit_id, t_abs) || !is_compatible(occ, patt, t_abs))
    return;
  
  mark_visited(patt, visit_id, t_abs);
  
  list<patt_link> super_patterns;
  get_super_patterns(occ, patt, supers);
  
  //Based on the super patterns, get the super terms.
  list<occurrence> new_terms;
  //Add onto the list this pattern, locally.
  new_terms.push_back(get_occurrence(patt, t_abs));

  //Get the terms corresponding to super patterns of patt.
  for(auto p_link = super_patterns.begin();p_link != super_patterns.end();p_link++) {
    find_terms(occ, new_terms, *(p_link.p_patt), t_abs + p_link->t_offset, visit_id);
  }
  
  //For each pair of new terms:
  for(auto p_term_a = new_terms.begin();p_term_a != new_terms.end();p_term++) {
    auto p_term_b = p_term_a;
    p_term_b++;  //Don't compare the term with itself
    while(p_term_b != new_terms.end()) {
      if(is_sub_occurrence(p_term_a->occ, p_term_b->occ)) {
	//Remove terms which are a strict sub of another term (not equal) in the occ.
	//This is because the smaller one's probability would be divided out of the
	//total probability for the multiplied terms anyway.
	p_term_b = new_terms.erase(p_term_b);
      } else
	p_term_b++;
    }
  }
  
  //Append new_terms to terms
  terms.splice(terms.end(), new_terms);
}

double model::prob(const occurrence& occ) {
  //Find the terms necessary to make occ
  list<occurrence> terms;
  find_terms(occ, terms);

  //Take the conditional product of terms which are different in the occ
  occurrence current_occ;
  double current_prob = 1.0;

  //For each term, divide out the overlap between this term and the last by calling prob recursively
  for(auto p_term = terms.begin();p_term != terms.end();p_term++) {
    current_prob *= global_prob(*p_term);
    occurrence intersection_occ = get_intersection(current_occ, *p_term);

    //Divide out the probability of the common part, since we are assuming that our patterns are
    //coniditionally independent wrt their common parts.  Do it by calling this function recursively.
    if(!intersection_occ.empty()) 
      current-prob /= prob(intersection_occ);
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

//Since the calling code does not a lot of information about the internal state of the mode, we have to make
//some decisions here about which time tick to return the completions for.  To do that we collect
//information about the number of patterns which have information about each tick.
//We also want to know what the potential events are at that tick, so that we can ask for the probability.
//This function supplies both those things.  I'm calling it a "heuristic" function, where "heuristic"
//is a latin word meaning "really I'm just guessing"
void model::get_completion_heuristics(const occurrence& occ,
				      map<int, double> total_prob_heuristic, //Adds up probability in a way that strictly speaking makes no sense
				      map<int, vector<event> > events_to_return_heuristic,
				      const pattern& patt,
				      int t_abs,
				      unsigned visit_id) {
  if(already_visited(patt, visit_id, t_abs) || !match(occ, patt, t_abs))
    return;
  
  mark_visited(patt, visit_id, t_abs);
  
  list<patt_link> super_patterns;
  get_super_patterns(occ, patt, supers);
  
  //Add heuristics for this pattern, locally.
  occurrence patt_occ = get_occurrence(patt, t_abs);
  for(auto p_event = patt_occ.begin();p_event != patt_occ.end();p_event++) {
    total_prob_heuristic[p_event->t] += local_prob(patt);
    events_to_return_heuristic[p_event->t].push_back(*p_event);
  }

  //Add in the heuristics corresponding to super patterns of patt.
  for(auto p_link = super_patterns.begin();p_link != super_patterns.end();p_link++) {
    get_completion_heruistics(occ, total_prob_heuristic, events_to_return_heuristic, *(p_link->p_patt), t_abs + p_link->t_offset, visit_id);
  }
  
}

//Returns the best t_abs, and the list of explicit events at that t_abs.
unsigned model::pick_best_t_abs(const occurrence& occ,
				 vector<event> &explicit_events) {
  map<int, double> total_prob_heuristic;
  map<int, vector<event> > events_to_return_heuristic;
  get_completion_heuristics(occ, total_prob_heuristic, events_to_return_heuristic, base_level_pattern, 0, get_new_visit_id());
  
  //Pick the best tick to expand and grab the list of events to try
  unsigned best_t_abs = 0;
  double best_total_prob = 0.0;
  for(auto p_tph = total_prob_heuristic.begin();p_tph != total_prob_heuristic.end();p_tph++) {
    if(p_tph->second > best_total_prob) {
      best_t_abs = p_tph->first;
      best_total_prob = p_tph->second;
    }
  }

  explicit_events = events_to_return_heuristic[best_t_abs];
  return best_t_abs;
}

//Return a completion_set containing information about all the first order events with their probability
//at a particular absolute time.
completion_set model::get_first_order_completions(const occurrence& occ) {
  int t_abs;
  vector<event> events;
  t_abs = pick_best_t_abs(occ, events);

  completion_set result_set(this, t_abs);
  for(auto p_event = events.begin();p_event != events.end();p_event++) {
    result_set.add_completion(*p_event, prob(get_union(get_occurrence(e), occ)));
  }

  return result_set;
}

