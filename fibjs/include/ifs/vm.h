/***************************************************************************
 *                                                                         *
 *   This file was automatically generated using idlc.js                   *
 *   PLEASE DO NOT EDIT!!!!                                                *
 *                                                                         *
 ***************************************************************************/

#ifndef _vm_base_H_
#define _vm_base_H_

/**
 @author Leo Hoo <lion@9465.net>
 */

#include "../object.h"

namespace fibjs
{

class SandBox_base;

class vm_base : public object_base
{
    DECLARE_CLASS(vm_base);

public:

};

}

#include "SandBox.h"

namespace fibjs
{
    inline ClassInfo& vm_base::class_info()
    {
        static ClassData::ClassObject s_object[] = 
        {
            {"SandBox", SandBox_base::class_info}
        };

        static ClassData s_cd = 
        { 
            "vm", NULL, 
            0, NULL, 1, s_object, 0, NULL, NULL, NULL,
            NULL
        };

        static ClassInfo s_ci(s_cd);
        return s_ci;
    }


}

#endif

