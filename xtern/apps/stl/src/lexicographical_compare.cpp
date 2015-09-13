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
// Name: lexicographical_compare
// Function: Returns true if the range [first1,last1) compares 
// lexicographically less than the range [first2,last2).
// Test case: Worst case (All elements)
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <omp.h>
#include <iostream>
#include <parallel/numeric>
#include "microbench.h"
#include <string.h>

// function generator:
char RandomNumber () { return (std::rand()%100); }

unsigned int data_size = 0;

//std::vector<int> myvector(1000*1000*1000);
//std::vector<int> result(1000*1000*1000);
std::vector<int> myvector(data_size);
std::vector<int> result(data_size);
//std::vector<int> myvector(10);
//std::vector<int> result(10);

int main(int argc, char * argv[])
{
    SET_INPUT_SIZE(argc, argv[1])
    myvector.resize(data_size);
    result.resize(data_size);

//    bool a = false;
    struct timeval start, end;
    fprintf(stderr, "omp num threads %d\n", omp_get_max_threads());
//    std::srand(SEED);
//    __gnu_parallel::generate (myvector.begin(), myvector.end(), RandomNumber, __gnu_parallel::sequential_tag());
//    std::srand(SEED);
//    __gnu_parallel::generate (result.begin(), result.end(), RandomNumber, __gnu_parallel::sequential_tag());
//    memcpy(&result[0], &myvector[0], myvector.size() * sizeof(int));


    //generate (myvector.begin(), myvector.end(), UniqueNumber, __gnu_parallel::sequential_tag());
//    __gnu_parallel::generate (myvector.begin(), myvector.end(), RandomNumber);
//    __gnu_parallel::generate (result.begin(), result.end(), RandomNumber);

    
//    std::cout << "myvector contains:";
//    for (std::vector<int>::iterator it=myvector.begin(); it != myvector.end(); ++it)
//      std::cout << ' ' << *it;
//    std::cout << '\n';
//
//    std::cout << "result contains:";
//    for (std::vector<int>::iterator it=result.begin(); it != result.end(); ++it)
//      std::cout << ' ' << *it;
//    std::cout << '\n';
 
    gettimeofday(&start, NULL);
    __gnu_parallel::lexicographical_compare(myvector.begin(), myvector.end(), result.begin(), result.end());
    gettimeofday(&end, NULL);
    fprintf(stderr, "real %.3f\n", ((end.tv_sec * 1000000 + end.tv_usec)
          - (start.tv_sec * 1000000 + start.tv_usec)) / 1000000.0);
    
//    std::cout << a << '\n';
//    if(!a)
//        std::cout << "equal" << '\n';

    return 0;
}
