/*
 * This file is part of WebCL – JavaScript bindings for OpenCL
 * http://webcl.nokiaresearch.com/
 *
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Contact: Jari Nikara  ;jari.nikara@nokia.com;
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 *
 * The package is based on a published Khronos OpenCL 1.1 Specification,
 * see http://www.khronos.org/opencl/.
 *
 * OpenCL is a trademark of Apple Inc.
 */

/** \file programwrapper.cpp
 * Program wrapper class implementation.
 */

#include "clwrappercommon_internal.h"
#include "clwrappercommon.h"
#include "programwrapper.h"
#include "devicewrapper.h"
#include "kernelwrapper.h"

#include <vector>
#include <string>

using std::vector;
using std::string;


InstanceRegistry<cl_program, ProgramWrapper*> ProgramWrapper::instanceRegistry;

ProgramWrapper::ProgramWrapper (cl_program aHandle)
    : Wrapper (),
      mWrapped (aHandle)
{

}


ProgramWrapper::~ProgramWrapper () {

}


cl_int ProgramWrapper::buildProgram (std::vector<DeviceWrapper*> const& aDevices,
                                     std::string aOptions,
                                     void (*aNotify)(cl_program, void*),
                                     void* aNotifyUserData) {
    D_METHOD_START;

    cl_device_id* devices = new(std::nothrow) cl_device_id [aDevices.size()];
    if (!devices) return CL_OUT_OF_HOST_MEMORY;
    size_t idx = 0;
    vector<DeviceWrapper*>::const_iterator i = aDevices.begin ();
    while (i != aDevices.end ()) {
        if (*i) {
            devices [idx++] = (*i)->getWrapped ();
        }
        ++i;
    }

    cl_int err = clBuildProgram (mWrapped, idx, idx > 0 ? devices : NULL, aOptions.c_str (),
                                 aNotify, aNotifyUserData);
    if (err != CL_SUCCESS) {
        D_LOG (LOG_LEVEL_ERROR, "clBuildProgram failed. (error %d)", err);
    }
    delete[] devices;

    return err;
}


cl_int ProgramWrapper::createKernel (std::string aKernelName, KernelWrapper** aResultOut) {
    D_METHOD_START;
    cl_int err = CL_SUCCESS;
    VALIDATE_ARG_POINTER (aResultOut, &err, err);

    cl_kernel kernel = clCreateKernel (mWrapped, aKernelName.c_str (), &err);
    if (err != CL_SUCCESS) {
        D_LOG (LOG_LEVEL_ERROR, "clCreateKernel failed. (error %d)", err);
        return err;
    }

    *aResultOut = new(std::nothrow) KernelWrapper (kernel);
    if (!aResultOut) return CL_OUT_OF_HOST_MEMORY;

    return err;
}


cl_int ProgramWrapper::createKernelsInProgram (std::vector<KernelWrapper*>& aResultOut) {
    D_METHOD_START;

    cl_uint num = 0;
    cl_kernel* buf = 0;
    cl_int err = clCreateKernelsInProgram (mWrapped, 0, 0, &num);
    if (err != CL_SUCCESS) {
        D_LOG (LOG_LEVEL_ERROR, " clCreateKernelsInProgram failed. (error %d)", err);
        return err;
    }

    buf = (cl_kernel*)malloc (sizeof(cl_kernel) * num);
    if (!buf) return CL_OUT_OF_HOST_MEMORY;

    err = clCreateKernelsInProgram (mWrapped, num, buf, 0);
    if (err != CL_SUCCESS) {
        free (buf);
        D_LOG (LOG_LEVEL_ERROR, " clCreateKernelsInProgram failed. (error %d)", err);
        return err;
    }

    aResultOut.clear ();
    for (cl_uint i = 0; i < num; ++i) {
        KernelWrapper* wrapper = new(std::nothrow) KernelWrapper (buf[i]);
        if (!wrapper) {
            D_LOG (LOG_LEVEL_ERROR, "Memory allocation failed.");
            free (buf);
            vector<KernelWrapper*>::iterator i = aResultOut.begin ();
            while (i != aResultOut.end ()) {
              KernelWrapper* p = *i;
              ++i;
              if (p)
                p->release ();
            }
            return CL_OUT_OF_HOST_MEMORY;
        }
        aResultOut.push_back (wrapper);
    }
    free (buf);

    return err;
}


/* static */
ProgramWrapper* ProgramWrapper::getNewOrExisting (cl_program aHandle) {
    D_METHOD_START;
    ProgramWrapper* res = 0;
    if (instanceRegistry.findById (aHandle, &res) && res) {
        res->retain ();
        return res;
    }
    return new(std::nothrow) ProgramWrapper (aHandle);
}


/* static */
cl_int ProgramWrapper::programInfoHelper (Wrapper const* aInstance, int aName,
                                          size_t aSize, void* aValueOut, size_t* aSizeOut) {
    cl_int err = CL_SUCCESS;
    ProgramWrapper const* instance = dynamic_cast<ProgramWrapper const*>(aInstance);
    VALIDATE_ARG_POINTER (instance, &err, err);
    return clGetProgramInfo (instance->getWrapped (), aName, aSize, aValueOut, aSizeOut);
}


/* static */
cl_int ProgramWrapper::programBuildInfoHelper (Wrapper const* aInstance, Wrapper const* aExtra,
                                               int aName, size_t aSize, void* aValueOut, size_t* aSizeOut) {
    cl_int err = CL_SUCCESS;
    ProgramWrapper const* instance = dynamic_cast<ProgramWrapper const*>(aInstance);
    DeviceWrapper const* device = dynamic_cast<DeviceWrapper const*>(aExtra);
    VALIDATE_ARG_POINTER (instance, &err, err);
    VALIDATE_ARG_POINTER (device, &err, err);
    return clGetProgramBuildInfo (instance->getWrapped (), device->getWrapped (),
                                  aName, aSize, aValueOut, aSizeOut);
}
