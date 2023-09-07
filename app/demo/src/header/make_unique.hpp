/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#pragma once
#include <memory>

#if !defined(_MSC_VER) && __cplusplus <= 201103L
namespace std {
template <typename T, typename... Args>
inline typename enable_if<!is_array<T>::value, unique_ptr<T>>::type make_unique(Args&&... args) {
    return unique_ptr<T>(new (nothrow) T(std::forward<Args>(args)...));
}

template <typename T>
inline typename enable_if<is_array<T>::value && extent<T>::value == 0, unique_ptr<T>>::type make_unique(size_t size) {
    using U = typename remove_extent<T>::type;
    return unique_ptr<T>(new (nothrow) U[size]());
}
template <typename T, typename... Args>
typename enable_if<extent<T>::value != 0, void>::type make_unique(Args&&...) = delete;

}  // namespace std
#endif /* !defined(_MSC_VER)&&__cplusplus<=201103L */