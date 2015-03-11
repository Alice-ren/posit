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

typedef struct {
  int ub; //upper bound
  int lb; //lower bound
} t_bounds;

//An event is a complete piece of data as it is received, not abstracted as a pattern, comprised as a position p
//(which can be thought of as an event type, since we do not assume position shift invariance) taking place at some time t.
typedef struct {
  bool p;
  int t;
} event;

bool operator<(const event& t1, const event& t2);
bool operator==(const event& t1, const event& t2);
bool operator!=(const event& t1, const event& t2);
void print_event(const event& e);

class pattern;
class event_ptr {
public:
  event_ptr(const pattern* p, int t_offset = 0)
  event_ptr(const pattern* p, bool end);
  bool operator++();
  event operator*() const;
  event operator->() const;
private:
  pattern* p_patt;
  unsigned i;
  int t_abs;
};
bool operator!=(const event_ptr& p1, const event_ptr& p2);
bool operator==(const event_ptr& p1, const event_ptr& p2);

class pattern {
public:
  pattern();
  pattern(bool p);
  void append(bool p, unsigned dt);
  event_ptr begin(int offset = 0) const;
  event_ptr end() const;
  int size() const;
  bool empty() const;
  int width() const;
  vector<bool> p;
  vector<unsigned> dt;
};

pattern insert(const pattern& patt, bool p, int t_offset);
void print_pattern(const pattern& p);
void print_patterns(const list<pattern>& patterns);
bool operator==(const pattern& p1, const pattern& p2);
bool operator!=(const pattern& p1, const pattern& p2);
pattern subtract(const pattern &p, int t_offset, const pattern &sub_p); //Returns the events for which event is in p but not in sub_p
pattern subtract(const pattern &p, int t_offset, const pattern &sub_p, int& result_offset); //Returns the events for which event is in p but not in sub_p
bool is_sub(const pattern &p, int t_offset, const pattern &sub_p);
bool is_sub(const pattern &p, int t_offset, const pattern &sub_p, int& result_offset);
bool is_compatible(const pattern& p1, int t_offset, const pattern& p2);
pattern get_xor(const pattern& p1, int t_offset, const pattern& p2);
pattern get_xor(const pattern& p1, int t_offset, const pattern& p2, int& result_offset);
pattern get_intersection(const pattern& p1, int t_offset, const pattern& p2);
pattern get_intersection(const pattern& p1, int t_offset, const pattern& p2, int& result_offset);
pattern get_union(const pattern& p1, int offset, const pattern& p2);
pattern get_union(const pattern& p1, int offset, const pattern& p2, int& result_offset);
bool is_single_valued(const pattern& p);
void convolute(const pattern& p1, const pattern& p2, list<pattern>& result, list<int>& result_offset);
void convolute(const pattern& p1, const pattern& p2, list<pattern>& result);

#endif
