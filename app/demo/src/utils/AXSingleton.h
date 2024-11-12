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

#include <stdio.h>
#include <unistd.h>
#include <type_traits>
#include <typeinfo>
#include "ax_base_type.h"

/* https://stackoverflow.com/questions/34519073/inherit-singleton */
template <typename T>
class CAXSingleton {
public:
    static T *GetInstance(AX_VOID) noexcept(std::is_nothrow_constructible<T>::value) {
        static T instance;
        static AX_BOOL sbInit = AX_FALSE;
        if (!sbInit) {
            if (!instance.InitOnce()) {
                /*
                    [ISSUE  ] GetInstance()->fn,  if GetInstance return nullptr, then SIGSEGV definitely.
                    [WR     ] If InitOnce fail, quit immediately. We cannot throw exception here because GetInstance declared as noexcept
                    [SUGGEST] Keep CAXSingleton::InitOnce always return AX_TRUE, move initialization to derived class, for example:
                                  class CDerivedClass :  public CAXSingleton<CDerivedClass> {
                                        friend class CAXSingleton<CDerivedClass>;
                                    public:
                                        AX_BOOL Init(AX_VOID);
                                  }

                                  // Be careful for the sequence and thread safe, invoke before used.
                                  if (!CDerivedClass::GetInstance()->Init()) {
                                    // TODO:
                                  }
                */
                printf("Create singleton instance fail, please check %s::InitOnce\n", typeid(instance).name());
                _exit(0);
            } else {
                sbInit = AX_TRUE;
            }
        }
        return &instance;
    };

protected:
    CAXSingleton(AX_VOID) noexcept = default;
    virtual ~CAXSingleton(AX_VOID) = default;

    virtual AX_BOOL InitOnce() {
        return AX_TRUE;
    };

private:
    CAXSingleton(const CAXSingleton &rhs) = delete;
    CAXSingleton &operator=(const CAXSingleton &rhs) = delete;
};
