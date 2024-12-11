#include <cassert>

#include <random>

template <class FLOAT_TYPE, size_t N>
Vector<FLOAT_TYPE, N>::Vector( std::initializer_list<FLOAT_TYPE> values ) {
  auto iterator = values.begin();
  for (size_t i = 0u; i < N; i++) {
    if ( iterator != values.end()) {
      vector[i] = *iterator++;
    } else {
      vector[i] = (i > 0 ? vector[i - 1] : 0.0);
    }
  }
}

template <class FLOAT_TYPE, size_t N>
Vector<FLOAT_TYPE, N>::Vector(FLOAT_TYPE angle ) {
  *this = { static_cast<FLOAT_TYPE>( cos(angle) ), static_cast<FLOAT_TYPE>(sin(angle)) };
}

template <class FLOAT_TYPE, size_t N>  
Vector<FLOAT_TYPE, N> & Vector<FLOAT_TYPE, N>::operator+=(const Vector<FLOAT_TYPE, N> addend) {
  for (size_t i = 0u; i < N; i++) {
    vector[i] += addend.vector[i];
  }
  return *this;
}

template <class FLOAT_TYPE, size_t N>  
Vector<FLOAT_TYPE, N> & Vector<FLOAT_TYPE, N>::operator-=(const Vector<FLOAT_TYPE, N> minuend) {
  for (size_t i = 0u; i < N; i++) {
    vector[i] -= minuend.vector[i];
  }
  return *this;
}

template <class FLOAT_TYPE, size_t N>  
Vector<FLOAT_TYPE, N> & Vector<FLOAT_TYPE, N>::operator*=(const FLOAT_TYPE factor) {
  for (size_t i = 0u; i < N; i++) {
    vector[i] *= factor;
  }
  return *this;
}

template <class FLOAT_TYPE, size_t N>  
Vector<FLOAT_TYPE, N> & Vector<FLOAT_TYPE, N>::operator/=(const FLOAT_TYPE factor) {
  for (size_t i = 0u; i < N; i++) {
    vector[i] /= factor;
  }
  return *this;
}


template <class FLOAT_TYPE, size_t N>    
Vector<FLOAT_TYPE, N> operator*(FLOAT_TYPE scalar, Vector<FLOAT_TYPE, N> value) {
  Vector<FLOAT_TYPE, N> scalar_product = value;

  scalar_product *= scalar;

  return scalar_product;
}

// ----------------------------------------------------------------------------
// neue Methode für Raytracing Aufgabe


template <class FLOAT_TYPE, size_t N>
Vector<FLOAT_TYPE, N> operator/(FLOAT_TYPE scalar, Vector<FLOAT_TYPE, N> value) {
    Vector<FLOAT_TYPE, N> result;
    for (size_t i = 0; i < N; ++i) {
        result[i] = value[i] / scalar;
    }
    return result;
}


// ----------------------------------------------------------------------------



template <class FLOAT_TYPE, size_t N>    
Vector<FLOAT_TYPE, N> operator+(const Vector<FLOAT_TYPE, N> value, const Vector<FLOAT_TYPE, N> addend) {
  Vector<FLOAT_TYPE, N> sum = value;
  sum += addend;
  return sum;
}

template <class FLOAT_TYPE, size_t N>    
Vector<FLOAT_TYPE, N> operator-(const Vector<FLOAT_TYPE, N> value, const Vector<FLOAT_TYPE, N> minuend) {
  Vector<FLOAT_TYPE, N> difference = value;
  difference -= minuend;
  return difference;
}

template <class FLOAT_TYPE, size_t N>  
FLOAT_TYPE & Vector<FLOAT_TYPE, N>::operator[](std::size_t i) {
  return vector[i];
}

template <class FLOAT_TYPE, size_t N>  
FLOAT_TYPE Vector<FLOAT_TYPE, N>::operator[](std::size_t i) const {
  return vector[i];
}


template <class FLOAT_TYPE, size_t N>
Vector<FLOAT_TYPE, 3u> Vector<FLOAT_TYPE, N>::cross_product(const Vector<FLOAT_TYPE, 3u> v) const {
  assert(N >= 3u);
  return {this->vector[1] * v.vector[2] - this->vector[2] * v.vector[1],
          this->vector[0] * v.vector[2] - this->vector[2] * v.vector[0],
          this->vector[0] * v.vector[1] - this->vector[1] * v.vector[0] };
}


