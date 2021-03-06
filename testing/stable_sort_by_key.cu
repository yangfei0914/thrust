#include <unittest/unittest.h>
#include <thrust/sort.h>
#include <thrust/functional.h>

template <typename T>
struct less_div_10
{
  __host__ __device__ bool operator()(const T &lhs, const T &rhs) const {return ((int) lhs) / 10 < ((int) rhs) / 10;}
};

template <typename T>
struct greater_div_10
{
  __host__ __device__ bool operator()(const T &lhs, const T &rhs) const {return ((int) lhs) / 10 > ((int) rhs) / 10;}
};


template <class Vector>
void InitializeSimpleStableKeyValueSortTest(Vector& unsorted_keys, Vector& unsorted_values,
                                            Vector& sorted_keys,   Vector& sorted_values)
{
    unsorted_keys.resize(9);   
    unsorted_values.resize(9);   
    unsorted_keys[0] = 25;   unsorted_values[0] = 0;   
    unsorted_keys[1] = 14;   unsorted_values[1] = 1; 
    unsorted_keys[2] = 35;   unsorted_values[2] = 2; 
    unsorted_keys[3] = 16;   unsorted_values[3] = 3; 
    unsorted_keys[4] = 26;   unsorted_values[4] = 4; 
    unsorted_keys[5] = 34;   unsorted_values[5] = 5; 
    unsorted_keys[6] = 36;   unsorted_values[6] = 6; 
    unsorted_keys[7] = 24;   unsorted_values[7] = 7; 
    unsorted_keys[8] = 15;   unsorted_values[8] = 8; 
    
    sorted_keys.resize(9);
    sorted_values.resize(9);
    sorted_keys[0] = 14;   sorted_values[0] = 1;    
    sorted_keys[1] = 16;   sorted_values[1] = 3; 
    sorted_keys[2] = 15;   sorted_values[2] = 8; 
    sorted_keys[3] = 25;   sorted_values[3] = 0; 
    sorted_keys[4] = 26;   sorted_values[4] = 4; 
    sorted_keys[5] = 24;   sorted_values[5] = 7; 
    sorted_keys[6] = 35;   sorted_values[6] = 2; 
    sorted_keys[7] = 34;   sorted_values[7] = 5; 
    sorted_keys[8] = 36;   sorted_values[8] = 6; 
}


template <class Vector>
void TestStableSortByKeySimple(void)
{
    typedef typename Vector::value_type T;

    Vector unsorted_keys, unsorted_values;
    Vector   sorted_keys,   sorted_values;

    InitializeSimpleStableKeyValueSortTest(unsorted_keys, unsorted_values, sorted_keys, sorted_values);

    thrust::stable_sort_by_key(unsorted_keys.begin(), unsorted_keys.end(), unsorted_values.begin(), less_div_10<T>());

    ASSERT_EQUAL(unsorted_keys,   sorted_keys);
    ASSERT_EQUAL(unsorted_values, sorted_values);
}
DECLARE_VECTOR_UNITTEST(TestStableSortByKeySimple);


template <typename T>
void TestStableSortAscendingKeyValue(const size_t n)
{
    thrust::host_vector<T>   h_keys = unittest::random_integers<T>(n);
    thrust::device_vector<T> d_keys = h_keys;
    
    thrust::host_vector<T>   h_values = unittest::random_integers<T>(n);
    thrust::device_vector<T> d_values = h_values;

    thrust::stable_sort_by_key(h_keys.begin(), h_keys.end(), h_values.begin(), less_div_10<T>());
    thrust::stable_sort_by_key(d_keys.begin(), d_keys.end(), d_values.begin(), less_div_10<T>());

    ASSERT_EQUAL(h_keys,   d_keys);
    ASSERT_EQUAL(h_values, d_values);
}
DECLARE_VARIABLE_UNITTEST(TestStableSortAscendingKeyValue);


