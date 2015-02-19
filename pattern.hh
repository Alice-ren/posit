/*****
      pattern.hh
      Defines probability spaces over events and sets of events
******/

#include <vector>
#include <list>
#include <iostream>
#include <climits>
#include "occurrence.hh"
using namespace std;

#ifndef PATTERN
#define PATTERN

/*
  A pattern is an abstraction generated from one or more occurrences with matching *absolute* positions, wrt the *relative* time between events in those occurrences.
  In this system our organizing principle is that sequences of events repeat with respect to time and that the probability of an event depends on
  how many similar sequences we have seen before per unit time.
  If we have some pattern ABC with a certain frequency, we are saying that the probability of seeing that pattern is that frequency in *any* context, or that the
  probability of seeing the substring AB is that frequency, but only in the context of __C.
  There is an exception to that rule: if we have a pattern ABC and also a pattern AB, the AB implicitly means (AB & ~ABC).  Because if we had seen ABC we would
  have recorded it that way.  So AB means the probability of AB in any context except for ABC.
  Note that if a pattern for a particular string does not exist in the table, we are not making the statement that the probability is zero.  We are making the
  statement that the pattern can be decomposed into conditionally independent subpatterns (with respect to their overlapping parts).
  When we store a pattern ABC, we are making the statement that ABC is not conditionally independent with respect to any two of its subpatterns.  It is a unique
  pattern that cannot be decomposed into parts.  That means if we have some pattern ABC we cannot use it to estimate the probability of AB outside of the context
  of __C.  In order to do that we have to analyze the input data to the model and generate the smaller patterns as we can.
  But if there is no larger pattern we can use smaller patterns to estimate the probability of the larger one by using the assumption of conditional independence;
  i.e. P(ABC) = P(AB)P(BC)/P(B)
  
*/

class pattern;
typedef struct {
  pattern* p_patt;
  unsigned t_offset;
} patt_link;

typedef struct {
  vector<unsigned> p;
  vector<unsigned> dt;
  double count;
  double total_events_at_creation;
  unsigned last_visit_id;
  list<unsigned> visited_t_abs; //for this visit id.  We only want to visit each pattern one time with a particular t_abs.
  
  //sub, super patterns
  list<patt_link> super_links;
  list<patt_link> sub_links;
} pattern;

bool already_visited(const pattern& p, unsigned visit_id, int t_abs);
void mark_visited(pattern& p, unsigned visit_id, int t_abs);
void get_super_patterns(const occurrence& occ, const pattern& p, list<patt_link> &supers);
void print_pattern(const pattern& p);
void print_patterns(const list<pattern>& patterns);
bool operator==(const pattern& p1, const pattern& p2);

//functions related to patterns and occurrences
occurrence get_occurrence(const pattern& pat, int t_abs);
pattern get_pattern(const occurrence& occ);

//functions related to matches
typedef struct {
  //pattern* p_patt;
  occurrence patt_occ;  //The whole matched pattern, as an occurrence
  occurrence match_occ; //Matched portion of patt_occ
  //event e;
  double local_count; //Count for the matched pattern as stored, not including superpatterns
  double match_global_count; //Count for the matched occurrence, including counts of supersets of the matched occurrence
  double patt_global_count; //Count for the matched pattern, including counts of supersets of the matched pattern.
  //Note: global_count >= local_count always.
  double prob;
} match;

void print_match(const match& match);
void print_matches(const list<match>& matches);

void get_pattern_matches(const occurrence& occ, const pattern& patt, list<patt_link>& matches);
bool get_suboccurrences_matching_pattern(const occurrence &occ, const pattern &patt, list<occurrence> &matches);
double count_suboccurrences_matching_pattern(const occurrence &occ, const pattern &patt);
bool get_occurrence_matches(const occurrence &occ, const pattern &patt, list<match> &matches); //Get all the matches
void set_global_counts(list<match> &matches); //Sets the global count of each match given that we know the local counts
void set_local_counts(list<match> &matches, occurrence oracle_occ); //sets the local count of each match given some occurrence to use to measure the global count
void make_unique(list<match> &matches); //Eliminate duplicate pattern occurrences from this list of matches
void make_unique(list<patt_link> &matches); //Eliminate duplicate pattern occurrences from this list of matches
void consolidate(list<match> &matches); //Eliminate duplicate pattern occurrences from this list of matches, and sum counts
bool match_occurrence_size_descending(const match& m1, const match& m2);
bool match_occurrence_size_ascending(const match& m1, const match& m2);
//bool match_global_count_descending(const match& m1, const match& m2);
bool match_new_event_descending(const match& m1, const match& m2);

#endif
