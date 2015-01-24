
#include "arithmetic.hh"
#include <string>
#include <iostream>
#include <cfloat>
#include <math.h>

using namespace std;

/*
  This library defines a very basic arithmetic coding library.  Look up arithmetic coder on wikipedia to see what that is.  I claim no ownership over the original concept.  Despite how simple an arithmetic coder is in principle, the actual implementation turns out to be pretty hairy and full of tricky corner cases.  In particular the carry_bits state variable is pretty ugly.  I put more work into these couple hundred lines of code than I would readliy admit to.  Note that I used the interval [-.5, .5) instead of [0.0, 1.0) because I felt that it is a bit cleaner.  That is just a cosmetic decision.
  
  Limitations: 
  You cannot rewind or decode out of order.
  Uses the bitfield class and the restrictions of that class carry forward.
  
  Sean Carter 2014-04-12
*/

arithmetic_writer::arithmetic_writer(string filename) : field(filename) {
  lb = -0.5;
  ub = 0.5 - DBL_EPSILON;
  carry_bits = 0;
}

void arithmetic_writer::push_bit(double prob, bool val) {
  double epsilon = prob*(ub - lb);
  epsilon -= fmod(epsilon, DBL_EPSILON);  //Fixme: this may not be necessary.  Revisit later when I am scraping the last 1% of speed optimizations.
  if(val) {
    ub = lb + epsilon - DBL_EPSILON;  //nasty corner case
    flush_bits();
  } else {
    lb += epsilon;
  }
}

void arithmetic_writer::flush_bits() {
  if(ub > 1.0 || lb > 1.0 || ub < -1.0 || lb < -1.0) { cout << "bounds out of bounds: lb=" << lb << " ub=" << ub << endl << flush; }
  while(true) {
    if(lb > 0.0 && ub > 0.0) {
      lb -= 0.25;
      ub -= 0.25;
      field.push_bit(1);
      while(carry_bits > 0) {
	field.push_bit(0);
	carry_bits--;
      }
			
    } else if(lb <= 0.0 && ub <= 0.0) {
      lb += 0.25;
      ub += 0.25;
      field.push_bit(0);
      while(carry_bits > 0) {
	carry_bits--;
	field.push_bit(1);
      }
    } else {
      if((ub - lb) >= 0.25d) {
	//most significant bits do not match - write back the character we're working on and exit.
	break;
      } else {
	carry_bits++;
      }
    }
		
    lb *= 2.0;
    ub *= 2.0;
    ub += DBL_EPSILON; //so that ub is always greater than lb
  }
}

void arithmetic_writer::write_file() {
  flush_bits();
  if(lb <= 0.0 && ub > 0.0) {
    //Corner case: set the lower bound to a value in our interval in which ub and lb are in the same range, to output the last bit and any carry bits
    //Could also have set ub to 0.0 with the same effect.
    lb = 0.0 + DBL_EPSILON;
    flush_bits();
  }
  field.write_file();
}

arithmetic_reader::arithmetic_reader(string filename) : field(filename) {
  ub = 0.5 - DBL_EPSILON;
  lb = -0.5;
  d = -0.5;
  double epsilon = 0.5;
  while(epsilon >= DBL_EPSILON) {
    if(field.pop_bit())
      d += epsilon;
		
    epsilon /= 2.0;
  }
  flush_bits();
}

bool arithmetic_reader::pop_bit(double prob) {
  double epsilon = prob*(ub - lb);
  epsilon -= fmod(epsilon, DBL_EPSILON);  //Fixme: this may not be necessary.  Revisit later when I am scraping the last 1% of speed optimizations.
  if(lb + epsilon > d) {
    ub = lb + epsilon - DBL_EPSILON;  //nasty corner case
    flush_bits();
    return true;
  } else {
    lb += epsilon;
    return false;
  }
}

void arithmetic_reader::flush_bits() {
  if(lb > d || d > ub || d > 1.0 || d < -1.0) { cout << "d out of bounds: d=" << d << endl << flush; }
  while(true) {
    if(lb == 0.0 && ub == 0.0) {
    } else if(lb > 0.0 && ub > 0.0) {
      lb -= 0.25;
      d -= 0.25; //d is between the other two by assumption (checked above)
      ub -= 0.25;
    } else if(lb <= 0.0 && ub <= 0.0) {
      lb += 0.25;
      d += 0.25; //d is between the other two by assumption (checked above)
      ub += 0.25;
    } else{
      //most significant bits do not match
      if((ub - lb) >= 0.25d) {
	break;
      }
    }
		
    //renormalize
    lb *= 2;
    d *= 2;
    ub *= 2;
		
    //feed in new digits in least significant bit
    ub += DBL_EPSILON;
		
    if(field.pop_bit())
      d += DBL_EPSILON;
  }
}