void TestStableSortDescendingKeyValue(void)
{
    const size_t n = 10027;

    thrust::host_vector<int>   h_keys = unittest::random_integers<int>(n);
    thrust::device_vector<int> d_keys = h_keys;
    
    thrust::host_vector<int>   h_values = unittest::random_integers<int>(n);
    thrust::device_vector<int> d_values = h_values;

    thrust::stable_sort_by_key(h_keys.begin(), h_keys.end(), h_values.begin(), greater_div_10<int>());
    thrust::stable_sort_by_key(d_keys.begin(), d_keys.end(), d_values.begin(), greater_div_10<int>());

    ASSERT_EQUAL(h_keys,   d_keys);
    ASSERT_EQUAL(h_values, d_values);
}
DECLARE_UNITTEST(TestStableSortDescendingKeyValue);


template <typename T, unsigned int N>
void _TestStableSortByKeyWithLargeKeys(void)
{
    size_t n = (128 * 1024) / sizeof(FixedVector<T,N>);

    thrust::host_vector< FixedVector<T,N> > h_keys(n);
    thrust::host_vector<   unsigned int   > h_vals(n);

    for(size_t i = 0; i < n; i++)
    {
        h_keys[i] = FixedVector<T,N>(rand());
        h_vals[i] = i;
    }

    thrust::device_vector< FixedVector<T,N> > d_keys = h_keys;
    thrust::device_vector<   unsigned int   > d_vals = h_vals;
    
    thrust::stable_sort_by_key(h_keys.begin(), h_keys.end(), h_vals.begin());
    thrust::stable_sort_by_key(d_keys.begin(), d_keys.end(), d_vals.begin());

    ASSERT_EQUAL_QUIET(h_keys, d_keys);
    ASSERT_EQUAL_QUIET(h_vals, d_vals);
}

void TestStableSortByKeyWithLargeKeys(void)
{
    _TestStableSortByKeyWithLargeKeys<int,    1>();
    _TestStableSortByKeyWithLargeKeys<int,    2>();
    _TestStableSortByKeyWithLargeKeys<int,    4>();
    _TestStableSortByKeyWithLargeKeys<int,    8>();
    _TestStableSortByKeyWithLargeKeys<int,   16>();
    _TestStableSortByKeyWithLargeKeys<int,   32>();
    _TestStableSortByKeyWithLargeKeys<int,   64>();
    _TestStableSortByKeyWithLargeKeys<int,  128>();
    _TestStableSortByKeyWithLargeKeys<int,  256>();
    _TestStableSortByKeyWithLargeKeys<int,  512>();
    _TestStableSortByKeyWithLargeKeys<int, 1024>();
    _TestStableSortByKeyWithLargeKeys<int, 2048>();
    _TestStableSortByKeyWithLargeKeys<int, 4096>();
    _TestStableSortByKeyWithLargeKeys<int, 8192>();
}
DECLARE_UNITTEST(TestStableSortByKeyWithLargeKeys);


template <typename T, unsigned int N>
void _TestStableSortByKeyWithLargeValues(void)
{
    size_t n = (128 * 1024) / sizeof(FixedVector<T,N>);

    thrust::host_vector<   unsigned int   > h_keys(n);
    thrust::host_vector< FixedVector<T,N> > h_vals(n);

    for(size_t i = 0; i < n; i++)
    {
        h_keys[i] = rand();
        h_vals[i] = FixedVector<T,N>(i);
    }

    thrust::device_vector<   unsigned int   > d_keys = h_keys;
    thrust::device_vector< FixedVector<T,N> > d_vals = h_vals;
    
    thrust::stable_sort_by_key(h_keys.begin(), h_keys.end(), h_vals.begin());
    thrust::stable_sort_by_key(d_keys.begin(), d_keys.end(), d_vals.begin());

    ASSERT_EQUAL_QUIET(h_keys, d_keys);
    ASSERT_EQUAL_QUIET(h_vals, d_vals);

    // so cuda::stable_merge_sort_by_key() is called
    thrust::stable_sort_by_key(h_keys.begin(), h_keys.end(), h_vals.begin(), greater_div_10<unsigned int>());
    thrust::stable_sort_by_key(d_keys.begin(), d_keys.end(), d_vals.begin(), greater_div_10<unsigned int>());

    ASSERT_EQUAL_QUIET(h_keys, d_keys);
    ASSERT_EQUAL_QUIET(h_vals, d_vals);
}

