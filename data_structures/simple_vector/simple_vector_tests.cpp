#include <cassert>
#include <iostream>
#include <numeric>
#include <stdexcept>

#include "simple_vector.h"

using namespace std;

void TestInitialization() {
    {
        SimpleVector<int> v;
        assert(v.GetSize() == 0u);
        assert(v.IsEmpty());
        assert(v.GetCapacity() == 0u);
    }

    {
        SimpleVector<int> v(5);
        assert(v.GetSize() == 5u);
        assert(v.GetCapacity() == 5u);
        assert(!v.IsEmpty());
        for (size_t i = 0; i < v.GetSize(); ++i) {
            assert(v[i] == 0);
        }
    }

    {
        SimpleVector<int> v(3, 42);
        assert(v.GetSize() == 3);
        assert(v.GetCapacity() == 3);
        for (size_t i = 0; i < v.GetSize(); ++i) {
            assert(v[i] == 42);
        }
    }

    {
        SimpleVector<int> v{1, 2, 3};
        assert(v.GetSize() == 3);
        assert(v.GetCapacity() == 3);
        assert(v[2] == 3);
    }
}

void TestMethods() {
    // At
    {
        SimpleVector<int> v(3);
        assert(&v.At(2) == &v[2]);

        try {
            v.At(3);
            assert(false);
        } catch (const std::out_of_range&) {
        } catch (...) {
            assert(false);
        }
    }

    // Clear
    {
        SimpleVector<int> v(10);
        const size_t old_capacity = v.GetCapacity();
        v.Clear();
        assert(v.GetSize() == 0);
        assert(v.GetCapacity() == old_capacity);
    }

    // Resize
    {
        SimpleVector<int> v(3);
        v[2] = 17;
        v.Resize(7);
        assert(v.GetSize() == 7);
        assert(v.GetCapacity() >= v.GetSize());
        assert(v[2] == 17);
        assert(v[3] == 0);
    }

    {
        SimpleVector<int> v(3);
        v[0] = 42;
        v[1] = 55;
        const size_t old_capacity = v.GetCapacity();
        v.Resize(2);
        assert(v.GetSize() == 2);
        assert(v.GetCapacity() == old_capacity);
        assert(v[0] == 42);
        assert(v[1] == 55);
    }

    {
        const size_t old_size = 3;
        SimpleVector<int> v(3);
        v.Resize(old_size + 5);
        v[3] = 42;
        v.Resize(old_size);
        v.Resize(old_size + 2);
        assert(v[3] == 0);
    }

    // iterators
    {
        SimpleVector<int> v;
        assert(v.begin() == nullptr);
        assert(v.end() == nullptr);
    }

    {
        SimpleVector<int> v(10, 42);
        assert(v.begin());
        assert(*v.begin() == 42);
        assert(v.end() == v.begin() + v.GetSize());
    }

    // Copy constructor
    {
        SimpleVector<int> v{1, 2, 3};
        SimpleVector<int> copy_v(v);
        assert(copy_v[0] == v[0]);
        assert(copy_v[2] == v[2]);
        assert(v.GetSize() == copy_v.GetSize());
        assert(v.GetCapacity() == copy_v.GetCapacity());

        v[0] = 11;
        assert(copy_v[0] == 1);
    }

    // operator=
    {
        SimpleVector<int> v1{1, 2, 3};
        SimpleVector<int> v2;
        v2 = v1;
        assert(v2[0] == v1[0]);
        assert(v2[2] == v1[2]);
        assert(v1.GetSize() == v2.GetSize());
        assert(v1.GetCapacity() == v2.GetCapacity());

        v1[0] = 11;
        assert(v2[0] == 1);
    }

    // PushBack
    {
        SimpleVector<int> v;
        v.PushBack(10);
        assert(v.GetSize() == 1);
        assert(v.GetCapacity() == 1);
        assert(v[0] == 10);

        v.PushBack(20);
        assert(v.GetSize() == 2);
        assert(v.GetCapacity() == 2);
        assert(v[1] == 20);
    }

    {
        SimpleVector<int> v{1, 2, 3};
        v.Resize(2);
        v.PushBack(4);

        assert(v.GetSize() == 3u);
        assert(v.GetCapacity() == 3u);
        assert(v[0] == 1);
        assert(v[2] == 4);
    }

    // Insert: size < capacity
    {
        SimpleVector<int> v{2, 3, 4};
        v.Resize(2);

        int* p = &v[0];
        int* new_p = v.Insert(p, 1);

        assert(v.GetSize() == 3u);
        assert(v.GetCapacity() == 3u);
        assert(v[0] == 1);
        assert(v[1] == 2);
        assert(v[2] == 3);
        assert(new_p == &v[0]);
    }

    // Insert: size == capacity
    {
        SimpleVector<int> v{1, 3, 4};
        int* p = &v[1];
        int* new_p = v.Insert(p, 2);

        assert(v.GetSize() == 4u);
        assert(v.GetCapacity() >= 4u);
        assert(v[0] == 1);
        assert(v[1] == 2);
        assert(v[2] == 3);
        assert(v[3] == 4);
        assert(new_p == &v[1]);
    }

    // ReserveConstructor
    {
        SimpleVector<int> v(Reserve(5));
        assert(v.GetCapacity() == 5);
        assert(v.IsEmpty());
    }

    // ReserveMethod
    {
        SimpleVector<int> v;
        v.Reserve(5);
        assert(v.GetCapacity() == 5);
        assert(v.IsEmpty());

        v.Reserve(1);
        assert(v.GetCapacity() == 5);

        for (int i = 0; i < 10; ++i) {
            v.PushBack(i);
        }

        assert(v.GetSize() == 10);
        v.Reserve(100);
        assert(v.GetSize() == 10);
        assert(v.GetCapacity() == 100);

        for (int i = 0; i < 10; ++i) {
            assert(v[i] == i);
        }
    }
}

