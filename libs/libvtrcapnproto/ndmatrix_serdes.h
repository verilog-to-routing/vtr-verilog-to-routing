#ifndef NDMATRIX_SERDES_H_
#define NDMATRIX_SERDES_H_

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
//
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