void TestStableSortByKeyWithLargeValues(void)
{
    _TestStableSortByKeyWithLargeValues<int,    1>();
    _TestStableSortByKeyWithLargeValues<int,    2>();
    _TestStableSortByKeyWithLargeValues<int,    4>();
    _TestStableSortByKeyWithLargeValues<int,    8>();
    _TestStableSortByKeyWithLargeValues<int,   16>();
    _TestStableSortByKeyWithLargeValues<int,   32>();
    _TestStableSortByKeyWithLargeValues<int,   64>();
    _TestStableSortByKeyWithLargeValues<int,  128>();
    _TestStableSortByKeyWithLargeValues<int,  256>();
    _TestStableSortByKeyWithLargeValues<int,  512>();
    _TestStableSortByKeyWithLargeValues<int, 1024>();
    _TestStableSortByKeyWithLargeValues<int, 2048>();
    _TestStableSortByKeyWithLargeValues<int, 4096>();
    _TestStableSortByKeyWithLargeValues<int, 8192>();
}
DECLARE_UNITTEST(TestStableSortByKeyWithLargeValues);


template <typename T, unsigned int N>
void _TestStableSortByKeyWithLargeKeysAndValues(void)
{
    size_t n = (128 * 1024) / sizeof(FixedVector<T,N>);

    thrust::host_vector< FixedVector<T,N> > h_keys(n);
    thrust::host_vector< FixedVector<T,N> > h_vals(n);

    for(size_t i = 0; i < n; i++)
    {
        h_keys[i] = FixedVector<T,N>(rand());
        h_vals[i] = FixedVector<T,N>(i);
    }

    thrust::device_vector< FixedVector<T,N> > d_keys = h_keys;
    thrust::device_vector< FixedVector<T,N> > d_vals = h_vals;
    
    thrust::stable_sort_by_key(h_keys.begin(), h_keys.end(), h_vals.begin());
    thrust::stable_sort_by_key(d_keys.begin(), d_keys.end(), d_vals.begin());

    ASSERT_EQUAL_QUIET(h_keys, d_keys);
    ASSERT_EQUAL_QUIET(h_vals, d_vals);
}

void TestStableSortByKeyWithLargeKeysAndValues(void)
{
    _TestStableSortByKeyWithLargeKeysAndValues<int,    1>();
    _TestStableSortByKeyWithLargeKeysAndValues<int,    2>();
    _TestStableSortByKeyWithLargeKeysAndValues<int,    4>();
    _TestStableSortByKeyWithLargeKeysAndValues<int,    8>();
    _TestStableSortByKeyWithLargeKeysAndValues<int,   16>();
    _TestStableSortByKeyWithLargeKeysAndValues<int,   32>();
    _TestStableSortByKeyWithLargeKeysAndValues<int,   64>();
    _TestStableSortByKeyWithLargeKeysAndValues<int,  128>();
    _TestStableSortByKeyWithLargeKeysAndValues<int,  256>();
    _TestStableSortByKeyWithLargeKeysAndValues<int,  512>();
    _TestStableSortByKeyWithLargeKeysAndValues<int, 1024>();
    _TestStableSortByKeyWithLargeKeysAndValues<int, 2048>();
    _TestStableSortByKeyWithLargeKeysAndValues<int, 4096>();
    _TestStableSortByKeyWithLargeKeysAndValues<int, 8192>();
}
DECLARE_UNITTEST(TestStableSortByKeyWithLargeKeysAndValues);

