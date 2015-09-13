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
// Name: search
// Function: Searches the range [first1,last1) for the first 
// occurrence of the sequence defined by [first2,last2), and 
// returns an iterator to its first element, or last1 if no occurrences are found.
// Test case: Worst case (not found)
#include <iostream>     // std::cout
#include <algorithm>    // std::generate
#include <vector>       // std::vector
#include <ctime>        // std::time
#include <cstdlib>      // std::rand, std::srand
#include <parallel/algorithm>
#include "microbench.h"
#include <string.h>

// function generator:
int RandomNumber () { return (std::rand()%100); }

//class generator:
//struct c_unique {
//  int current;
//  c_unique() {current=0;}
//  int operator()() {return ++current;}
//} UniqueNumber;

unsigned int data_size = 0;

//std::vector<int> myvector(1000*1000*1000);
std::vector<int> myvector(data_size);
//std::vector<int> myvector(1000);

#define START_OFF 10000
#define SECOND_SIZE 1000000
#define NOT_IN_ITEM 101

std::vector<int> second(SECOND_SIZE);

int main (int argc, char * argv[]) {
    SET_INPUT_SIZE(argc, argv[1])
    myvector.resize(data_size);

    struct timeval start, end;
//    int items[] = {ITEM, 11, 42, 29, 73, 21, 19, 84, 37, 98, 24, 15, 70, 13, 26, 91, 80, 56, 73, 62};
//    std::vector<int> second(items, items+19);
    fprintf(stderr, "omp num threads %d\n", omp_get_max_threads());
//    std::srand(SEED);
//    __gnu_parallel::generate (myvector.begin(), myvector.end(), RandomNumber, __gnu_parallel::sequential_tag());

//    memcpy(&second[0], &myvector[START_OFF], SECOND_SIZE * sizeof(int));
    for (std::vector<int>::iterator it=second.begin(); it!=second.end(); ++it)
      *it = NOT_IN_ITEM;
  
//    std::cout << "second contains:";
//    for (std::vector<int>::iterator it=second.begin(); it!=second.end(); ++it)
//      std::cout << ' ' << *it;
//    std::cout << '\n';
//    std::cout << *(second.end() - 1) << "\n";

  
    gettimeofday(&start, NULL);
    __gnu_parallel::search (myvector.begin(), myvector.end(), second.begin(), second.end());
    gettimeofday(&end, NULL);
    fprintf(stderr, "real %.3f\n", ((end.tv_sec * 1000000 + end.tv_usec)
          - (start.tv_sec * 1000000 + start.tv_usec)) / 1000000.0);
 
//    std::cout << "Found" << *it << "\n";
//    std::cout << "myvector contains:";
//    std::vector<int>::iterator it;
//    for (it=myvector.begin(); it!=myvector.end() && *it != ITEM; ++it)
//      //std::cout << "\, " << *it;
//        ;
//    for(int i = 0; i < 20; i++)
//        std::cout << *(it+i) << ' ';
//    std::cout << '\n';
   
    return 0;
}
