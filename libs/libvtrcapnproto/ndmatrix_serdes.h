#ifndef NDMATRIX_SERDES_H_
#define NDMATRIX_SERDES_H_
// Provide generic functions for serializing vtr::NdMatrix to and from Cap'n
// proto Matrix.
//
// Functions:
//  ToNdMatrix - Converts Matrix capnproto message to vtr::NdMatrix
//  FromNdMatrix - Converts vtr::NdMatrix to Matrix capnproto
//
// Example:
//
//  Schema:
//
//  using Matrix = import "matrix.capnp";
//
//
//  struct Test {
//    struct Vec2 {
//      x @0 :Float32;
//      y @1 :Float32;
//    }
//    vectors @0 :Matrix.Matrix(Vec2)
//  }
//
//  C++:
//
//  struct Vec2 {
//    float x;
//    float y;
//  };
//
//  static void ToVec2(Vec2 *out, const Test::Vec2::Reader& in) {
//    out->x = in.getX();
//    out->y = in.getY();
//  }
//
//  static void FromVec2(Test::Vec2::Builder* out, const Vec2 &in) {
//    out->setX(in.x);
//    out->setY(in.y);
//  }
//
//  void example(std::string file) {
//      vtr::NdMatrix<Vec2, 3> mat_in({3, 3, 3}, {2, 3});
//
//      ::capnp::MallocMessageBuilder builder;
//      auto test = builder.initRoot<Test>();
//      auto vectors = test.getVectors();
//
//      FromNdMatrix<3, Test::Vec2, Vec2>(&vectors, mat_in, FromVec2);
//      writeMessageToFile(file, &builder);
//
//      MmapFile f(file);
//      ::capnp::FlatArrayMessageReader reader(f.getData());
//      auto test = reader.getRoot<Test>();
//
//      vtr::NdMatrix<Vec2, 3> mat_out;
//      ToNdMatrix<3, Test::Vec2, Vec2>(&mat_out, test.getVectors(), FromVec2);
//  }
#include <functional>
#include "vtr_ndmatrix.h"
#include "vpr_error.h"
#include "matrix.capnp.h"

// Generic function to convert from Matrix capnproto message to vtr::NdMatrix.
//
// Template arguments:
//  N = Number of matrix dimensions, must be fixed.
//  CapType = Source capnproto message type that is a single element the
//            Matrix capnproto message.
//  CType = Target C++ type that is a single element of vtr::NdMatrix.
//
// Arguments:
//  m_out = Target vtr::NdMatrix.
//  m_in = Source capnproto message reader.
//  copy_fun = Function to convert from CapType to CType.
template<size_t N, typename CapType, typename CType>
void ToNdMatrix(
    vtr::NdMatrix<CType, N>* m_out,
    const typename Matrix<CapType>::Reader& m_in,
    const std::function<void(CType*, const typename CapType::Reader&)>& copy_fun) {
    const auto& dims = m_in.getDims();
    if (N != dims.size()) {
        VPR_THROW(VPR_ERROR_OTHER,
                  "Wrong dimension of template N = %zu, m_in.getDims() = %zu",
                  N, dims.size());
    }

    std::array<size_t, N> dim_sizes;
    size_t required_elements = 1;
    for (size_t i = 0; i < N; ++i) {
        dim_sizes[i] = dims[i];
        required_elements *= dims[i];
    }
    m_out->resize(dim_sizes);

    const auto& data = m_in.getData();
    if (data.size() != required_elements) {
        VPR_THROW(VPR_ERROR_OTHER,
                  "Wrong number of elements, expected %zu, actual %zu",
                  required_elements, data.size());
    }

    for (size_t i = 0; i < required_elements; ++i) {
        copy_fun(&m_out->get(i), data[i].getValue());
    }
}

// Generic function to convert from vtr::NdMatrix to Matrix capnproto message.
//
// Template arguments:
//  N = Number of matrix dimensions, must be fixed.
//  CapType = Target capnproto message type that is a single element the
//            Matrix capnproto message.
//  CType = Source C++ type that is a single element of vtr::NdMatrix.
//
// Arguments:
//  m_out = Target capnproto message builder.
//  m_in = Source vtr::NdMatrix.
//  copy_fun = Function to convert from CType to CapType.
//
template<size_t N, typename CapType, typename CType>
void FromNdMatrix(
    typename Matrix<CapType>::Builder* m_out,
    const vtr::NdMatrix<CType, N>& m_in,
    const std::function<void(typename CapType::Builder*, const CType&)>& copy_fun) {
    size_t elements = 1;
    auto dims = m_out->initDims(N);
    for (size_t i = 0; i < N; ++i) {
        dims.set(i, m_in.dim_size(i));
        elements *= dims[i];
    }

    auto data = m_out->initData(elements);

    for (size_t i = 0; i < elements; ++i) {
        auto elem = data[i].getValue();
        copy_fun(&elem, m_in.get(i));
    }
}

#endif /* NDMATRIX_SERDES_H_ */
