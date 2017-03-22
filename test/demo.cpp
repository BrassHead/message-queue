/* 

demo - channel.hpp single file library demonstration program

Copyright © 2017 Brian Bray

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <iostream>
using std::cout;
#include <thread>
#include <future>
#include <vector>
using std::vector;

#include "channel.hpp"

using vlong = long long;


static const vlong kilo = 1000;
static const vlong mega = kilo*kilo;
// static const vlong giga = kilo*mega;

// Parallel sieve for prime numbers. Counts the number of primes less than 'n'.
//
// Parameters 
static const vlong n = 1*mega;          // Search up to 'n'
typedef int ptype;                      // An integral type large enough for n
static const int checksize = 1000;      // How many primes to divide per check
static const int ncheckers = 2;         // Number of checkers spawned per layer

// A buncha channels for longs. Potential primes will be streamed through these
// to checker tasks and untimately the accumulator task.

typedef channel<ptype> Potentials;
typedef vector<ptype> Primes;
static Potentials queues[20];

// The generator. Generates potential prime numbers less than n. Except 2

void generator(Potentials &out)
{
    for (ptype i=3; i<n; i+=2) {
        out.send(i);
    }
    for (int nc=0; nc<ncheckers; nc++)
    {
        out.send(0);        // Termination signal
    }
}

// The checker. Passes along potential primes that  

void checker(Primes &tocheck, Potentials &in, Potentials &out)
{
    while (ptype c = in.get()) {
        bool isDef = false;
        for (ptype prime : tocheck) {
            if (prime*prime>c) {
                isDef = true;
                out.send(c);            // Definitively a prime
                break;
            } else if (0==c%prime) {
                isDef = true;
                break;                  // Definitively not a prime
            }
        }
        if (!isDef) {
            out.send(c);                // Keep checking
        }
    }
    out.send(0);                        // Termination
}

// The accumulator. This task does the final counting. It also accumulates
// prime numbers and spawns checkers to act as sieves.

void accumulator()
{
    const int maxLayers=100;
    Primes acc[maxLayers];
    acc[0].reserve(checksize);
    // Spawn the generator as starting input
    // allocate the first primes accumulator
    // while accumulating === while new primes are less than sqrt(n) && 
    //           accumulator not full
    //   get a potential prime
    //   check it against the primes I already have
    //   if its prime, count and add to accumulator (keep order!)
    //   if the accumulator is full, 
    //     spawn a layer of checkers
    //     allocate next accumulator
    // after all the layers are in place, just count the primes coming in.
    // until you get a 0
}


int main(int argc, char *argv[])
{
    cout << "\n\nDemo starting soon[tm] !\n\n";


    return 0;
}