// neue Methoden!!!!
// Länge eines Vektors: Vektor v-> = (1 2 3),
// Länge = Betrag von v-> = Wurzel von (1^2 + 2^2 + 3^2)
template <class FLOAT_TYPE, size_t N>
FLOAT_TYPE Vector<FLOAT_TYPE, N>::length() const {
    /*FLOAT_TYPE sum_of_squares = 0.0;
    for (size_t i = 0u; i < N; i++) {
        sum_of_squares += vector[i] * vector[i];
    }
    return sqrt(sum_of_squares);*/
    return std::sqrt(square_of_length());
}

template <class FLOAT_TYPE, size_t N>
FLOAT_TYPE Vector<FLOAT_TYPE, N>::square_of_length() const {
    FLOAT_TYPE sum_of_squares = 0.0;
    for (size_t i = 0u; i < N; i++) {
        sum_of_squares += vector[i] * vector[i];
    }
    return sum_of_squares;
}

// Skalarprodukt zweier Vektoren:
// a-> * b-> = (a1 a2 a3) * (b1 b2 b3)
template <class FLOAT_TYPE, size_t N>
FLOAT_TYPE operator*(Vector<FLOAT_TYPE, N> vector1, const Vector<FLOAT_TYPE, N> vector2) {
    FLOAT_TYPE scalar_product = 0.0;
    for (size_t i = 0u; i < N; i++) {
        scalar_product += vector1[i] * vector2[i];
    }
    return scalar_product;
}







template <class FLOAT_TYPE, size_t N>
void Vector<FLOAT_TYPE, N>::normalize() {
  *this /= length(); //  +/- INFINITY if length is (near to) zero
}

template <class FLOAT_TYPE, size_t N>  
Vector<FLOAT_TYPE, N> Vector<FLOAT_TYPE, N>::get_reflective(Vector<FLOAT_TYPE, N> normal) const {
  assert(0.99999 < normal.square_of_length() && normal.square_of_length()  < 1.000001); 
  return *this - static_cast<FLOAT_TYPE>(2.0) * (*this * normal ) * normal;
}

template <class FLOAT_TYPE, size_t N>
FLOAT_TYPE Vector<FLOAT_TYPE, N>::angle(size_t axis_1, size_t axis_2) const {
  Vector<FLOAT_TYPE, N> normalized = (1.0f / length()) * *this;
  return atan2( normalized[axis_2], normalized[axis_1] );
}



// -------------------------------
// Raytracer
//

template <class FLOAT_TYPE, size_t N>
Vector<FLOAT_TYPE, N> normalizeVector(Vector<FLOAT_TYPE, N> vec) {
    return vec / vec.length(); // Die Länge des Vektors sollte aufgerufen werden, und Sie sollten den Vektor nicht selbst modifizieren
}


// (raytracing in one weekend)
inline float random_float() {
    static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    static std::mt19937 generator;
    return distribution(generator);
}

inline float random_float(float min, float max) {
    // Returns a random real in [min,max).
    return min + (max-min)*random_float();
}



/*
static Vector3df random() {
    return Vector3df{random_float(), random_float(), random_float()};
}

static Vector3df random(float min, float max) {
    return Vector3df{random_float(min,max), random_float(min,max), random_float(min,max)};
}
*/

/*
inline Vector3df random_in_unit_sphere() {
    while (true) {
        auto p = random(-1,1);
        if (p.square_of_length() < 1)
            return p;
    }
}

inline Vector3df unit_vector(Vector3df v) {
    return 1/v.length() * v;
}

inline Vector3df random_unit_vector() {
    return unit_vector(random_in_unit_sphere());
}

inline Vector3df random_on_hemisphere(const Vector3df& normal) {
    Vector3df on_unit_sphere = random_unit_vector();
    if ((on_unit_sphere * normal) > 0.0) // In the same hemisphere as the normal
        return on_unit_sphere;
    else
        return (-1.0f) * on_unit_sphere;
}
 */
