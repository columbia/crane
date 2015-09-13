/* Copyright (c) 2013,  Regents of the Columbia University 
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other 
 * materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Parallel mode STL algorithms
// Name: merge
// Function: Combines the elements in the sorted ranges 
// [first1,last1) and [first2,last2), into a new range 
// beginning at result with all its elements sorted.
// Test case: Normal case
#include <iostream>     // std::cout
#include <algorithm>    // std::set_union, std::sort
#include <vector>       // std::vector
#include <iterator>
#include <parallel/algorithm>
#include "microbench.h"


#define DATA_SIZE 1000*1000*500
//#define DATA_SIZE 4

//int RandomNumber () { return (std::rand()%100); }

//class generator:
struct c_unique {
  int current;
  c_unique() {current=1;}
  int operator()() {
    current += 2; 
    return current;
  }
} OddNumber;

struct c_unique_1 {
  int current;
  c_unique_1() {current=0;}
  int operator()() {
    current += 2; 
    return current;
  }
} EvenNumber;

unsigned int data_size = 0;


// @INPUT:
// first: sequential odd number 
// sencond: sequential even number 
// Note: both inputs are sequential to reduce initialization time
//  std::vector<int> first(DATA_SIZE);
//  std::vector<int> second(DATA_SIZE);
//  std::vector<int> v(DATA_SIZE*2);                     
  std::vector<int> first(data_size / 2);
  std::vector<int> second(data_size / 2);
  std::vector<int> v(data_size);                     

int main (int argc, char * argv[]) {
    SET_INPUT_SIZE(argc, argv[1])
    first.resize(data_size / 2);
    second.resize(data_size / 2);
    v.resize(data_size);
     
    struct timeval start, end;
    fprintf(stderr, "omp num threads %d\n", omp_get_max_threads());
//    __gnu_parallel::generate (first.begin(), first.end(), OddNumber, __gnu_parallel::sequential_tag());
//    __gnu_parallel::generate (second.begin(), second.end(), EvenNumber, __gnu_parallel::sequential_tag());


    fprintf(stderr, "initialization done...\n");
//    generate (first.begin(), first.end(), UniqueNumber, __gnu_parallel::sequential_tag());
//    generate (second.begin(), second.end(), UniqueNumber, __gnu_parallel::sequential_tag());
//  
//    sort (first.begin(),first.end(), __gnu_parallel::sequential_tag()); 
//    sort (second.begin(),second.end(), __gnu_parallel::sequential_tag());
  
  
    gettimeofday(&start, NULL);
    __gnu_parallel::merge(first.begin(), first.end(), second.begin(), second.end(), v.begin());
    gettimeofday(&end, NULL);
    fprintf(stderr, "real %.3f\n", ((end.tv_sec * 1000000 + end.tv_usec)
          - (start.tv_sec * 1000000 + start.tv_usec)) / 1000000.0);
  
//    v.resize(it-v.begin()); 
//  
//    std::cout << "The union has " << (v.size()) << " elements:\n";
//    for (std::vector<int>::iterator it=v.begin(); it!=v.end(); ++it)
//      std::cout << ' ' << *it;
//    std::cout << '\n';
  
    return 0;
}
