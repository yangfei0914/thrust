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
void InitializeSimpleStableKeySortTest(Vector& unsorted_keys, Vector& sorted_keys)
{
    unsorted_keys.resize(9);   
    unsorted_keys[0] = 25; 
    unsorted_keys[1] = 14; 
    unsorted_keys[2] = 35; 
    unsorted_keys[3] = 16; 
    unsorted_keys[4] = 26; 
    unsorted_keys[5] = 34; 
    unsorted_keys[6] = 36; 
    unsorted_keys[7] = 24; 
    unsorted_keys[8] = 15; 
    
    sorted_keys.resize(9);
    sorted_keys[0] = 14; 
    sorted_keys[1] = 16; 
    sorted_keys[2] = 15; 
    sorted_keys[3] = 25; 
    sorted_keys[4] = 26; 
    sorted_keys[5] = 24; 
    sorted_keys[6] = 35; 
    sorted_keys[7] = 34; 
    sorted_keys[8] = 36; 
}


template <class Vector>
void TestStableSortSimple(void)
{
    typedef typename Vector::value_type T;

    Vector unsorted_keys;
    Vector   sorted_keys;

    InitializeSimpleStableKeySortTest(unsorted_keys, sorted_keys);

    thrust::stable_sort(unsorted_keys.begin(), unsorted_keys.end(), less_div_10<T>());

    ASSERT_EQUAL(unsorted_keys,   sorted_keys);
}
DECLARE_VECTOR_UNITTEST(TestStableSortSimple);


template <typename T>
void TestStableSortAscendingKey(const size_t n)
{
    thrust::host_vector<T>   h_data = unittest::random_integers<T>(n);
    thrust::device_vector<T> d_data = h_data;

    thrust::stable_sort(h_data.begin(), h_data.end(), less_div_10<T>());
    thrust::stable_sort(d_data.begin(), d_data.end(), less_div_10<T>());

    ASSERT_EQUAL(h_data, d_data);
}
DECLARE_VARIABLE_UNITTEST(TestStableSortAscendingKey);


void TestStableSortDescendingKey(void)
{
    const size_t n = 10027;

    thrust::host_vector<int>   h_data = unittest::random_integers<int>(n);
    thrust::device_vector<int> d_data = h_data;

    thrust::stable_sort(h_data.begin(), h_data.end(), greater_div_10<int>());
    thrust::stable_sort(d_data.begin(), d_data.end(), greater_div_10<int>());

    ASSERT_EQUAL(h_data, d_data);
}
DECLARE_UNITTEST(TestStableSortDescendingKey);


template <typename T, unsigned int N>
void _TestStableSortWithLargeKeys(void)
{
    size_t n = (128 * 1024) / sizeof(FixedVector<T,N>);

    thrust::host_vector< FixedVector<T,N> > h_keys(n);

    for(size_t i = 0; i < n; i++)
        h_keys[i] = FixedVector<T,N>(rand());

    thrust::device_vector< FixedVector<T,N> > d_keys = h_keys;
    
    thrust::stable_sort(h_keys.begin(), h_keys.end());
    thrust::stable_sort(d_keys.begin(), d_keys.end());

    ASSERT_EQUAL_QUIET(h_keys, d_keys);
}

void TestStableSortWithLargeKeys(void)
{
    _TestStableSortWithLargeKeys<int,    1>();
    _TestStableSortWithLargeKeys<int,    2>();
    _TestStableSortWithLargeKeys<int,    4>();
    _TestStableSortWithLargeKeys<int,    8>();
    _TestStableSortWithLargeKeys<int,   16>();
    _TestStableSortWithLargeKeys<int,   32>();
    _TestStableSortWithLargeKeys<int,   64>();
    _TestStableSortWithLargeKeys<int,  128>();
    _TestStableSortWithLargeKeys<int,  256>();
    _TestStableSortWithLargeKeys<int,  512>();
    _TestStableSortWithLargeKeys<int, 1024>();
    _TestStableSortWithLargeKeys<int, 2048>();
    _TestStableSortWithLargeKeys<int, 4096>();
    _TestStableSortWithLargeKeys<int, 8192>();
}
DECLARE_UNITTEST(TestStableSortWithLargeKeys);


template <typename T>
struct comp_mod3 : public thrust::binary_function<T,T,bool>
{
    T * table;

    comp_mod3(T * table) : table(table) {}

    __host__ __device__
    bool operator()(T a, T b)
    {
        return table[(int) a] < table[(int) b];
    }
};

template <typename Vector>
void TestStableSortWithIndirection(void)
{
    // add numbers modulo 3 with external lookup table
    typedef typename Vector::value_type T;

    Vector data(7);
    data[0] = 1;
    data[1] = 3;
    data[2] = 5;
    data[3] = 3;
    data[4] = 0;
    data[5] = 2;
    data[6] = 1;

    Vector table(6);
    table[0] = 0;
    table[1] = 1;
    table[2] = 2;
    table[3] = 0;
    table[4] = 1;
    table[5] = 2;

    thrust::stable_sort(data.begin(), data.end(), comp_mod3<T>(thrust::raw_pointer_cast(&table[0])));
    
    ASSERT_EQUAL(data[0], T(3));
    ASSERT_EQUAL(data[1], T(3));
    ASSERT_EQUAL(data[2], T(0));
    ASSERT_EQUAL(data[3], T(1));
    ASSERT_EQUAL(data[4], T(1));
    ASSERT_EQUAL(data[5], T(5));
    ASSERT_EQUAL(data[6], T(2));
}
DECLARE_VECTOR_UNITTEST(TestStableSortWithIndirection);

