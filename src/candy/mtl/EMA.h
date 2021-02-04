/*************************************************************************************************
EMA -- Copyright (c) 2020, Markus Iser, KIT - Karlsruhe Institute of Technology

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/

#ifndef EMA_H_
#define EMA_H_

//Exponential Moving Average (EMA)
//Implementation of the "robust initialization" like in CaDiCaL by Armin Biere
class EMA {
    double value;
    float alpha, beta;
    unsigned int wait, period;
public:
    EMA(double alpha_) : value(1), alpha(alpha_), beta(1), wait(1), period(1) {}
    void update(double next) { 
        value += beta * (next - value); 

        if (beta > alpha && --wait == 0) {
            wait = period = (2 * period);
            beta *= .5;
            if (beta < alpha) beta = alpha;
        }
    }
    operator double() const { return value; }
};

#endif