class X {
   public:
    X() : X(5) {}
    X(size_t num) : x_(num) {}
    X(const X& other) = delete;
    X& operator=(const X& other) = delete;
    X(X&& other) {
        x_ = exchange(other.x_, 0);
    }
    X& operator=(X&& other) {
        x_ = exchange(other.x_, 0);
        return *this;
    }
    size_t GetX() const {
        return x_;
    }

   private:
    size_t x_;
};

SimpleVector<int> GenerateVector(size_t size) {
    SimpleVector<int> v(size);
    iota(v.begin(), v.end(), 1);
    return v;
}

void TestTemporaryObjConstructor() {
    const size_t size = 1000000;
    SimpleVector<int> moved_vector(GenerateVector(size));
    assert(moved_vector.GetSize() == size);
}

void TestTemporaryObjOperator() {
    const size_t size = 1000000;
    SimpleVector<int> moved_vector;
    assert(moved_vector.GetSize() == 0);
    moved_vector = GenerateVector(size);
    assert(moved_vector.GetSize() == size);
}

void TestNamedMoveConstructor() {
    const size_t size = 1000000;
    SimpleVector<int> vector_to_move(GenerateVector(size));
    assert(vector_to_move.GetSize() == size);

    SimpleVector<int> moved_vector(move(vector_to_move));
    assert(moved_vector.GetSize() == size);
    assert(vector_to_move.GetSize() == 0);
}

void TestNamedMoveOperator() {
    const size_t size = 1000000;
    SimpleVector<int> vector_to_move(GenerateVector(size));
    assert(vector_to_move.GetSize() == size);

    SimpleVector<int> moved_vector = move(vector_to_move);
    assert(moved_vector.GetSize() == size);
    assert(vector_to_move.GetSize() == 0);
}

void TestNoncopiableMoveConstructor() {
    const size_t size = 5;
    SimpleVector<X> vector_to_move;

    for (size_t i = 0; i < size; ++i) {
        vector_to_move.PushBack(X(i));
    }

    SimpleVector<X> moved_vector = move(vector_to_move);
    assert(moved_vector.GetSize() == size);
    assert(vector_to_move.GetSize() == 0);

    for (size_t i = 0; i < size; ++i) {
        assert(moved_vector[i].GetX() == i);
    }
}

void TestNoncopiablePushBack() {
    const size_t size = 5;
    SimpleVector<X> v;

    for (size_t i = 0; i < size; ++i) {
        v.PushBack(X(i));
    }

    assert(v.GetSize() == size);

    for (size_t i = 0; i < size; ++i) {
        assert(v[i].GetX() == i);
    }
}

void TestNoncopiableInsert() {
    const size_t size = 5;
    SimpleVector<X> v;

    for (size_t i = 0; i < size; ++i) {
        v.PushBack(X(i));
    }

    v.Insert(v.begin(), X(size + 1));
    assert(v.GetSize() == size + 1);
    assert(v.begin()->GetX() == size + 1);

    v.Insert(v.end(), X(size + 2));
    assert(v.GetSize() == size + 2);
    assert((v.end() - 1)->GetX() == size + 2);

    v.Insert(v.begin() + 3, X(size + 3));
    assert(v.GetSize() == size + 3);
    assert((v.begin() + 3)->GetX() == size + 3);
}

void TestNoncopiableErase() {
    const size_t size = 3;
    SimpleVector<X> v;

    for (size_t i = 0; i < size; ++i) {
        v.PushBack(X(i));
    }

    auto it = v.Erase(v.begin());
    assert(it->GetX() == 1);
}

int main() {
    cout << "Start..." << endl;

    TestInitialization();
    TestMethods();

    // test move
    TestTemporaryObjConstructor();
    TestTemporaryObjOperator();
    TestNamedMoveConstructor();
    TestNamedMoveOperator();
    TestNoncopiableMoveConstructor();
    TestNoncopiablePushBack();
    TestNoncopiableInsert();
    TestNoncopiableErase();

    cout << "Done!" << endl;

    return 0;
}
