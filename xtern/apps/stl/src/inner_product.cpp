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
// Name: inner_product
// Function: Returns the result of accumulating init with the inner products of the
// pairs formed by the elements of two ranges starting at first1 and first2.
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
int RandomNumber () { return (std::rand()%10); }

unsigned int data_size = 0;


//std::vector<int> myvector(1000*1000*600);
//std::vector<int> second(1000*1000*600);
std::vector<int> myvector(data_size);
std::vector<int> second(data_size);
//std::vector<int> myvector(1000);
//std::vector<int> result(1000);

int main(int argc, char * argv[])
{
    SET_INPUT_SIZE(argc, argv[1])
    myvector.resize(data_size);
    second.resize(data_size);

    struct timeval start, end;
    int init = 10;
    fprintf(stderr, "omp num threads %d\n", omp_get_max_threads());
    //generate (myvector.begin(), myvector.end(), UniqueNumber, __gnu_parallel::sequential_tag());
//    std::srand(SEED);
//    __gnu_parallel::generate (myvector.begin(), myvector.end(), RandomNumber, __gnu_parallel::sequential_tag());
 
//    std::srand(SEED);
//    __gnu_parallel::generate (second.begin(), second.end(), RandomNumber, __gnu_parallel::sequential_tag());
//    memcpy(&second[0], &myvector[0], myvector.size() * sizeof(int));


    //__gnu_parallel::partial_sum(myvector.begin(), myvector.end(), result.begin());
    
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
    __gnu_parallel::inner_product(myvector.begin(), myvector.end(), second.begin(), init);
    gettimeofday(&end, NULL);
    fprintf(stderr, "real %.3f\n", ((end.tv_sec * 1000000 + end.tv_usec)
          - (start.tv_sec * 1000000 + start.tv_usec)) / 1000000.0);


    return 0;
}
