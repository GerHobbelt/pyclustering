/*!

@authors Andrei Novikov (pyclustering@yandex.ru)
@date 2014-2020
@copyright GNU Public License

@cond GNU_PUBLIC_LICENSE
    pyclustering is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    pyclustering is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
@endcond

*/


#pragma once


#include <algorithm>
#include <cstddef>
#include <functional>
#include <future>
#include <vector>

#include <pyclustering/parallel/spinlock.hpp>


#if defined(WIN32) || (_WIN32) || (_WIN64)
#define PARALLEL_IMPLEMENTATION_PPL
#else
#define PARALLEL_IMPLEMENTATION_ASYNC_POOL
#endif


#if defined(PARALLEL_IMPLEMENTATION_PPL)
#include <ppl.h>
#endif


namespace pyclustering {

namespace parallel {


/* Pool of threads is used to prevent overhead in case of nested loop */
static const std::size_t AMOUNT_HARDWARE_THREADS = std::thread::hardware_concurrency();
static const std::size_t AMOUNT_THREADS = (AMOUNT_HARDWARE_THREADS > 1) ? (AMOUNT_HARDWARE_THREADS - 1) : 0;


/*!

@brief Parallelizes for-loop using all available cores.
@details `parallel_for` uses PPL in case of Windows operating system and own implemention that is based
          on pure C++ functionality for concurency such as `std::future` and `std::async`.

Advanced uses might use one of the define to use specific implementation of the `parallel_for` loop:
1. PARALLEL_IMPLEMENTATION_ASYNC_POOL - own parallel implementation based on `std::async` pool.
2. PARALLEL_IMPLEMENTATION_NONE       - parallel implementation is not used.
3. PARALLEL_IMPLEMENTATION_PPL        - parallel PPL implementation (windows system only).
4. PARALLEL_IMPLEMENTATION_OPENMP     - parallel OpenMP implementation.

@param[in] p_start: initial value for the loop.
@param[in] p_end: final value of the loop - calculations are performed until current counter value is less than final value `i < p_end`.
@param[in] p_step: step that is used to iterate over the loop.
@param[in] p_task: body of the loop that defines actions that should be done on each iteration.

*/
template <typename TypeIndex, typename TypeAction>
void parallel_for(const TypeIndex p_start, const TypeIndex p_end, const TypeIndex p_step, const TypeAction & p_task) {
#if defined(PARALLEL_IMPLEMENTATION_ASYNC_POOL)
#if defined(NEGATIVE_STEPS)
    const TypeIndex interval_length = (p_end > p_start) ? (p_end - p_start) : (p_start - p_end);

    TypeIndex interval_step = (interval_length / p_step) / (static_cast<TypeIndex>(AMOUNT_THREADS) + 1);
    if ((p_step > TypeIndex(0) && interval_step < p_step) || 
        (p_step < TypeIndex(0) && interval_length > p_step)) {
        interval_step = p_step;
    }

    std::size_t amount_threads = static_cast<std::size_t>(interval_length / interval_step);   /* amount of data might be less than amount of threads. */
    if (amount_threads > AMOUNT_THREADS) {
        amount_threads = AMOUNT_THREADS;
    }
    else if (amount_threads > 0) {
        amount_threads--;   /* current thread is also considered. */
    }
#else
    /*

    Microsoft `concurrency::parallel_for` implementation does not support negative step. The cite from the documentation about `concurrency::parallel_for`.
    The loop iteration must be forward. The parallel_for algorithm throws an exception of type std::invalid_argument if the _Step parameter is less than 1.

    Therefore let's support the same behavior.

    */

    const TypeIndex interval_length = p_end - p_start;

    TypeIndex interval_step = (interval_length / p_step) / (static_cast<TypeIndex>(AMOUNT_THREADS) + 1);
    if (interval_step < p_step)  {
        interval_step = p_step;
    }

    std::size_t amount_threads = static_cast<std::size_t>(interval_length / interval_step);
    if (amount_threads > AMOUNT_THREADS) {
        amount_threads = AMOUNT_THREADS;
    }
    else if (amount_threads > 0) {
        amount_threads--;   /* current thread is also considered. */
    }
#endif

    TypeIndex current_start = p_start;
    TypeIndex current_end = p_start + interval_step;

    std::vector<std::future<void>> future_storage(amount_threads);

    for (std::size_t i = 0; i < amount_threads; ++i) {
        const auto async_task = [&p_task, current_start, current_end, p_step](){
            for (TypeIndex i = current_start; i < current_end; i += p_step) {
                p_task(i);
            }
        };

        future_storage[i] = std::async(std::launch::async, async_task);

        current_start = current_end;
        current_end += interval_step;
    }

    for (TypeIndex i = current_start; i < p_end; i += p_step) {
        p_task(i);
    }

    for (auto & feature : future_storage) {
        feature.get();
    }
#elif defined(PARALLEL_IMPLEMENTATION_PPL)
    concurrency::parallel_for(p_start, p_end, p_step, p_task);
#elif defined(PARALLEL_IMPLEMENTATION_OPENMP)
    #pragma omp parallel for
    for (TypeIndex i = p_start; i < p_end, i += p_step) {
        p_task(i);
    }
#else
    for (std::size_t i = p_start; i < p_end; i += p_step) {
        p_task(i);
    }
#endif
}


/*!

@brief Parallelizes for-loop using all available cores.
@details `parallel_for` uses PPL in case of Windows operating system and own implemention that is based
on pure C++ functionality for concurency such as `std::future` and `std::async`.

Advanced uses might use one of the define to use specific implementation of the `parallel_for` loop:
1. PARALLEL_IMPLEMENTATION_ASYNC_POOL - own parallel implementation based on `std::async` pool.
2. PARALLEL_IMPLEMENTATION_NONE       - parallel implementation is not used.
3. PARALLEL_IMPLEMENTATION_PPL        - parallel PPL implementation (windows system only).
4. PARALLEL_IMPLEMENTATION_OPENMP     - parallel OpenMP implementation.

@param[in] p_start: initial value for the loop.
@param[in] p_end: final value of the loop - calculations are performed until current counter value is less than final value `i < p_end`.
@param[in] p_task: body of the loop that defines actions that should be done on each iteration.

*/
template <typename TypeIndex, typename TypeAction>
void parallel_for(const TypeIndex p_start, const TypeIndex p_end, const TypeAction & p_task) {
    parallel_for(p_start, p_end, std::size_t(1), p_task);
}


/*!

@brief Parallelizes for-each-loop using all available cores.
@details `parallel_each` uses PPL in case of Windows operating system and own implemention that is based
          on pure C++ functionality for concurency such as `std::future` and `std::async`.

Advanced uses might use one of the define to use specific implementation of the `parallel_for_each` loop:
1. PARALLEL_IMPLEMENTATION_ASYNC_POOL - own parallel implementation based on `std::async` pool.
2. PARALLEL_IMPLEMENTATION_NONE       - parallel implementation is not used.
3. PARALLEL_IMPLEMENTATION_PPL        - parallel PPL implementation (windows system only).
4. PARALLEL_IMPLEMENTATION_OPENMP     - parallel OpenMP implementation.

@param[in] p_begin: initial iterator from that the loop starts.
@param[in] p_end: end iterator that defines when the loop should stop `iter != p_end`.
@param[in] p_task: body of the loop that defines actions that should be done for each element.

*/
template <typename TypeIter, typename TypeAction>
void parallel_for_each(const TypeIter p_begin, const TypeIter p_end, const TypeAction & p_task) {
#if defined(PARALLEL_IMPLEMENTATION_ASYNC_POOL)
    const std::size_t interval_length = std::distance(p_begin, p_end);
    const std::size_t step = std::max(interval_length / (AMOUNT_THREADS + 1), std::size_t(1));

    std::size_t amount_threads = static_cast<std::size_t>(interval_length / step);
    if (amount_threads > AMOUNT_THREADS) {
        amount_threads = AMOUNT_THREADS;
    }
    else if (amount_threads > 0) {
        amount_threads--;   /* current thread is also considered. */
    }

    auto current_start = p_begin;
    auto current_end = p_begin + step;

    std::vector<std::future<void>> future_storage(amount_threads);

    for (std::size_t i = 0; i < amount_threads; ++i) {
        auto async_task = [&p_task, current_start, current_end](){
            for (auto iter = current_start; iter != current_end; ++iter) {
                p_task(*iter);
            }
        };

        future_storage[i] = std::async(std::launch::async, async_task);

        current_start = current_end;
        current_end += step;
    }

    for (auto iter = current_start; iter != p_end; ++iter) {
        p_task(*iter);
    }

    for (auto & feature : future_storage) {
        feature.get();
    }
#elif defined(PARALLEL_IMPLEMENTATION_PPL)
    concurrency::parallel_for_each(p_begin, p_end, p_task);
#else
    for (auto iter = p_begin; iter != p_end; ++iter) {
        p_task(*iter);
    }
#endif
}


/*!

@brief Parallelizes for-each-loop using all available cores.
@details `parallel_each` uses PPL in case of Windows operating system and own implemention that is based
          on pure C++ functionality for concurency such as `std::future` and `std::async`.

@param[in] p_container: iterable container that should be processed.
@param[in] p_task: body of the loop that defines actions that should be done for each element.

*/
template <typename TypeContainer, typename TypeAction>
void parallel_for_each(const TypeContainer & p_container, const TypeAction & p_task) {
    parallel_for_each(std::begin(p_container), std::end(p_container), p_task);
}


}

}