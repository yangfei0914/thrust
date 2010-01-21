#include <thrust/host_vector.h>
#include <thrust/device_vector.h>
#include <thrust/generate.h>
#include <thrust/unique.h>
#include <thrust/functional.h>
#include <cstdlib>
#include <iostream>
#include <iomanip>

// convert a linear index to a row index
template <typename T>
struct linear_index_to_row_index : public thrust::unary_function<size_t,size_t>
{
    T C; // number of columns
    
    __host__ __device__
    linear_index_to_row_index(T _C) : C(_C) {}

    __host__ __device__
    T operator()(T i)
    {
        return i / C;
    }
};

int main(void)
{
    int R = 5;     // number of rows
    int C = 8;     // number of columns

    // initialize data
    thrust::device_vector<int> array(R * C);
    for (int i = 0; i < array.size(); i++)
        array[i] = rand() % 3;
    
    // allocate storage for row sums and indices
    thrust::device_vector<int> row_sums(R);
    thrust::device_vector<int> row_indices(R);
    
    // compute row sums by summing values with equal row indices
    thrust::unique_copy_by_key(thrust::make_transform_iterator(thrust::counting_iterator<int>(0), linear_index_to_row_index<int>(C)),
                               thrust::make_transform_iterator(thrust::counting_iterator<int>(0), linear_index_to_row_index<int>(C)) + (R*C),
                               array.begin(),
                               row_indices.begin(),
                               row_sums.begin(),
                               thrust::equal_to<int>(),
                               thrust::plus<int>());

    // print data 
    for(size_t i = 0; i < R; i++)
    {
        std::cout << "[";
        for(size_t j = 0; j < C; j++)
            std::cout << std::setw(8) << array[i * C + j] << " ";
        std::cout << "] = " << std::setw(8) << row_sums[i] << "\n";
    }

    return 0;
}

