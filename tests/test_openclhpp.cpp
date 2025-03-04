#define CL_HPP_UNIT_TEST_ENABLE

// We want to support all versions
#define CL_HPP_MINIMUM_OPENCL_VERSION 100
# include <CL/opencl.hpp>
# define TEST_RVALUE_REFERENCES
# define VECTOR_CLASS cl::vector
# define STRING_CLASS cl::string

extern "C"
{
#include <unity.h>
#include <cmock.h>
#include "Mockcl.h"
#include "Mockcl_ext.h"
#include <string.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/// Creates fake IDs that are easy to identify

static inline cl_platform_id make_platform_id(int index)
{
    return (cl_platform_id) (size_t) (0x1a1a1a1a + index);
}

static inline cl_context make_context(int index)
{
    return (cl_context) (size_t) (0xcccccccc + index);
}

static inline cl_device_id make_device_id(int index)
{
    return (cl_device_id) (size_t) (0xdededede + index);
}

static inline cl_mem make_mem(int index)
{
    return (cl_mem) (size_t) (0x33333333 + index);
}

static inline cl_command_queue make_command_queue(int index)
{
    return (cl_command_queue) (size_t) (0xc0c0c0c0 + index);
}

static inline cl_kernel make_kernel(int index)
{
    return (cl_kernel) (size_t) (0xcececece + index);
}

static inline cl_program make_program(int index)
{
    return (cl_program)(size_t)(0xcfcfcfcf + index);
}

static inline cl_command_buffer_khr make_command_buffer_khr(int index)
{
    return (cl_command_buffer_khr)(size_t)(0x8f8f8f8f + index);
}

static inline cl_event make_event(int index)
{
    return (cl_event)(size_t)(0xd0d0d0d0 + index);
}

#if defined(cl_khr_semaphore)
static inline cl_semaphore_khr make_semaphore_khr(int index)
{
    return (cl_semaphore_khr)(size_t)(0xa0b0c0e0 + index);
}
#endif

/* Pools of pre-allocated wrapped objects for tests. There is no device pool,
 * because there is no way to know whether the test wants the device to be
 * reference countable or not.
 */
static const int POOL_MAX = 5;
static cl::Platform platformPool[POOL_MAX];
static cl::Context contextPool[POOL_MAX];
static cl::CommandQueue commandQueuePool[POOL_MAX];
static cl::Buffer bufferPool[POOL_MAX];
static cl::Image2D image2DPool[POOL_MAX];
static cl::Image3D image3DPool[POOL_MAX];
static cl::Kernel kernelPool[POOL_MAX];
static cl::Program programPool[POOL_MAX];
#if defined(cl_khr_command_buffer)
static cl::CommandBufferKhr commandBufferKhrPool[POOL_MAX];
#endif
#if defined(cl_khr_semaphore)
static cl::Semaphore semaphorePool[POOL_MAX];
#endif

/****************************************************************************
 * Stub functions shared by multiple tests
 ****************************************************************************/
/**
 * Stub implementation of clGetCommandQueueInfo that returns the first context.
 */
static cl_int clGetCommandQueueInfo_context(
    cl_command_queue id,
    cl_command_queue_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    (void) id;
    (void) num_calls;


    TEST_ASSERT_EQUAL_HEX(CL_QUEUE_CONTEXT, param_name);
    TEST_ASSERT(param_value == nullptr || param_value_size >= sizeof(cl_context));
    if (param_value_size_ret != nullptr)
        *param_value_size_ret = sizeof(cl_context);
    if (param_value != nullptr)
        *(cl_context *)param_value = make_context(0);

    return CL_SUCCESS;
}

/**
 * Stub implementation of clGetDeviceInfo that just returns the first platform.
 */
static cl_int clGetDeviceInfo_platform(
    cl_device_id id,
    cl_device_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    (void) id;
    (void) num_calls;

    TEST_ASSERT_EQUAL_HEX(CL_DEVICE_PLATFORM, param_name);
    TEST_ASSERT(param_value == nullptr || param_value_size >= sizeof(cl_platform_id));
    if (param_value_size_ret != nullptr)
        *param_value_size_ret = sizeof(cl_platform_id);
    if (param_value != nullptr)
        *(cl_platform_id *) param_value = make_platform_id(0);
    return CL_SUCCESS;
}

/**
 * Stub implementation of clGetContextInfo that just returns the first device.
 */
static cl_int clGetContextInfo_device(
    cl_context id,
    cl_context_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    (void) id;
    (void) num_calls;

    TEST_ASSERT_EQUAL_HEX(CL_CONTEXT_DEVICES, param_name);
    TEST_ASSERT(param_value == nullptr || param_value_size >= sizeof(cl_device_id));
    if (param_value_size_ret != nullptr)
        *param_value_size_ret = sizeof(cl_device_id);
    if (param_value != nullptr)
        *(cl_device_id *) param_value = make_device_id(0);
    return CL_SUCCESS;
}


/**
 * Stub implementation of clGetPlatformInfo that returns a specific version.
 * It also checks that the id is the zeroth platform.
 */
static cl_int clGetPlatformInfo_version(
    cl_platform_id id,
    cl_platform_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    const char *version)
{
    size_t bytes = strlen(version) + 1;

    TEST_ASSERT_NOT_NULL(id);
    TEST_ASSERT_EQUAL_PTR(make_platform_id(0), id);
    TEST_ASSERT_EQUAL_HEX(CL_PLATFORM_VERSION, param_name);
    TEST_ASSERT(param_value == nullptr || param_value_size >= bytes);
    if (param_value_size_ret != nullptr)
        *param_value_size_ret = bytes;
    if (param_value != nullptr)
        strcpy((char *) param_value, version);
    return CL_SUCCESS;
}

/**
 * A stub for clGetPlatformInfo that will only support querying
 * CL_PLATFORM_VERSION, and will return version 1.1.
 */
static cl_int clGetPlatformInfo_version_1_1(
    cl_platform_id id,
    cl_platform_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    (void) num_calls;
    return clGetPlatformInfo_version(
        id, param_name, param_value_size, param_value,
        param_value_size_ret, "OpenCL 1.1 Mock");
}

/**
 * A stub for clGetPlatformInfo that will only support querying
 * CL_PLATFORM_VERSION, and will return version 1.2.
 */
static cl_int clGetPlatformInfo_version_1_2(
    cl_platform_id id,
    cl_platform_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    (void) num_calls;
    return clGetPlatformInfo_version(
        id, param_name, param_value_size, param_value,
        param_value_size_ret, "OpenCL 1.2 Mock");
}

/**
 * A stub for clGetPlatformInfo that will only support querying
 * CL_PLATFORM_VERSION, and will return version 2.0.
 */
static cl_int clGetPlatformInfo_version_2_0(
    cl_platform_id id,
    cl_platform_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    (void) num_calls;
    return clGetPlatformInfo_version(
        id, param_name, param_value_size, param_value,
        param_value_size_ret, "OpenCL 2.0 Mock");
}

#if CL_HPP_TARGET_OPENCL_VERSION >= 300
/**
 * A stub for clGetPlatformInfo that will only support querying
 * CL_PLATFORM_VERSION, and will return version 3.0.
 */
static cl_int clGetPlatformInfo_version_3_0(
    cl_platform_id id,
    cl_platform_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    (void) num_calls;
    return clGetPlatformInfo_version(
        id, param_name, param_value_size, param_value,
        param_value_size_ret, "OpenCL 3.0 Mock");
}
#endif

/* Simulated reference counts. The table points to memory held by the caller.
 * This makes things simpler in the common case of only one object to be
 * reference counted.
 */
class RefcountTable
{
private:
    size_t n; // number of objects
    void * const *objects; // object IDs
    int *refcounts;        // current refcounts

    size_t find(void *object)
    {
        size_t idx = 0;
        while (idx < n && objects[idx] != object)
            idx++;
        TEST_ASSERT(idx < n);
        TEST_ASSERT(refcounts[idx] > 0); // otherwise object has been destroyed
        return idx;
    }

public:
    RefcountTable() : n(0), objects(nullptr), refcounts(nullptr) {}

    void init(size_t n, void * const *objects, int *refcounts)
    {
        this->n = n;
        this->objects = objects;
        this->refcounts = refcounts;
    }

    void reset()
    {
        init(0, nullptr, nullptr);
    }

    cl_int retain(void *object)
    {
        size_t idx = find(object);
        ++refcounts[idx];
        return CL_SUCCESS;
    }

    cl_int release(void *object)
    {
        size_t idx = find(object);
        --refcounts[idx];
        return CL_SUCCESS;
    }
};

/* Stubs for retain/release calls that track reference counts. The stubs
 * check that the reference count never becomes negative and that a zero
 * reference count is never incremented.
 *
 * Use the prepareRefcount* calls to set up the global variables first.
 */

#define MAKE_REFCOUNT_STUBS(cl_type, retainfunc, releasefunc, table) \
    static RefcountTable table; \
    static cl_int retainfunc ## _refcount(cl_type object, int num_calls) \
    { \
        (void) num_calls; \
        return table.retain(object); \
    } \
    static cl_int releasefunc ## _refcount(cl_type object, int num_calls) \
    { \
        (void) num_calls; \
        return table.release(object); \
    } \
    static void prepare_ ## table(size_t n, cl_type const *objects, int *refcounts) \
    { \
        table.init(n, (void * const *) objects, refcounts); \
        retainfunc ## _StubWithCallback(retainfunc ## _refcount); \
        releasefunc ## _StubWithCallback(releasefunc ## _refcount); \
    }

MAKE_REFCOUNT_STUBS(cl_program, clRetainProgram, clReleaseProgram, programRefcounts)
MAKE_REFCOUNT_STUBS(cl_command_queue, clRetainCommandQueue, clReleaseCommandQueue, commandQueueRefcounts)
MAKE_REFCOUNT_STUBS(cl_device_id, clRetainDevice, clReleaseDevice, deviceRefcounts)
MAKE_REFCOUNT_STUBS(cl_context, clRetainContext, clReleaseContext, contextRefcounts)
MAKE_REFCOUNT_STUBS(cl_mem, clRetainMemObject, clReleaseMemObject, memRefcounts)
// Deactivated because unused for now.
#if defined(cl_khr_command_buffer) && 0
MAKE_REFCOUNT_STUBS(cl_command_buffer_khr, clRetainCommandBufferKHR, clReleaseCommandBufferKHR, commandBufferKhrRefcounts)
#endif

/* The indirection through MAKE_MOVE_TESTS2 with a prefix parameter is to
 * prevent the simple-minded parser from Unity from identifying tests from the
 * macro value.
 */
#ifdef TEST_RVALUE_REFERENCES
#define MAKE_MOVE_TESTS2(prefix, type, makeFunc, releaseFunc, pool) \
    void prefix ## MoveAssign ## type ## NonNull(void) \
    { \
        releaseFunc ## _ExpectAndReturn(makeFunc(0), CL_SUCCESS); \
        pool[0] = std::move(pool[1]); \
        TEST_ASSERT_EQUAL_PTR(makeFunc(1), pool[0]()); \
        TEST_ASSERT_NULL(pool[1]()); \
    } \
    \
    void prefix ## MoveAssign ## type ## Null(void) \
    { \
        pool[0]() = nullptr; \
        pool[0] = std::move(pool[1]); \
        TEST_ASSERT_EQUAL_PTR(makeFunc(1), pool[0]()); \
        TEST_ASSERT_NULL(pool[1]()); \
    } \
    \
    void prefix ## MoveConstruct ## type ## NonNull(void) \
    { \
        cl::type tmp(std::move(pool[0])); \
        TEST_ASSERT_EQUAL_PTR(makeFunc(0), tmp()); \
        TEST_ASSERT_NULL(pool[0]()); \
        tmp() = nullptr; \
    } \
    \
    void prefix ## MoveConstruct ## type ## Null(void) \
    { \
        cl::type empty; \
        cl::type tmp(std::move(empty)); \
        TEST_ASSERT_NULL(tmp()); \
        TEST_ASSERT_NULL(empty()); \
    }
#else
#define MAKE_MOVE_TESTS2(prefix, type, makeFunc, releaseFunc, pool) \
    void prefix ## MoveAssign ## type ## NonNull(void) {} \
    void prefix ## MoveAssign ## type ## Null(void) {} \
    void prefix ## MoveConstruct ## type ## NonNull(void) {} \
    void prefix ## MoveConstruct ## type ## Null(void) {}
#endif // !TEST_RVALUE_REFERENCES
#define MAKE_MOVE_TESTS(type, makeFunc, releaseFunc, pool) \
    MAKE_MOVE_TESTS2(test, type, makeFunc, releaseFunc, pool)

void setUp(void)
{
    /* init extensions addresses with mocked functions */
#if defined(cl_khr_command_buffer)
    cl::pfn_clCreateCommandBufferKHR = ::clCreateCommandBufferKHR;
    cl::pfn_clFinalizeCommandBufferKHR = ::clFinalizeCommandBufferKHR;
    cl::pfn_clRetainCommandBufferKHR = ::clRetainCommandBufferKHR;
    cl::pfn_clReleaseCommandBufferKHR = ::clReleaseCommandBufferKHR;
    cl::pfn_clGetCommandBufferInfoKHR = ::clGetCommandBufferInfoKHR;
#endif
#if defined(cl_khr_semaphore)
    cl::pfn_clCreateSemaphoreWithPropertiesKHR = ::clCreateSemaphoreWithPropertiesKHR;
    cl::pfn_clReleaseSemaphoreKHR = ::clReleaseSemaphoreKHR;
    cl::pfn_clRetainSemaphoreKHR = ::clRetainSemaphoreKHR;
    cl::pfn_clEnqueueWaitSemaphoresKHR = ::clEnqueueWaitSemaphoresKHR;
    cl::pfn_clEnqueueSignalSemaphoresKHR = ::clEnqueueSignalSemaphoresKHR;
    cl::pfn_clGetSemaphoreInfoKHR = ::clGetSemaphoreInfoKHR;
#endif
#if defined(cl_khr_external_semaphore)
    cl::pfn_clGetSemaphoreHandleForTypeKHR = ::clGetSemaphoreHandleForTypeKHR;
#endif
#ifdef cl_khr_external_memory
    cl::pfn_clEnqueueAcquireExternalMemObjectsKHR = ::clEnqueueAcquireExternalMemObjectsKHR;
    cl::pfn_clEnqueueReleaseExternalMemObjectsKHR = ::clEnqueueReleaseExternalMemObjectsKHR;
#endif

    /* We reach directly into the objects rather than using assignment to
     * avoid the reference counting functions from being called.
     */
    for (int i = 0; i < POOL_MAX; i++)
    {
        platformPool[i]() = make_platform_id(i);
        contextPool[i]() = make_context(i);
        commandQueuePool[i]() = make_command_queue(i);
        bufferPool[i]() = make_mem(i);
        image2DPool[i]() = make_mem(i);
        image3DPool[i]() = make_mem(i);
        kernelPool[i]() = make_kernel(i);
        programPool[i]() = make_program(i);
#if defined(cl_khr_command_buffer)
        commandBufferKhrPool[i]() = make_command_buffer_khr(i);
#endif
#if defined(cl_khr_semaphore)
        semaphorePool[i]() = make_semaphore_khr(i);
#endif
    }

    programRefcounts.reset();
    deviceRefcounts.reset();
    contextRefcounts.reset();
    memRefcounts.reset();
}

void tearDown(void)
{
    /* Wipe out the internal state to avoid a release call being made */
    for (int i = 0; i < POOL_MAX; i++)
    {
        platformPool[i]() = nullptr;
        contextPool[i]() = nullptr;
        commandQueuePool[i]() = nullptr;
        bufferPool[i]() = nullptr;
        image2DPool[i]() = nullptr;
        image3DPool[i]() = nullptr;
        kernelPool[i]() = nullptr;
        programPool[i]() = nullptr;
#if defined(cl_khr_command_buffer)
        commandBufferKhrPool[i]() = nullptr;
#endif
#if defined(cl_khr_semaphore)
        semaphorePool[i]() = nullptr;
#endif
    }

#if defined(cl_khr_command_buffer)
    cl::pfn_clCreateCommandBufferKHR = nullptr;
    cl::pfn_clFinalizeCommandBufferKHR = nullptr;
    cl::pfn_clRetainCommandBufferKHR = nullptr;
    cl::pfn_clReleaseCommandBufferKHR = nullptr;
    cl::pfn_clGetCommandBufferInfoKHR = nullptr;
#endif
#if defined(cl_khr_semaphore)
    cl::pfn_clCreateSemaphoreWithPropertiesKHR = nullptr;
    cl::pfn_clReleaseSemaphoreKHR = nullptr;
    cl::pfn_clRetainSemaphoreKHR = nullptr;
    cl::pfn_clEnqueueWaitSemaphoresKHR = nullptr;
    cl::pfn_clEnqueueSignalSemaphoresKHR = nullptr;
    cl::pfn_clGetSemaphoreInfoKHR = nullptr;
#endif
#ifdef cl_khr_external_memory
    cl::pfn_clEnqueueAcquireExternalMemObjectsKHR = nullptr;
    cl::pfn_clEnqueueReleaseExternalMemObjectsKHR = nullptr;
#endif
}

/****************************************************************************
 * Tests for cl::Context
 ****************************************************************************/

void testCopyContextNonNull(void)
{
    clReleaseContext_ExpectAndReturn(make_context(0), CL_SUCCESS);
    clRetainContext_ExpectAndReturn(make_context(1), CL_SUCCESS);

    contextPool[0] = contextPool[1];
    TEST_ASSERT_EQUAL_PTR(make_context(1), contextPool[0]());
}

void testMoveAssignContextNonNull(void);
void testMoveAssignContextNull(void);
void testMoveConstructContextNonNull(void);
void testMoveConstructContextNull(void);
MAKE_MOVE_TESTS(Context, make_context, clReleaseContext, contextPool)

/// Stub for querying CL_CONTEXT_DEVICES that returns two devices
static cl_int clGetContextInfo_testContextGetDevices(
    cl_context context,
    cl_context_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    (void) num_calls;
    TEST_ASSERT_EQUAL_PTR(make_context(0), context);
    TEST_ASSERT_EQUAL_HEX(CL_CONTEXT_DEVICES, param_name);
    TEST_ASSERT(param_value == nullptr || param_value_size >= 2 * sizeof(cl_device_id));
    if (param_value_size_ret != nullptr)
        *param_value_size_ret = 2 * sizeof(cl_device_id);
    if (param_value != nullptr)
    {
        cl_device_id *devices = (cl_device_id *) param_value;
        devices[0] = make_device_id(0);
        devices[1] = make_device_id(1);
    }
    return CL_SUCCESS;
}

/// Test that queried devices are not refcounted
void testContextGetDevices1_1(void)
{
    clGetContextInfo_StubWithCallback(clGetContextInfo_testContextGetDevices);
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_1);

    VECTOR_CLASS<cl::Device> devices = contextPool[0].getInfo<CL_CONTEXT_DEVICES>();
    TEST_ASSERT_EQUAL(2, devices.size());
    TEST_ASSERT_EQUAL_PTR(make_device_id(0), devices[0]());
    TEST_ASSERT_EQUAL_PTR(make_device_id(1), devices[1]());
}

/// Test that queried devices are correctly refcounted
void testContextGetDevices1_2(void)
{
    clGetContextInfo_StubWithCallback(clGetContextInfo_testContextGetDevices);
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_2);

    clRetainDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);
    clRetainDevice_ExpectAndReturn(make_device_id(1), CL_SUCCESS);

    VECTOR_CLASS<cl::Device> devices = contextPool[0].getInfo<CL_CONTEXT_DEVICES>();
    TEST_ASSERT_EQUAL(2, devices.size());
    TEST_ASSERT_EQUAL_PTR(make_device_id(0), devices[0]());
    TEST_ASSERT_EQUAL_PTR(make_device_id(1), devices[1]());

    // Prevent release in the destructor
    devices[0]() = nullptr;
    devices[1]() = nullptr;
}

#if !defined(__APPLE__) && !defined(__MACOS)
// This is used to get a list of all platforms, so expect two calls
// First, return to say we have two platforms
// Then return the two platform id_s
static cl_int clGetPlatformIDs_testContextFromType(
    cl_uint num_entries,
    cl_platform_id *platforms,
    cl_uint *num_platforms,
    int num_calls)
{
    if (num_calls == 0)
    {
        TEST_ASSERT_NULL(platforms);
        TEST_ASSERT_NOT_NULL(num_platforms);
        *num_platforms = 2;
        return CL_SUCCESS;
    }
    else if (num_calls == 1)
    {
        TEST_ASSERT_NOT_NULL(platforms);
        TEST_ASSERT_EQUAL(2, num_entries);
        platforms[0] = make_platform_id(0);
        platforms[1] = make_platform_id(1);
        return CL_SUCCESS;
    }
    else
    {
        TEST_FAIL_MESSAGE("clGetPlatformIDs called too many times");
        return CL_INVALID_VALUE;
    }
}
#endif

#if !defined(__APPLE__) && !defined(__MACOS)
// Expect three calls to this
// 1. Platform 1, we have no GPUs
// 2. Platform 2, we have two GPUs
// 3. Here are the two cl_device_id's
static cl_int clGetDeviceIDs_testContextFromType(
    cl_platform_id  platform,
    cl_device_type  device_type,
    cl_uint  num_entries,
    cl_device_id  *devices,
    cl_uint  *num_devices,
    int num_calls)
{
    if (num_calls == 0)
    {
        TEST_ASSERT_EQUAL_PTR(make_platform_id(0), platform);
        TEST_ASSERT_EQUAL(CL_DEVICE_TYPE_GPU, device_type);
        TEST_ASSERT_NOT_NULL(num_devices);
        return CL_DEVICE_NOT_FOUND;
    }
    else if (num_calls == 1)
    {
        TEST_ASSERT_EQUAL_PTR(make_platform_id(1), platform);
        TEST_ASSERT_EQUAL(CL_DEVICE_TYPE_GPU, device_type);
        TEST_ASSERT_NOT_NULL(num_devices);
        *num_devices = 2;
        return CL_SUCCESS;
    }
    else if (num_calls == 2)
    {
        TEST_ASSERT_EQUAL_PTR(make_platform_id(1), platform);
        TEST_ASSERT_EQUAL(CL_DEVICE_TYPE_GPU, device_type);
        TEST_ASSERT_EQUAL(2, num_entries);
        TEST_ASSERT_NOT_NULL(devices);
        devices[0] = make_device_id(0);
        devices[1] = make_device_id(1);
        return CL_SUCCESS;
    }
    else
    {
        TEST_FAIL_MESSAGE("clGetDeviceIDs called too many times");
        return CL_INVALID_VALUE;
    }
}
#endif

// Stub for clCreateContextFromType
// - expect platform 1 with GPUs and non-null properties
static cl_context clCreateContextFromType_testContextFromType(
    const cl_context_properties  *properties,
    cl_device_type  device_type,
    void  (CL_CALLBACK *pfn_notify) (const char *errinfo,
    const void  *private_info,
    size_t  cb,
    void  *user_data),
    void  *user_data,
    cl_int  *errcode_ret,
    int num_calls)
{
    (void) pfn_notify;
    (void) user_data;
    (void) num_calls;

    TEST_ASSERT_EQUAL(CL_DEVICE_TYPE_GPU, device_type);
#if !defined(__APPLE__) && !defined(__MACOS)
    TEST_ASSERT_NOT_NULL(properties);
    TEST_ASSERT_EQUAL(CL_CONTEXT_PLATFORM, properties[0]);
    TEST_ASSERT_EQUAL(make_platform_id(1), properties[1]);
#else
    (void) properties;
#endif
    if (errcode_ret)
        *errcode_ret = CL_SUCCESS;
    return make_context(0);
}

void testContextFromType(void)
{
#if !defined(__APPLE__) && !defined(__MACOS)
    clGetPlatformIDs_StubWithCallback(clGetPlatformIDs_testContextFromType);
    clGetDeviceIDs_StubWithCallback(clGetDeviceIDs_testContextFromType);

    // The opencl.hpp header will perform an extra retain here to be consistent
    // with other APIs retaining runtime-owned objects before releasing them
    clRetainDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);
    clRetainDevice_ExpectAndReturn(make_device_id(1), CL_SUCCESS);

    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_2);

    // End of scope of vector of devices within constructor
    clReleaseDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);
    clReleaseDevice_ExpectAndReturn(make_device_id(1), CL_SUCCESS);
#endif

    clCreateContextFromType_StubWithCallback(clCreateContextFromType_testContextFromType);

    cl::Context context(CL_DEVICE_TYPE_GPU);
    TEST_ASSERT_EQUAL_PTR(make_context(0), context());

    clReleaseContext_ExpectAndReturn(make_context(0), CL_SUCCESS);
}

void testContextFromTypeNonNullProperties(void)
{
    clCreateContextFromType_StubWithCallback(clCreateContextFromType_testContextFromType);

    const cl_context_properties props[] = {
        CL_CONTEXT_PLATFORM, (cl_context_properties)make_platform_id(1), 0 };
    cl::Context context(CL_DEVICE_TYPE_GPU, props);
    TEST_ASSERT_EQUAL_PTR(make_context(0), context());

    clReleaseContext_ExpectAndReturn(make_context(0), CL_SUCCESS);
}

static cl_context clCreateContext_testContextNonNullProperties(
    const cl_context_properties* properties,
    cl_uint num_devices,
    const cl_device_id* devices,
    void  (CL_CALLBACK *pfn_notify) (const char *errinfo, const void  *private_info, size_t  cb, void  *user_data),
    void  *user_data,
    cl_int  *errcode_ret,
    int num_calls)
{
    (void) pfn_notify;
    (void) user_data;
    (void) num_calls;

    TEST_ASSERT_NOT_NULL(properties);
    TEST_ASSERT_GREATER_THAN(0, num_devices);
    for (int i = 0; i < (int)num_devices; i++) {
        TEST_ASSERT_EQUAL(make_device_id(i), devices[i]);
    }
    if (errcode_ret != nullptr)
        *errcode_ret = CL_SUCCESS;
    return make_context(0);
}

void testContextWithDeviceNonNullProperties(void)
{
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_1);
    clCreateContext_StubWithCallback(clCreateContext_testContextNonNullProperties);
    clReleaseContext_ExpectAndReturn(make_context(0), CL_SUCCESS);

    const cl_context_properties props[] = {
        CL_CONTEXT_PLATFORM, (cl_context_properties)make_platform_id(0), 0 };
    cl::Device device = cl::Device(make_device_id(0));

    cl::Context context(device, props);
    TEST_ASSERT_EQUAL_PTR(make_context(0), context());
}

void testContextWithDevicesNonNullProperties(void)
{
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_1);
    clCreateContext_StubWithCallback(clCreateContext_testContextNonNullProperties);
    clReleaseContext_ExpectAndReturn(make_context(0), CL_SUCCESS);

    const cl_context_properties props[] = {
        CL_CONTEXT_PLATFORM, (cl_context_properties)make_platform_id(0), 0 };
    cl::Device device0 = cl::Device(make_device_id(0));
    cl::Device device1 = cl::Device(make_device_id(1));

    cl::Context context({device0, device1}, props);
    TEST_ASSERT_EQUAL_PTR(make_context(0), context());
}

/****************************************************************************
 * Tests for cl::CommandQueue
 ****************************************************************************/

void testMoveAssignCommandQueueNonNull(void);
void testMoveAssignCommandQueueNull(void);
void testMoveConstructCommandQueueNonNull(void);
void testMoveConstructCommandQueueNull(void);
MAKE_MOVE_TESTS(CommandQueue, make_command_queue, clReleaseCommandQueue, commandQueuePool)

// Stub for clGetCommandQueueInfo that returns context 0
static cl_int clGetCommandQueueInfo_testCommandQueueGetContext(
    cl_command_queue command_queue,
    cl_command_queue_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    (void) num_calls;
    TEST_ASSERT_EQUAL_PTR(make_command_queue(0), command_queue);
    TEST_ASSERT_EQUAL_HEX(CL_QUEUE_CONTEXT, param_name);
    TEST_ASSERT(param_value == nullptr || param_value_size >= sizeof(cl_context));
    if (param_value_size_ret != nullptr)
        *param_value_size_ret = sizeof(cl_context);
    if (param_value != nullptr)
        *(cl_context *) param_value = make_context(0);
    return CL_SUCCESS;
}

void testCommandQueueGetContext(void)
{
    cl_context expected = make_context(0);
    int refcount = 1;

    clGetCommandQueueInfo_StubWithCallback(clGetCommandQueueInfo_testCommandQueueGetContext);
    prepare_contextRefcounts(1, &expected, &refcount);

    cl::Context ctx = commandQueuePool[0].getInfo<CL_QUEUE_CONTEXT>();
    TEST_ASSERT_EQUAL_PTR(expected, ctx());
    TEST_ASSERT_EQUAL(2, refcount);

    ctx() = nullptr;
}

// Stub for clGetCommandQueueInfo that returns device 0
static cl_int clGetCommandQueueInfo_testCommandQueueGetDevice(
    cl_command_queue command_queue,
    cl_command_queue_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    (void) num_calls;
    TEST_ASSERT_EQUAL_PTR(make_command_queue(0), command_queue);
    TEST_ASSERT_EQUAL_HEX(CL_QUEUE_DEVICE, param_name);
    TEST_ASSERT(param_value == nullptr || param_value_size >= sizeof(cl_device_id));
    if (param_value_size_ret != nullptr)
        *param_value_size_ret = sizeof(cl_device_id);
    if (param_value != nullptr)
        *(cl_device_id *) param_value = make_device_id(0);
    return CL_SUCCESS;
}

void testCommandQueueGetDevice1_1(void)
{
    cl_device_id expected = make_device_id(0);

    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_1);
    clGetCommandQueueInfo_StubWithCallback(clGetCommandQueueInfo_testCommandQueueGetDevice);

    cl::Device device = commandQueuePool[0].getInfo<CL_QUEUE_DEVICE>();
    TEST_ASSERT_EQUAL_PTR(expected, device());

    device() = nullptr;
}

void testCommandQueueGetDevice1_2(void)
{
    cl_device_id expected = make_device_id(0);
    int refcount = 1;

    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_2);
    clGetCommandQueueInfo_StubWithCallback(clGetCommandQueueInfo_testCommandQueueGetDevice);
    prepare_deviceRefcounts(1, &expected, &refcount);

    cl::Device device = commandQueuePool[0].getInfo<CL_QUEUE_DEVICE>();
    TEST_ASSERT_EQUAL_PTR(expected, device());
    TEST_ASSERT_EQUAL(2, refcount);

    device() = nullptr;
}

#if CL_HPP_TARGET_OPENCL_VERSION < 200
// stub for clCreateCommandQueue - returns queue zero
static cl_command_queue clCreateCommandQueue_testCommandQueueFromSpecifiedContext(
    cl_context context,
    cl_device_id device,
    cl_command_queue_properties properties,
    cl_int *errcode_ret,
    int num_calls)
{
    (void) num_calls;
    TEST_ASSERT_EQUAL_PTR(make_context(0), context);
    TEST_ASSERT_EQUAL_PTR(make_device_id(0), device);
    TEST_ASSERT(properties == 0);
    if (errcode_ret != nullptr)
        *errcode_ret = CL_SUCCESS;
    return make_command_queue(0);
}
#else
// stub for clCreateCommandQueueWithProperties - returns queue zero
static cl_command_queue clCreateCommandQueueWithProperties_testCommandQueueFromSpecifiedContext(
    cl_context context,
    cl_device_id device,
    const cl_queue_properties *properties,
    cl_int *errcode_ret,
    int num_calls)
{
    (void)num_calls;
    TEST_ASSERT_EQUAL_PTR(make_context(0), context);
    TEST_ASSERT_EQUAL_PTR(make_device_id(0), device);
    TEST_ASSERT(properties[0] == CL_QUEUE_PROPERTIES);
    TEST_ASSERT(properties[1] == 0);
    if (errcode_ret != nullptr)
        *errcode_ret = CL_SUCCESS;
    return make_command_queue(0);
}
#endif // #if CL_HPP_TARGET_OPENCL_VERSION < 200

void testCommandQueueFromSpecifiedContext(void)
{
    cl_command_queue expected = make_command_queue(0);
    cl_context expected_context =  make_context(0);
    cl_device_id expected_device = make_device_id(0);

    int context_refcount = 1;
    int device_refcount = 1;
    prepare_contextRefcounts(1, &expected_context, &context_refcount);
    prepare_deviceRefcounts(1, &expected_device, &device_refcount);

    // This is the context we will pass in to test
    cl::Context context = contextPool[0];

    // Assumes the context contains the fi rst device
    clGetContextInfo_StubWithCallback(clGetContextInfo_device);

#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    clCreateCommandQueueWithProperties_StubWithCallback(clCreateCommandQueueWithProperties_testCommandQueueFromSpecifiedContext);
#else // #if CL_HPP_TARGET_OPENCL_VERSION >= 200
    clCreateCommandQueue_StubWithCallback(clCreateCommandQueue_testCommandQueueFromSpecifiedContext);
#endif // #if CL_HPP_TARGET_OPENCL_VERSION >= 200
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);

#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_2_0);
#else // #if CL_HPP_TARGET_OPENCL_VERSION >= 200
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_2);
#endif // #if CL_HPP_TARGET_OPENCL_VERSION >= 200
    clReleaseCommandQueue_ExpectAndReturn(expected, CL_SUCCESS);

    cl::CommandQueue queue(context);
    TEST_ASSERT_EQUAL_PTR(expected, queue());

    // Context not destroyed yet
    TEST_ASSERT_EQUAL(2, context_refcount);
    // Device object destroyed at end of scope
    TEST_ASSERT_EQUAL(1, device_refcount);

}

/****************************************************************************
 * Tests for cl::Device
 ****************************************************************************/

void testCopyDeviceNonNull1_1(void)
{
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_1);

    cl::Device d0(make_device_id(0));
    cl::Device d1(make_device_id(1));
    d0 = d1;
}

void testCopyDeviceNonNull1_2(void)
{
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_2);
    clReleaseDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);
    clRetainDevice_ExpectAndReturn(make_device_id(1), CL_SUCCESS);

    cl::Device d0(make_device_id(0));
    cl::Device d1(make_device_id(1));
    d0 = d1;

    // Prevent destructor from interfering with the test
    d0() = nullptr;
    d1() = nullptr;
}

void testCopyDeviceFromNull1_1(void)
{
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_1);
    // No other calls expected

    cl::Device d(make_device_id(0));
    d = cl::Device();
}

void testCopyDeviceFromNull1_2(void)
{
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_2);
    clReleaseDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);

    cl::Device d(make_device_id(0));
    d = cl::Device();
}

void testCopyDeviceToNull1_1(void)
{
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_1);
    // No other calls expected

    cl::Device d0;
    cl::Device d1(make_device_id(0));
    d0 = d1;
}

void testCopyDeviceToNull1_2(void)
{
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_2);
    clRetainDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);

    cl::Device d0;
    cl::Device d1(make_device_id(0));
    d0 = d1;

    // Prevent destructor from interfering with the test
    d0() = nullptr;
    d1() = nullptr;
}

void testCopyDeviceSelf(void)
{
    // Use 1.2 to check the retain/release calls
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_2);
    clReleaseDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);
    clRetainDevice_ExpectAndReturn(make_device_id(1), CL_SUCCESS);

    cl::Device d0(make_device_id(0));
    cl::Device d1(make_device_id(1));
    d0 = d1;

    // Prevent destructor from interfering with the test
    d0() = nullptr;
    d1() = nullptr;
}

void testAssignDeviceNull(void)
{
    // Any version will do here
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_2);
    clReleaseDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);

    cl::Device d(make_device_id(0));
    d = (cl_device_id) nullptr;
}

// These tests do not use the MAKE_MOVE_TESTS helper because they need to
// check whether the device is reference-countable, and to check that
// the reference-countable flag is correctly moved.
void testMoveAssignDeviceNonNull(void)
{
#ifdef TEST_RVALUE_REFERENCES
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_2);

    // Release called when trg overwritten
    clReleaseDevice_ExpectAndReturn(make_device_id(1), CL_SUCCESS);

    cl::Device src(make_device_id(0));
    cl::Device trg(make_device_id(1));
    trg = std::move(src);
    TEST_ASSERT_EQUAL_PTR(make_device_id(0), trg());
    TEST_ASSERT_NULL(src());

    // Prevent destructor from interfering with the test
    trg() = nullptr;
#endif
}

void testMoveAssignDeviceNull(void)
{
#ifdef TEST_RVALUE_REFERENCES
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_2);

    cl::Device trg;
    cl::Device src(make_device_id(1));
    trg = std::move(src);
    TEST_ASSERT_EQUAL_PTR(make_device_id(1), trg());
    TEST_ASSERT_NULL(src());

    // Prevent destructor from interfering with the test
    trg() = nullptr;
#endif
}

void testMoveConstructDeviceNonNull(void)
{
#ifdef TEST_RVALUE_REFERENCES
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_2);

    cl::Device src(make_device_id(0));
    cl::Device trg(std::move(src));
    TEST_ASSERT_EQUAL_PTR(make_device_id(0), trg());
    TEST_ASSERT_NULL(src());

    // Prevent destructor from interfering with the test
    trg() = nullptr;
#endif
}

void testMoveConstructDeviceNull(void)
{
#ifdef TEST_RVALUE_REFERENCES
    cl::Device empty;
    cl::Device trg(std::move(empty));
    TEST_ASSERT_NULL(trg());
    TEST_ASSERT_NULL(empty());
#endif
}

void testDestroyDevice1_1(void)
{
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_1);
    // No other calls expected

    cl::Device d(make_device_id(0));
}

void testDestroyDevice1_2(void)
{
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_2);
    clReleaseDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);

    cl::Device d(make_device_id(0));
}

static cl_int clGetDeviceIDs_PlatformWithZeroDevices(
    cl_platform_id  platform,
    cl_device_type  device_type,
    cl_uint  num_entries,
    cl_device_id  *devices,
    cl_uint  *num_devices,
    int num_calls)
{
    (void) num_entries;
    (void) devices;

    if (num_calls == 0)
    {
        TEST_ASSERT_EQUAL_PTR(make_platform_id(0), platform);
        TEST_ASSERT_EQUAL(CL_DEVICE_TYPE_ALL, device_type);
        TEST_ASSERT_NOT_NULL(num_devices);
        return CL_DEVICE_NOT_FOUND;
    }
    else
    {
        TEST_FAIL_MESSAGE("clGetDeviceIDs called too many times");
        return CL_INVALID_VALUE;
    }
}

void testPlatformWithZeroDevices(void)
{
    clGetDeviceIDs_StubWithCallback(clGetDeviceIDs_PlatformWithZeroDevices);

    cl::Platform p(make_platform_id(0));
    std::vector<cl::Device> devices;

    cl_int errCode = p.getDevices(CL_DEVICE_TYPE_ALL, &devices);
    TEST_ASSERT_EQUAL(CL_SUCCESS, errCode);
    TEST_ASSERT_EQUAL(0, devices.size());
}

/****************************************************************************
 * Tests for cl::Buffer
 ****************************************************************************/

void testMoveAssignBufferNonNull(void);
void testMoveAssignBufferNull(void);
void testMoveConstructBufferNonNull(void);
void testMoveConstructBufferNull(void);
MAKE_MOVE_TESTS(Buffer, make_mem, clReleaseMemObject, bufferPool)

// Stub of clCreateBuffer for testBufferConstructorContextInterator
// - return the first memory location

static cl_mem clCreateBuffer_testBufferConstructorContextIterator(
    cl_context context,
    cl_mem_flags flags,
    size_t size,
    void *host_ptr,
    cl_int *errcode_ret,
    int num_calls)
{
    (void) num_calls;

    TEST_ASSERT_EQUAL_PTR(make_context(0), context);
    TEST_ASSERT_BITS(CL_MEM_COPY_HOST_PTR, flags, !CL_MEM_COPY_HOST_PTR);
    TEST_ASSERT_BITS(CL_MEM_READ_ONLY, flags, CL_MEM_READ_ONLY);
    TEST_ASSERT_EQUAL(sizeof(int)*1024, size);
    TEST_ASSERT_NULL(host_ptr);
    if (errcode_ret)
        *errcode_ret = CL_SUCCESS;
    return make_mem(0);
}

// Declare forward these functions
static void * clEnqueueMapBuffer_testCopyHostToBuffer(
    cl_command_queue command_queue,
    cl_mem buffer,
    cl_bool blocking_map,
    cl_map_flags map_flags,
    size_t offset,
    size_t size,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event,
    cl_int *errcode_ret,
    int num_calls);

static cl_int clEnqueueUnmapMemObject_testCopyHostToBuffer(
    cl_command_queue  command_queue ,
    cl_mem  memobj,
    void  *mapped_ptr,
    cl_uint  num_events_in_wait_list ,
    const cl_event  *event_wait_list ,
    cl_event  *event,
    int num_calls);

static cl_int clWaitForEvents_testCopyHostToBuffer(
    cl_uint num_events,
    const cl_event *event_list,
    int num_calls);

static cl_int clReleaseEvent_testCopyHostToBuffer(
    cl_event event,
    int num_calls);

void testBufferConstructorContextIterator(void)
{
    cl_mem expected = make_mem(0);

    // Assume this context includes make_device_id(0) for stub clGetContextInfo_device
    cl::Context context(make_context(0));

    clCreateBuffer_StubWithCallback(clCreateBuffer_testBufferConstructorContextIterator);
    clGetContextInfo_StubWithCallback(clGetContextInfo_device);
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_2_0);
#else // #if CL_HPP_TARGET_OPENCL_VERSION >= 200
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_2);
#endif //#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    clRetainDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);

#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    clCreateCommandQueueWithProperties_StubWithCallback(clCreateCommandQueueWithProperties_testCommandQueueFromSpecifiedContext);
#else // #if CL_HPP_TARGET_OPENCL_VERSION >= 200
    clCreateCommandQueue_StubWithCallback(clCreateCommandQueue_testCommandQueueFromSpecifiedContext);
#endif //#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    clReleaseDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);
    clEnqueueMapBuffer_StubWithCallback(clEnqueueMapBuffer_testCopyHostToBuffer);
    clEnqueueUnmapMemObject_StubWithCallback(clEnqueueUnmapMemObject_testCopyHostToBuffer);
    clWaitForEvents_StubWithCallback(clWaitForEvents_testCopyHostToBuffer);
    clReleaseEvent_StubWithCallback(clReleaseEvent_testCopyHostToBuffer);
    clReleaseCommandQueue_ExpectAndReturn(make_command_queue(0), CL_SUCCESS);

    std::vector<int> host(1024);

    cl::Buffer buffer(context, host.begin(), host.end(), true);

    TEST_ASSERT_EQUAL_PTR(expected, buffer());

    // Tidy up at end of test
    clReleaseMemObject_ExpectAndReturn(expected, CL_SUCCESS);
    clReleaseContext_ExpectAndReturn(make_context(0), CL_SUCCESS);
}

void testBufferConstructorQueueIterator(void)
{
    cl_context expected_context = make_context(0);
    int context_refcount = 1;
    cl_mem expected = make_mem(0);

    cl::CommandQueue queue(make_command_queue(0));

    prepare_contextRefcounts(1, &expected_context, &context_refcount);
    clGetCommandQueueInfo_StubWithCallback(clGetCommandQueueInfo_context);
    clCreateBuffer_StubWithCallback(clCreateBuffer_testBufferConstructorContextIterator);

    clEnqueueMapBuffer_StubWithCallback(clEnqueueMapBuffer_testCopyHostToBuffer);
    clEnqueueUnmapMemObject_StubWithCallback(clEnqueueUnmapMemObject_testCopyHostToBuffer);
    clWaitForEvents_StubWithCallback(clWaitForEvents_testCopyHostToBuffer);
    clReleaseEvent_StubWithCallback(clReleaseEvent_testCopyHostToBuffer);

    std::vector<int> host(1024);

    cl::Buffer buffer(queue, host.begin(), host.end(), true);

    TEST_ASSERT_EQUAL_PTR(expected, buffer());
    TEST_ASSERT_EQUAL(1, context_refcount);

    // Tidy up at end of test
    clReleaseMemObject_ExpectAndReturn(expected, CL_SUCCESS);
    clReleaseCommandQueue_ExpectAndReturn(make_command_queue(0), CL_SUCCESS);
}

static cl_mem clCreateBufferWithProperties_testBufferWithProperties(
    cl_context context,
    const cl_mem_properties *properties,
    cl_mem_flags flags,
    size_t size,
    void *host_ptr,
    cl_int *errcode_ret,
    int num_calls)
{
    TEST_ASSERT_EQUAL(0, num_calls);
    TEST_ASSERT_EQUAL_PTR(contextPool[0](), context);
    TEST_ASSERT_NOT_NULL(properties);
    TEST_ASSERT_EQUAL(11, *properties);
    TEST_ASSERT_EQUAL(0, flags);
    TEST_ASSERT_EQUAL(0, size);
    TEST_ASSERT_NULL(host_ptr);
    if (errcode_ret)
        *errcode_ret = CL_SUCCESS;

    return make_mem(0);
}

void testBufferWithProperties(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 300
    clCreateBufferWithProperties_StubWithCallback(clCreateBufferWithProperties_testBufferWithProperties);

    VECTOR_CLASS<cl_mem_properties> props{11};
    cl_int err;
    cl::Buffer buffer(contextPool[0], props, 0, 0, nullptr, &err);

    TEST_ASSERT_EQUAL_PTR(make_mem(0), buffer());
    TEST_ASSERT_EQUAL(CL_SUCCESS, err);

    // prevent destructor from interfering with the test
    buffer() = nullptr;
#endif
}

/****************************************************************************
 * Tests for cl::Image1DBuffer
 ****************************************************************************/

/**
 * Stub for querying CL_IMAGE_BUFFER and returning make_mem(1).
 */
cl_int clGetImageInfo_testGetImageInfoBuffer(
    cl_mem image, cl_image_info param_name,
    size_t param_value_size, void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    TEST_ASSERT_EQUAL(0, num_calls);
    TEST_ASSERT_EQUAL_PTR(make_mem(0), image);
    TEST_ASSERT_EQUAL_HEX(CL_IMAGE_BUFFER, param_name);
    TEST_ASSERT_EQUAL(sizeof(cl_mem), param_value_size);

    if (param_value != nullptr)
    {
        *(cl_mem *) param_value = make_mem(1);
    }
    if (param_value_size_ret != nullptr)
        *param_value_size_ret = sizeof(cl_mem);
    return CL_SUCCESS;
}

void testGetImageInfoBuffer(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    cl_mem expected = make_mem(1);
    int refcount = 1;

    clGetImageInfo_StubWithCallback(clGetImageInfo_testGetImageInfoBuffer);
    prepare_memRefcounts(1, &expected, &refcount);

    cl::Image1DBuffer image(make_mem(0));
    const cl::Buffer &buffer = image.getImageInfo<CL_IMAGE_BUFFER>();
    TEST_ASSERT_EQUAL_PTR(make_mem(1), buffer());
    // Ref count should be 2 here because buffer has not been destroyed yet
    TEST_ASSERT_EQUAL(2, refcount);

    // prevent destructor from interfering with the test
    image() = nullptr;
#endif
}

/**
 * Stub for querying CL_IMAGE_BUFFER and returning nullptr.
 */
cl_int clGetImageInfo_testGetImageInfoBufferNull(
    cl_mem image, cl_image_info param_name,
    size_t param_value_size, void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    TEST_ASSERT_EQUAL(0, num_calls);
    TEST_ASSERT_EQUAL_PTR(make_mem(0), image);
    TEST_ASSERT_EQUAL_HEX(CL_IMAGE_BUFFER, param_name);
    TEST_ASSERT_EQUAL(sizeof(cl_mem), param_value_size);

    if (param_value != nullptr)
    {
        *(cl_mem *) param_value = nullptr;
    }
    if (param_value_size_ret != nullptr)
        *param_value_size_ret = sizeof(cl_mem);
    return CL_SUCCESS;
}

void testGetImageInfoBufferNull(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    clGetImageInfo_StubWithCallback(clGetImageInfo_testGetImageInfoBufferNull);

    cl::Image2D image(make_mem(0));
    cl::Buffer buffer = image.getImageInfo<CL_IMAGE_BUFFER>();
    TEST_ASSERT_NULL(buffer());

    // prevent destructor from interfering with the test
    image() = nullptr;
#endif
}

void testGetImageInfoBufferOverwrite(void)
{
    clGetImageInfo_StubWithCallback(clGetImageInfo_testGetImageInfoBuffer);
    clReleaseMemObject_ExpectAndReturn(make_mem(2), CL_SUCCESS);
    clRetainMemObject_ExpectAndReturn(make_mem(1), CL_SUCCESS);

    cl::Image2D image(make_mem(0));
    cl::Buffer buffer(make_mem(2));
    cl_int status = image.getImageInfo(CL_IMAGE_BUFFER, &buffer);
    TEST_ASSERT_EQUAL(CL_SUCCESS, status);
    TEST_ASSERT_EQUAL_PTR(make_mem(1), buffer());

    // prevent destructor from interfering with the test
    image() = nullptr;
    buffer() = nullptr;
}

/**
 * A stub for clCreateImage that creates an image from a buffer
 * passing the buffer's cl_mem straight through.
 */
cl_mem clCreateImage_image1dbuffer(
    cl_context context,
    cl_mem_flags flags,
    const cl_image_format *image_format,
    const cl_image_desc *image_desc,
    void *host_ptr,
    cl_int *errcode_ret,
    int num_calls)
{
    (void) context;
    (void) flags;
    (void) host_ptr;
    (void) num_calls;

    TEST_ASSERT_NOT_NULL(image_format);
    TEST_ASSERT_NOT_NULL(image_desc);
    TEST_ASSERT_EQUAL_HEX(CL_MEM_OBJECT_IMAGE1D_BUFFER, image_desc->image_type);

    if (errcode_ret != nullptr)
        *errcode_ret = CL_SUCCESS;

    // Return the passed buffer as the cl_mem
    return image_desc->buffer;
}

void testConstructImageFromBuffer(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 120
    const size_t width = 64;
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_2);
    clCreateImage_StubWithCallback(clCreateImage_image1dbuffer);
    clReleaseMemObject_ExpectAndReturn(make_mem(0), CL_SUCCESS);
    clReleaseContext_ExpectAndReturn(make_context(0), CL_SUCCESS);

    cl::Context context(make_context(0));
    cl::Buffer buffer(make_mem(0));
    cl::Image1DBuffer image(
        context,
        CL_MEM_READ_ONLY,
        cl::ImageFormat(CL_R, CL_SIGNED_INT32),
        width,
        buffer);

    // Check that returned buffer matches the original
    TEST_ASSERT_EQUAL_PTR(buffer(), image());

    buffer() = nullptr;
#endif
}

/****************************************************************************
 * Tests for cl::Image2D
 ****************************************************************************/

void testMoveAssignImage2DNonNull(void);
void testMoveAssignImage2DNull(void);
void testMoveConstructImage2DNonNull(void);
void testMoveConstructImage2DNull(void);
MAKE_MOVE_TESTS(Image2D, make_mem, clReleaseMemObject, image2DPool)

#ifdef CL_USE_DEPRECATED_OPENCL_1_1_APIS
static cl_mem clCreateImage2D_testCreateImage2D_1_1(
    cl_context context,
    cl_mem_flags flags,
    const cl_image_format *image_format,
    size_t image_width,
    size_t image_height,
    size_t image_row_pitch,
    void *host_ptr,
    cl_int *errcode_ret,
    int num_calls)
{
    TEST_ASSERT_EQUAL(0, num_calls);
    TEST_ASSERT_EQUAL_PTR(make_context(0), context);
    TEST_ASSERT_EQUAL_HEX(CL_MEM_READ_WRITE, flags);

    TEST_ASSERT_NOT_NULL(image_format);
    TEST_ASSERT_EQUAL_HEX(CL_R, image_format->image_channel_order);
    TEST_ASSERT_EQUAL_HEX(CL_FLOAT, image_format->image_channel_data_type);

    TEST_ASSERT_EQUAL(64, image_width);
    TEST_ASSERT_EQUAL(32, image_height);
    TEST_ASSERT_EQUAL(256, image_row_pitch);
    TEST_ASSERT_NULL(host_ptr);

    if (errcode_ret != nullptr)
        *errcode_ret = CL_SUCCESS;
    return make_mem(0);
}
#endif

void testCreateImage2D_1_1(void)
{
#ifdef CL_USE_DEPRECATED_OPENCL_1_1_APIS
    clGetContextInfo_StubWithCallback(clGetContextInfo_device);
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_1);
    clCreateImage2D_StubWithCallback(clCreateImage2D_testCreateImage2D_1_1);

    cl_int err;
    cl::Context context;
    context() = make_context(0);
    cl::Image2D image(
        context, CL_MEM_READ_WRITE,
        cl::ImageFormat(CL_R, CL_FLOAT), 64, 32, 256, nullptr, &err);

    TEST_ASSERT_EQUAL(CL_SUCCESS, err);
    TEST_ASSERT_EQUAL_PTR(make_mem(0), image());

    context() = nullptr;
    image() = nullptr;
#endif
}

static cl_mem clCreateImage_testCreateImage2D_1_2(
    cl_context context,
    cl_mem_flags flags,
    const cl_image_format *image_format,
    const cl_image_desc *image_desc,
    void *host_ptr,
    cl_int *errcode_ret,
    int num_calls)
{
    TEST_ASSERT_EQUAL(0, num_calls);
    TEST_ASSERT_EQUAL_PTR(make_context(0), context);
    TEST_ASSERT_EQUAL_HEX(CL_MEM_READ_WRITE, flags);

    TEST_ASSERT_NOT_NULL(image_format);
    TEST_ASSERT_EQUAL_HEX(CL_R, image_format->image_channel_order);
    TEST_ASSERT_EQUAL_HEX(CL_FLOAT, image_format->image_channel_data_type);

    TEST_ASSERT_NOT_NULL(image_desc);
    TEST_ASSERT_EQUAL_HEX(CL_MEM_OBJECT_IMAGE2D, image_desc->image_type);
    TEST_ASSERT_EQUAL(64, image_desc->image_width);
    TEST_ASSERT_EQUAL(32, image_desc->image_height);
    TEST_ASSERT_EQUAL(256, image_desc->image_row_pitch);
    TEST_ASSERT_EQUAL(0, image_desc->num_mip_levels);
    TEST_ASSERT_EQUAL(0, image_desc->num_samples);
    TEST_ASSERT_NULL(image_desc->buffer);

    TEST_ASSERT_NULL(host_ptr);

    if (errcode_ret != nullptr)
        *errcode_ret = CL_SUCCESS;
    return make_mem(0);
}

void testCreateImage2D_1_2(void)
{
    clGetContextInfo_StubWithCallback(clGetContextInfo_device);
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_2);
    clCreateImage_StubWithCallback(clCreateImage_testCreateImage2D_1_2);

    cl_int err;
    cl::Context context;
    context() = make_context(0);
    cl::Image2D image(
        context, CL_MEM_READ_WRITE,
        cl::ImageFormat(CL_R, CL_FLOAT), 64, 32, 256, nullptr, &err);

    TEST_ASSERT_EQUAL(CL_SUCCESS, err);
    TEST_ASSERT_EQUAL_PTR(make_mem(0), image());

    context() = nullptr;
    image() = nullptr;
}

/****************************************************************************
 * Tests for cl::Image3D
 ****************************************************************************/

void testMoveAssignImage3DNonNull(void);
void testMoveAssignImage3DNull(void);
void testMoveConstructImage3DNonNull(void);
void testMoveConstructImage3DNull(void);
MAKE_MOVE_TESTS(Image3D, make_mem, clReleaseMemObject, image3DPool)

#ifdef CL_USE_DEPRECATED_OPENCL_1_1_APIS
static cl_mem clCreateImage3D_testCreateImage3D_1_1(
    cl_context context,
    cl_mem_flags flags,
    const cl_image_format *image_format,
    size_t image_width,
    size_t image_height,
    size_t image_depth,
    size_t image_row_pitch,
    size_t image_slice_pitch,
    void *host_ptr,
    cl_int *errcode_ret,
    int num_calls)
{
    TEST_ASSERT_EQUAL(0, num_calls);
    TEST_ASSERT_EQUAL_PTR(make_context(0), context);
    TEST_ASSERT_EQUAL_HEX(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, flags);

    TEST_ASSERT_NOT_NULL(image_format);
    TEST_ASSERT_EQUAL_HEX(CL_R, image_format->image_channel_order);
    TEST_ASSERT_EQUAL_HEX(CL_FLOAT, image_format->image_channel_data_type);

    TEST_ASSERT_EQUAL(64, image_width);
    TEST_ASSERT_EQUAL(32, image_height);
    TEST_ASSERT_EQUAL(16, image_depth);
    TEST_ASSERT_EQUAL(256, image_row_pitch);
    TEST_ASSERT_EQUAL(65536, image_slice_pitch);
    TEST_ASSERT_EQUAL_PTR((void *)(size_t)0xdeadbeef, host_ptr);

    if (errcode_ret != nullptr)
        *errcode_ret = CL_SUCCESS;
    return make_mem(0);
}
#endif

void testCreateImage3D_1_1(void)
{
#ifdef CL_USE_DEPRECATED_OPENCL_1_1_APIS
    clGetContextInfo_StubWithCallback(clGetContextInfo_device);
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_1);
    clCreateImage3D_StubWithCallback(clCreateImage3D_testCreateImage3D_1_1);

    cl_int err;
    cl::Context context;
    context() = make_context(0);
    cl::Image3D image(
        context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        cl::ImageFormat(CL_R, CL_FLOAT), 64, 32, 16, 256, 65536, (void *)(size_t)0xdeadbeef, &err);

    TEST_ASSERT_EQUAL(CL_SUCCESS, err);
    TEST_ASSERT_EQUAL_PTR(make_mem(0), image());

    context() = nullptr;
    image() = nullptr;
#endif
}

static cl_mem clCreateImage_testCreateImage3D_1_2(
    cl_context context,
    cl_mem_flags flags,
    const cl_image_format *image_format,
    const cl_image_desc *image_desc,
    void *host_ptr,
    cl_int *errcode_ret,
    int num_calls)
{
    TEST_ASSERT_EQUAL(0, num_calls);
    TEST_ASSERT_EQUAL_PTR(make_context(0), context);
    TEST_ASSERT_EQUAL_HEX(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, flags);

    TEST_ASSERT_NOT_NULL(image_format);
    TEST_ASSERT_EQUAL_HEX(CL_R, image_format->image_channel_order);
    TEST_ASSERT_EQUAL_HEX(CL_FLOAT, image_format->image_channel_data_type);

    TEST_ASSERT_NOT_NULL(image_desc);
    TEST_ASSERT_EQUAL_HEX(CL_MEM_OBJECT_IMAGE3D, image_desc->image_type);
    TEST_ASSERT_EQUAL(64, image_desc->image_width);
    TEST_ASSERT_EQUAL(32, image_desc->image_height);
    TEST_ASSERT_EQUAL(16, image_desc->image_depth);
    TEST_ASSERT_EQUAL(256, image_desc->image_row_pitch);
    TEST_ASSERT_EQUAL(65536, image_desc->image_slice_pitch);
    TEST_ASSERT_EQUAL(0, image_desc->num_mip_levels);
    TEST_ASSERT_EQUAL(0, image_desc->num_samples);
    TEST_ASSERT_NULL(image_desc->buffer);

    TEST_ASSERT_EQUAL_PTR((void *)(size_t)0xdeadbeef, host_ptr);

    if (errcode_ret != nullptr)
        *errcode_ret = CL_SUCCESS;
    return make_mem(0);
}

void testCreateImage3D_1_2(void)
{
    clGetContextInfo_StubWithCallback(clGetContextInfo_device);
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_2);
    clCreateImage_StubWithCallback(clCreateImage_testCreateImage3D_1_2);

    cl_int err;
    cl::Context context;
    context() = make_context(0);
    cl::Image3D image(
        context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        cl::ImageFormat(CL_R, CL_FLOAT), 64, 32, 16, 256, 65536, (void *)(size_t)0xdeadbeef, &err);

    TEST_ASSERT_EQUAL(CL_SUCCESS, err);
    TEST_ASSERT_EQUAL_PTR(make_mem(0), image());

    context() = nullptr;
    image() = nullptr;
}

/****************************************************************************
 * Tests for cl::Kernel
 ****************************************************************************/
void testMoveAssignKernelNonNull(void);
void testMoveAssignKernelNull(void);
void testMoveConstructKernelNonNull(void);
void testMoveConstructKernelNull(void);
MAKE_MOVE_TESTS(Kernel, make_kernel, clReleaseKernel, kernelPool)

static cl_int scalarArg;
static cl_int3 vectorArg;

void testKernelSetArgScalar(void)
{
    scalarArg = 0xcafebabe;
    clSetKernelArg_ExpectAndReturn(make_kernel(0), 3, 4, &scalarArg, CL_SUCCESS);
    kernelPool[0].setArg(3, scalarArg);
}

void testKernelSetArgVector(void)
{
    vectorArg.s[0] = 0x12345678;
    vectorArg.s[1] = 0x23456789;
    vectorArg.s[2] = 0x87654321;
    clSetKernelArg_ExpectAndReturn(make_kernel(0), 2, 16, &vectorArg, CL_SUCCESS);
    kernelPool[0].setArg(2, vectorArg);
}

void testKernelSetArgMem(void)
{
    clSetKernelArg_ExpectAndReturn(make_kernel(0), 1, sizeof(cl_mem), &bufferPool[1](), CL_SUCCESS);
    kernelPool[0].setArg(1, bufferPool[1]);
}

void testKernelSetArgLocal(void)
{
    clSetKernelArg_ExpectAndReturn(make_kernel(0), 2, 123, nullptr, CL_SUCCESS);
    kernelPool[0].setArg(2, cl::Local(123));
}

void testKernelSetArgBySetKernelArgSVMPointerWithUniquePtrType()
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    std::unique_ptr<int> buffer(new int(1000));
    clSetKernelArgSVMPointer_ExpectAndReturn(make_kernel(0), 1, buffer.get(), CL_SUCCESS);
    TEST_ASSERT_EQUAL(kernelPool[0].setArg(1, buffer), CL_SUCCESS);
#endif
}

void testKernelSetArgBySetKernelArgSVMPointerWithVectorType()
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    VECTOR_CLASS<int> vec(1000);
    clSetKernelArgSVMPointer_ExpectAndReturn(make_kernel(1), 2, vec.data(), CL_SUCCESS);
    TEST_ASSERT_EQUAL(kernelPool[1].setArg(2, vec), CL_SUCCESS);
#endif
}

void testKernelSetArgBySetKernelArgSVMPointerWithPointerType()
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    cl_mem *memory = &bufferPool[1]();
    clSetKernelArgSVMPointer_ExpectAndReturn(make_kernel(2), 3, &bufferPool[1](), CL_SUCCESS);
    TEST_ASSERT_EQUAL(kernelPool[2].setArg(3, memory), CL_SUCCESS);
#endif
}

void testKernelSetExecInfo(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    cl_bool val = CL_TRUE;
    // Using CL_KERNEL_EXEC_INFO_SVM_FINE_GRAIN_SYSTEM in the tests since it's
    // defined by the core spec but this function is particularly useful for
    // vendor extensions.
    clSetKernelExecInfo_ExpectAndReturn(make_kernel(0),
                                        CL_KERNEL_EXEC_INFO_SVM_FINE_GRAIN_SYSTEM,
                                        sizeof(cl_bool), &val, CL_SUCCESS);
    kernelPool[0].setExecInfo(CL_KERNEL_EXEC_INFO_SVM_FINE_GRAIN_SYSTEM, val);
    // Also test the typesafe version
    clSetKernelExecInfo_ExpectAndReturn(make_kernel(0),
                                        CL_KERNEL_EXEC_INFO_SVM_FINE_GRAIN_SYSTEM,
                                        sizeof(cl_bool), &val, CL_SUCCESS);
    kernelPool[0].setExecInfo<CL_KERNEL_EXEC_INFO_SVM_FINE_GRAIN_SYSTEM>(val);
#endif
}

/****************************************************************************
 * Tests for cl::copy
 ****************************************************************************/

// This method should allocate some host accesible memory
// so we must do this ourselves
void *some_host_memory;

static void * clEnqueueMapBuffer_testCopyHostToBuffer(
    cl_command_queue command_queue,
    cl_mem buffer,
    cl_bool blocking_map,
    cl_map_flags map_flags,
    size_t offset,
    size_t size,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event,
    cl_int *errcode_ret,
    int num_calls)
{
    (void) offset;
    (void) num_events_in_wait_list;
    (void) event_wait_list;
    (void) num_calls;

    TEST_ASSERT_EQUAL_PTR(make_command_queue(0), command_queue);
    TEST_ASSERT_EQUAL_PTR(make_mem(0), buffer);
    TEST_ASSERT_EQUAL(CL_TRUE, blocking_map);
    TEST_ASSERT_EQUAL(CL_MAP_WRITE, map_flags);
    TEST_ASSERT_EQUAL(sizeof(int)*1024, size);

    some_host_memory = malloc(sizeof(int) * 1024);

    // Set the return event
    if (event)
        *event = nullptr;

    // Set the return error code
    if (errcode_ret)
        *errcode_ret = CL_SUCCESS;

    return some_host_memory;
}

static cl_int clEnqueueUnmapMemObject_testCopyHostToBuffer(
    cl_command_queue  command_queue ,
    cl_mem  memobj,
    void  *mapped_ptr,
    cl_uint  num_events_in_wait_list ,
    const cl_event  *event_wait_list ,
    cl_event  *event,
    int num_calls)
{
    (void) num_events_in_wait_list;
    (void) event_wait_list;
    (void) num_calls;

    TEST_ASSERT_EQUAL_PTR(make_command_queue(0), command_queue);
    TEST_ASSERT_EQUAL_PTR(make_mem(0), memobj);
    TEST_ASSERT_EQUAL_PTR(some_host_memory, mapped_ptr);
    TEST_ASSERT_NOT_NULL(event);
    return CL_SUCCESS;
}

static cl_int clWaitForEvents_testCopyHostToBuffer(
    cl_uint num_events,
    const cl_event *event_list,
    int num_calls)
{
    (void) num_calls;

    TEST_ASSERT_NOT_NULL(event_list);
    TEST_ASSERT_EQUAL(1, num_events);
    return CL_SUCCESS;
}

static cl_int clReleaseEvent_testCopyHostToBuffer(
    cl_event event,
    int num_calls)
{
    (void) num_calls;

    TEST_ASSERT_NOT_NULL(event);
    return CL_SUCCESS;
}

void testCopyHostToBuffer(void)
{
    cl_context context_expect = make_context(0);
    int context_refcount = 1;
    prepare_contextRefcounts(1, &context_expect, &context_refcount);
    cl::Context context = contextPool[0];

    cl_mem mem_expect = make_mem(0);
    int mem_refcount = 1;
    prepare_memRefcounts(1, &mem_expect, &mem_refcount);
    cl::Buffer buffer(make_mem(0));

    cl_command_queue queue_expect = make_command_queue(0);
    cl::CommandQueue queue(queue_expect);
    clReleaseCommandQueue_ExpectAndReturn(queue_expect, CL_SUCCESS);

    // Returns the pointer to host memory
    clEnqueueMapBuffer_StubWithCallback(clEnqueueMapBuffer_testCopyHostToBuffer);
    clEnqueueUnmapMemObject_StubWithCallback(clEnqueueUnmapMemObject_testCopyHostToBuffer);

    clWaitForEvents_StubWithCallback(clWaitForEvents_testCopyHostToBuffer);
    clReleaseEvent_StubWithCallback(clReleaseEvent_testCopyHostToBuffer);

    std::vector<int> host(1024);
    for (int i = 0; i < 1024; i++)
        host[i] = i;

    cl::copy(queue, host.begin(), host.end(), buffer);

    // Check that the memory was copied to some_host_memory
    TEST_ASSERT_EQUAL_MEMORY(&host[0], some_host_memory, sizeof(int) * 1024);

    free(some_host_memory);

}

/****************************************************************************
* Tests for building Programs
****************************************************************************/

static cl_int clGetDeviceInfo_testGetBuildInfo(
    cl_device_id device,
    cl_device_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    (void) device;
    (void) num_calls;

    TEST_ASSERT_EQUAL(param_name, CL_DEVICE_PLATFORM);
    TEST_ASSERT_EQUAL(param_value_size, sizeof(cl_platform_id));
    TEST_ASSERT_NOT_EQUAL(param_value, nullptr);
    TEST_ASSERT_EQUAL(param_value_size_ret, nullptr);
    cl_platform_id temp = make_platform_id(0);
    memcpy(param_value, &temp, sizeof(cl_platform_id));
    return CL_SUCCESS;
}


static  cl_int clGetProgramBuildInfo_testGetBuildInfo(
    cl_program program,
    cl_device_id device,
    cl_program_build_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    (void) program;
    (void) device;
    (void) num_calls;

    TEST_ASSERT_EQUAL(param_name, CL_PROGRAM_BUILD_LOG);

    const char returnString[] = 
        "This is the string returned by the build info function.";
    if (param_value) {
        ::size_t returnSize = param_value_size;
        if (sizeof(returnString) < returnSize) {
            returnSize = sizeof(returnString);
        }
        memcpy(param_value, returnString, returnSize);
    }
    else {
        if (param_value_size_ret) {
            *param_value_size_ret = sizeof(returnString);
        }
    }

    return CL_SUCCESS;
}

void testGetBuildInfo(void)
{
    cl_device_id fakeDevice = make_device_id(0);
    clGetDeviceInfo_ExpectAndReturn(fakeDevice, CL_DEVICE_PLATFORM, sizeof(cl_platform_id), nullptr, nullptr, CL_SUCCESS);
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_testGetBuildInfo);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_2);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_2);
    clGetProgramBuildInfo_StubWithCallback(clGetProgramBuildInfo_testGetBuildInfo);
    clGetProgramBuildInfo_StubWithCallback(clGetProgramBuildInfo_testGetBuildInfo);

    cl::Program prog(make_program(0));
    cl::Device dev(fakeDevice);
    
    cl_int err;
    std::string log = prog.getBuildInfo<CL_PROGRAM_BUILD_LOG>(dev, &err);

    prog() = nullptr;
    dev() = nullptr;
}

static cl_int clBuildProgram_testBuildProgram(
    cl_program           program,
    cl_uint              num_devices,
    const cl_device_id * device_list,
    const char *         options,
    void (CL_CALLBACK *  pfn_notify)(cl_program program, void * user_data),
    void *               user_data,
    int num_calls)
{
    (void) num_calls;

    TEST_ASSERT_EQUAL(program, make_program(0));
    TEST_ASSERT_NOT_EQUAL(num_devices, 0);
    TEST_ASSERT_NOT_EQUAL(device_list, nullptr);
    TEST_ASSERT_EQUAL(options, nullptr);
    TEST_ASSERT_EQUAL(pfn_notify, nullptr);
    TEST_ASSERT_EQUAL(user_data, nullptr);

    for (cl_uint i = 0; i < num_devices; i++) {
        TEST_ASSERT_EQUAL(device_list[i], make_device_id(i));
    }

    return CL_SUCCESS;
}

void testBuildProgramSingleDevice(void)
{
    cl_program program = make_program(0);
    cl_device_id device_id = make_device_id(0);

    // Creating a device queries the platform version:
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_2);

    clBuildProgram_StubWithCallback(clBuildProgram_testBuildProgram);

    // Building the program queries the program build log:
    clRetainDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);
    clGetProgramBuildInfo_StubWithCallback(clGetProgramBuildInfo_testGetBuildInfo);
    clGetProgramBuildInfo_StubWithCallback(clGetProgramBuildInfo_testGetBuildInfo);
    clReleaseDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);
    clReleaseDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);

    clReleaseProgram_ExpectAndReturn(program, CL_SUCCESS);

    cl::Program prog(program);
    cl::Device dev(device_id);

    cl_int errcode = prog.build(dev);

    TEST_ASSERT_EQUAL(errcode, CL_SUCCESS);
}

/**
* Stub implementation of clGetCommandQueueInfo that returns first one image then none
*/
static cl_int clGetSupportedImageFormats_testGetSupportedImageFormats(
    cl_context context,
    cl_mem_flags flags,
    cl_mem_object_type image_type,
    cl_uint num_entries,
    cl_image_format *image_formats,
    cl_uint *num_image_formats,
    int num_calls)
{
    (void) context;
    (void) flags;
    (void) image_type;

    // Catch failure case that causes error in bugzilla 13355:
    // returns CL_INVALID_VALUE if flags or image_type are not valid, 
    // or if num_entries is 0 and image_formats is not nullptr.
    if (num_entries == 0 && image_formats != nullptr) {
        return CL_INVALID_VALUE;
    }
    if (num_entries == 0)  {
        // If num_entries was 0 this is the query for number
        if (num_image_formats) {
            if (num_calls == 0) {
                *num_image_formats = 1;
            }
            else {
                *num_image_formats = 0;
            }
        }
    }
    else {
        // Should return something
        TEST_ASSERT_NOT_NULL(image_formats);
        
        // For first call we should return one format here
        if (num_calls == 1) {
            TEST_ASSERT_EQUAL(num_entries, 1);
            image_formats[0] = cl::ImageFormat(CL_RGB, CL_FLOAT);
        }
    }

    return CL_SUCCESS;
}

void testGetSupportedImageFormats(void)
{
    cl_context ctx_cl = make_context(0);

    clGetSupportedImageFormats_StubWithCallback(clGetSupportedImageFormats_testGetSupportedImageFormats);
    clGetSupportedImageFormats_StubWithCallback(clGetSupportedImageFormats_testGetSupportedImageFormats);
    clReleaseContext_ExpectAndReturn(make_context(0), CL_SUCCESS);

    cl::Context ctx(ctx_cl);
    std::vector<cl::ImageFormat> formats;
    cl_int ret = CL_SUCCESS;

    ret = ctx.getSupportedImageFormats(
        CL_MEM_READ_WRITE,
        CL_MEM_OBJECT_IMAGE2D,
        &formats);
    TEST_ASSERT_EQUAL(ret, CL_SUCCESS);
    TEST_ASSERT_EQUAL(formats.size(), 1);
    ret = ctx.getSupportedImageFormats(
        CL_MEM_READ_WRITE,
        CL_MEM_OBJECT_IMAGE2D,
        &formats);
    TEST_ASSERT_EQUAL(formats.size(), 0);
    TEST_ASSERT_EQUAL(ret, CL_SUCCESS);
}

void testCreateSubDevice(void)
{
    // TODO

}

void testGetContextInfoDevices(void)
{
    // TODO
}

#if CL_HPP_TARGET_OPENCL_VERSION >= 200
static cl_mem clCreateImage_testCreateImage2DFromBuffer_2_0(
    cl_context context,
    cl_mem_flags flags,
    const cl_image_format *image_format,
    const cl_image_desc *image_desc,
    void *host_ptr,
    cl_int *errcode_ret,
    int num_calls)
{
    (void) context;
    (void) flags;
    (void) num_calls;

    TEST_ASSERT_NOT_NULL(image_format);
    TEST_ASSERT_NOT_NULL(image_desc);
    TEST_ASSERT_NULL(host_ptr);
    TEST_ASSERT_EQUAL_HEX(CL_MEM_OBJECT_IMAGE2D, image_desc->image_type);

    // Return the passed buffer as the cl_mem and success for the error code
    if (errcode_ret) {
        *errcode_ret = CL_SUCCESS;
    }
    return image_desc->buffer;
}
#endif

void testCreateImage2DFromBuffer_2_0(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    clGetContextInfo_StubWithCallback(clGetContextInfo_device);
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_2_0);
    clCreateImage_StubWithCallback(clCreateImage_testCreateImage2DFromBuffer_2_0);
    clReleaseMemObject_ExpectAndReturn(make_mem(0), CL_SUCCESS);
    clReleaseContext_ExpectAndReturn(make_context(0), CL_SUCCESS);

    cl_int err;
    cl::Context context(make_context(0));

    // Create buffer
    // Create image from buffer
    cl::Buffer buffer(make_mem(0));
    cl::Image2D imageFromBuffer(
        context,
        cl::ImageFormat(CL_R, CL_FLOAT), buffer, 64, 32, 256, &err);

    TEST_ASSERT_EQUAL_PTR(buffer(), imageFromBuffer());
    TEST_ASSERT_EQUAL(CL_SUCCESS, err);

    buffer() = nullptr;
#endif
}

#if CL_HPP_TARGET_OPENCL_VERSION >= 200
static cl_mem clCreateImage_testCreateImage2D_2_0(
    cl_context context,
    cl_mem_flags flags,
    const cl_image_format *image_format,
    const cl_image_desc *image_desc,
    void *host_ptr,
    cl_int *errcode_ret,
    int num_calls)
{
    TEST_ASSERT_EQUAL(0, num_calls);
    TEST_ASSERT_EQUAL_PTR(make_context(0), context);
    TEST_ASSERT_EQUAL_HEX(CL_MEM_READ_WRITE, flags);

    TEST_ASSERT_NOT_NULL(image_format);
    TEST_ASSERT_EQUAL_HEX(CL_RGBA, image_format->image_channel_order);
    TEST_ASSERT_EQUAL_HEX(CL_FLOAT, image_format->image_channel_data_type);

    TEST_ASSERT_NOT_NULL(image_desc);
    TEST_ASSERT_EQUAL_HEX(CL_MEM_OBJECT_IMAGE2D, image_desc->image_type);
    TEST_ASSERT_EQUAL(64, image_desc->image_width);
    TEST_ASSERT_EQUAL(32, image_desc->image_height);
    TEST_ASSERT_EQUAL(256, image_desc->image_row_pitch);
    TEST_ASSERT_EQUAL(0, image_desc->num_mip_levels);
    TEST_ASSERT_EQUAL(0, image_desc->num_samples);
    TEST_ASSERT_NULL(image_desc->buffer);

    TEST_ASSERT_NULL(host_ptr);

    if (errcode_ret != nullptr)
        *errcode_ret = CL_SUCCESS;
    return make_mem(0);
}
#endif

#if CL_HPP_TARGET_OPENCL_VERSION >= 200
static cl_mem clCreateImage_testCreateImage2DFromImage_2_0(
    cl_context context,
    cl_mem_flags flags,
    const cl_image_format *image_format,
    const cl_image_desc *image_desc,
    void *host_ptr,
    cl_int *errcode_ret,
    int num_calls)
{
    (void) context;
    (void) flags;
    (void) num_calls;

    TEST_ASSERT_NOT_NULL(image_format);
    TEST_ASSERT_NOT_NULL(image_desc);
    TEST_ASSERT_NULL(host_ptr);
    TEST_ASSERT_EQUAL_HEX(CL_MEM_OBJECT_IMAGE2D, image_desc->image_type);

    // Return the passed buffer as the cl_mem and success for the error code
    if (errcode_ret) {
        *errcode_ret = CL_SUCCESS;
    }
    return image_desc->buffer;
}
#endif

#if CL_HPP_TARGET_OPENCL_VERSION >= 200
static cl_int clGetImageInfo_testCreateImage2DFromImage_2_0(
    cl_mem image,
    cl_image_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    (void) image;
    (void) param_name;
    (void) param_value_size;
    (void) param_value;
    (void) param_value_size_ret;

    TEST_ASSERT_INT_WITHIN(6, 0, num_calls);
    return CL_SUCCESS;
}
#endif

void testCreateImage2DFromImage_2_0(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    clGetContextInfo_StubWithCallback(clGetContextInfo_device);
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_2_0);
    clCreateImage_StubWithCallback(clCreateImage_testCreateImage2D_2_0);


    cl_int err;
    cl::Context context(make_context(0));

    // As in 1.2 2D image test, needed as source for image-from-image
    cl::Image2D image(
        context, CL_MEM_READ_WRITE,
        cl::ImageFormat(CL_RGBA, CL_FLOAT), 64, 32, 256, nullptr, &err);

    TEST_ASSERT_EQUAL(CL_SUCCESS, err);
    TEST_ASSERT_EQUAL_PTR(make_mem(0), image());

    // Continue state for next phase
    clGetImageInfo_StubWithCallback(clGetImageInfo_testCreateImage2DFromImage_2_0);
    clCreateImage_StubWithCallback(clCreateImage_testCreateImage2DFromImage_2_0);
    clReleaseMemObject_ExpectAndReturn(make_mem(0), CL_SUCCESS);
    clReleaseMemObject_ExpectAndReturn(make_mem(0), CL_SUCCESS);
    clReleaseContext_ExpectAndReturn(make_context(0), CL_SUCCESS);

    // Create 2D image from 2D Image with a new channel order
    cl::Image2D imageFromImage(
        context,
        CL_sRGB,
        image,
        &err
        );

    TEST_ASSERT_EQUAL(CL_SUCCESS, err);
    TEST_ASSERT_EQUAL_PTR(image(), imageFromImage());

    //imageFromImage() = nullptr;
    //image() = nullptr;
    //context() = nullptr;
#endif
}

// Note that default tests maintain state when run from the same
// unit process.
// One default setting test will maintain the defaults until the end.
void testSetDefaultPlatform(void)
{
    cl::Platform p(make_platform_id(1));
    cl::Platform p2 = cl::Platform::setDefault(p);
    cl::Platform p3 = cl::Platform::getDefault();
    TEST_ASSERT_EQUAL(p(), p2());
    TEST_ASSERT_EQUAL(p(), p3());
}

// Note that default tests maintain state when run from the same
// unit process.
// One default setting test will maintain the defaults until the end.
void testSetDefaultPlatformTwice(void)
{
    cl::Platform p(make_platform_id(2));
    cl::Platform p2 = cl::Platform::getDefault();
    cl::Platform p3 = cl::Platform::setDefault(p);
    // Set default should have failed
    TEST_ASSERT_EQUAL(p2(), p3());
    TEST_ASSERT_NOT_EQUAL(p(), p3());
}

// Note that default tests maintain state when run from the same
// unit process.
// One default setting test will maintain the defaults until the end.
void testSetDefaultContext(void)
{   

    clRetainContext_ExpectAndReturn(make_context(1), CL_SUCCESS);
    clRetainContext_ExpectAndReturn(make_context(1), CL_SUCCESS);
    clRetainContext_ExpectAndReturn(make_context(1), CL_SUCCESS);
    clReleaseContext_ExpectAndReturn(make_context(1), CL_SUCCESS);
    clReleaseContext_ExpectAndReturn(make_context(1), CL_SUCCESS);
    clReleaseContext_ExpectAndReturn(make_context(1), CL_SUCCESS);

    cl::Context c(make_context(1));
    cl::Context c2 = cl::Context::setDefault(c);
    cl::Context c3 = cl::Context::getDefault();
    TEST_ASSERT_EQUAL(c(), c2());
    TEST_ASSERT_EQUAL(c(), c3());
}

// Note that default tests maintain state when run from the same
// unit process.
// One default setting test will maintain the defaults until the end.
void testSetDefaultCommandQueue(void)
{
    clRetainCommandQueue_ExpectAndReturn(make_command_queue(1), CL_SUCCESS);
    clRetainCommandQueue_ExpectAndReturn(make_command_queue(1), CL_SUCCESS);
    clRetainCommandQueue_ExpectAndReturn(make_command_queue(1), CL_SUCCESS);
    clReleaseCommandQueue_ExpectAndReturn(make_command_queue(1), CL_SUCCESS);
    clReleaseCommandQueue_ExpectAndReturn(make_command_queue(1), CL_SUCCESS);
    clReleaseCommandQueue_ExpectAndReturn(make_command_queue(1), CL_SUCCESS);

    cl::CommandQueue c(make_command_queue(1));
    cl::CommandQueue c2 = cl::CommandQueue::setDefault(c);
    cl::CommandQueue c3 = cl::CommandQueue::getDefault();
    TEST_ASSERT_EQUAL(c(), c2());
    TEST_ASSERT_EQUAL(c(), c3());
}

// Note that default tests maintain state when run from the same
// unit process.
// One default setting test will maintain the defaults until the end.
void testSetDefaultDevice(void)
{
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_2_0);

    clRetainDevice_ExpectAndReturn(make_device_id(1), CL_SUCCESS);
    clRetainDevice_ExpectAndReturn(make_device_id(1), CL_SUCCESS);
    clRetainDevice_ExpectAndReturn(make_device_id(1), CL_SUCCESS);
    clReleaseDevice_ExpectAndReturn(make_device_id(1), CL_SUCCESS);
    clReleaseDevice_ExpectAndReturn(make_device_id(1), CL_SUCCESS);
    clReleaseDevice_ExpectAndReturn(make_device_id(1), CL_SUCCESS);

    cl::Device d(make_device_id(1));
    cl::Device  d2 = cl::Device::setDefault(d);
    cl::Device  d3 = cl::Device::getDefault();
    TEST_ASSERT_EQUAL(d(), d2());
    TEST_ASSERT_EQUAL(d(), d3());
}

#if CL_HPP_TARGET_OPENCL_VERSION >= 200
static cl_command_queue clCreateCommandQueueWithProperties_testCommandQueueDevice(
    cl_context context,
    cl_device_id device,
    const cl_queue_properties *properties,
    cl_int *errcode_ret,
    int num_calls)
{
    (void)num_calls;
    TEST_ASSERT_EQUAL_PTR(make_context(1), context);
    TEST_ASSERT_EQUAL_PTR(make_device_id(1), device);
    TEST_ASSERT_EQUAL(properties[0], CL_QUEUE_PROPERTIES);
    static cl_command_queue default_ = 0;

    if (errcode_ret != nullptr)
        *errcode_ret = CL_SUCCESS;

    if ((properties[1] & CL_QUEUE_ON_DEVICE_DEFAULT) == 0) {
        TEST_ASSERT_EQUAL(properties[1], (CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_ON_DEVICE));
        if (properties[2] == CL_QUEUE_SIZE) {
            TEST_ASSERT_EQUAL(properties[3], 256);
            TEST_ASSERT_EQUAL(properties[4], 0);
            return make_command_queue(2);
        }
        else {
            TEST_ASSERT_EQUAL(properties[2], 0);
            return make_command_queue(3);
        }
    }
    else {
        TEST_ASSERT_EQUAL(properties[1], (CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_ON_DEVICE | CL_QUEUE_ON_DEVICE_DEFAULT));
        if (default_ == 0) {
            default_ = make_command_queue(4);
        }
        return default_;
    }
}
#endif

void testCreateDeviceCommandQueue(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    clRetainContext_ExpectAndReturn(make_context(1), CL_SUCCESS);
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_2_0);
    clCreateCommandQueueWithProperties_StubWithCallback(clCreateCommandQueueWithProperties_testCommandQueueDevice);       
    clReleaseCommandQueue_ExpectAndReturn(make_command_queue(4), CL_SUCCESS);
    clReleaseCommandQueue_ExpectAndReturn(make_command_queue(4), CL_SUCCESS);
    clReleaseCommandQueue_ExpectAndReturn(make_command_queue(2), CL_SUCCESS);
    clReleaseCommandQueue_ExpectAndReturn(make_command_queue(3), CL_SUCCESS);
    clReleaseDevice_ExpectAndReturn(make_device_id(1), CL_SUCCESS);
    clReleaseContext_ExpectAndReturn(make_context(1), CL_SUCCESS);
    clReleaseContext_ExpectAndReturn(make_context(1), CL_SUCCESS);

    cl::Context c(make_context(1));
    cl::Context c2 = cl::Context::setDefault(c);
    cl::Device d(make_device_id(1));

    cl::DeviceCommandQueue dq(c, d);
    cl::DeviceCommandQueue dq2(c, d, 256);    

    cl::DeviceCommandQueue dqd = cl::DeviceCommandQueue::makeDefault(c, d);
    cl::DeviceCommandQueue dqd2 = cl::DeviceCommandQueue::makeDefault(c, d);

    TEST_ASSERT_EQUAL(dqd(), dqd2());
#endif
}

#if CL_HPP_TARGET_OPENCL_VERSION >= 200
static cl_mem clCreatePipe_testCreatePipe(
    cl_context context,
    cl_mem_flags flags,
    cl_uint packet_size,
    cl_uint num_packets,
    const cl_pipe_properties *props,
    cl_int *errcode_ret,
    int num_calls)
{
    (void) context;
    (void) packet_size;
    (void) num_packets;
    (void) num_calls;

    if (flags == 0) {
        flags = CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS;
    }
    TEST_ASSERT_EQUAL(flags, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS);
    TEST_ASSERT_NULL(props);

    if (errcode_ret)
        *errcode_ret = CL_SUCCESS;
    return make_mem(0);
}
#endif

#if CL_HPP_TARGET_OPENCL_VERSION >= 200
static cl_int clGetPipeInfo_testCreatePipe(
    cl_mem pipe,
    cl_pipe_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    (void) pipe;
    (void) num_calls;

    TEST_ASSERT_NOT_NULL(param_value);
    if (param_name == CL_PIPE_PACKET_SIZE) {
        *static_cast<cl_uint*>(param_value) = 16;
        if (param_value_size_ret) {
            *param_value_size_ret = param_value_size;
        }
        return CL_SUCCESS;
    }
    else if (param_name == CL_PIPE_MAX_PACKETS) {
        *static_cast<cl_uint*>(param_value) = 32;
        if (param_value_size_ret) {
            *param_value_size_ret = param_value_size;
        }
        return CL_SUCCESS;
    }
    else {
        TEST_FAIL();
        return CL_INVALID_VALUE;
    }
}
#endif

void testCreatePipe(void)
{    
#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    clCreatePipe_StubWithCallback(clCreatePipe_testCreatePipe);
    clGetPipeInfo_StubWithCallback(clGetPipeInfo_testCreatePipe);
    clRetainContext_ExpectAndReturn(make_context(1), CL_SUCCESS);
    clReleaseContext_ExpectAndReturn(make_context(1), CL_SUCCESS);
    clReleaseMemObject_ExpectAndReturn(make_mem(0), CL_SUCCESS);
    clReleaseMemObject_ExpectAndReturn(make_mem(0), CL_SUCCESS);
    clReleaseContext_ExpectAndReturn(make_context(1), CL_SUCCESS);

    cl::Context c(make_context(1));
    cl::Pipe p(c, 16, 32);
    cl::Pipe p2(16, 32);

    cl_uint size = p2.getInfo<CL_PIPE_PACKET_SIZE>();
    cl_uint packets;
    p2.getInfo(CL_PIPE_MAX_PACKETS, &packets);

    TEST_ASSERT_EQUAL(size, 16);
    TEST_ASSERT_EQUAL(packets, 32);
#endif
}

#if CL_HPP_TARGET_OPENCL_VERSION >= 210
static cl_int clGetKernelSubGroupInfo_testSubGroups(cl_kernel kernel,
    cl_device_id device,
    cl_kernel_sub_group_info param_name,
    size_t input_value_size,
    const void *input_value,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    (void) kernel;
    (void) device;
    (void) input_value_size;
    (void) param_value_size;
    (void) num_calls;

    TEST_ASSERT_NOT_NULL(input_value);
    TEST_ASSERT_NOT_NULL(param_value);

    if (param_name == CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE_KHR) {
        *static_cast<size_t*>(param_value) = 32;
        if (param_value_size_ret) {
            *param_value_size_ret = sizeof(size_t);
        }
        return CL_SUCCESS;
    }
    else if (param_name == CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE_KHR) {
        *static_cast<size_t*>(param_value) = 2;
        if (param_value_size_ret) {
            *param_value_size_ret = sizeof(size_t);
        }
        return CL_SUCCESS;
    }
    else {
        TEST_ABORT();
        return CL_INVALID_OPERATION;
    }
}
#endif

void testSubGroups(void)
{
// TODO support testing cl_khr_subgroups on 2.0
#if CL_HPP_TARGET_OPENCL_VERSION >= 210
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_2_0);
    clGetKernelSubGroupInfo_StubWithCallback(clGetKernelSubGroupInfo_testSubGroups);
    clReleaseDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);
    clReleaseKernel_ExpectAndReturn(make_kernel(0), CL_SUCCESS);

    cl::Kernel k(make_kernel(0));
    cl::Device d(make_device_id(0));
    cl_int err;
    cl::NDRange ndrange(8, 8);
    size_t res1 = k.getSubGroupInfo<CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE_KHR>(
        d, ndrange, &err);
    size_t res2 = 0;
    err = k.getSubGroupInfo(
        d, CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE_KHR, ndrange, &res2);

    TEST_ASSERT_EQUAL(res1, 32);
    TEST_ASSERT_EQUAL(res2, 2);
#endif
}

/**
* Stub implementation of clGetDeviceInfo that returns an absense of builtin kernels
*/
static cl_int clGetDeviceInfo_builtin(
    cl_device_id id,
    cl_device_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    (void) id;
    (void) param_value_size;

    // Test to verify case where empty string is returned - so size is 0
    (void)num_calls;
    TEST_ASSERT_EQUAL_HEX(CL_DEVICE_BUILT_IN_KERNELS, param_name);
    if (param_value == nullptr) {
        if (param_value_size_ret != nullptr) {
            *param_value_size_ret = 0;
        }
    }
    return CL_SUCCESS;
}

void testBuiltInKernels(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 120
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_2_0);
    clReleaseDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);

    cl::Device d0(make_device_id(0));

    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_builtin);
    cl::string s = d0.getInfo<CL_DEVICE_BUILT_IN_KERNELS>();
#endif
}

#if CL_HPP_TARGET_OPENCL_VERSION >= 210
/**
 * Stub implementation of clCloneKernel that returns a new kernel object
 */
static cl_kernel clCloneKernel_simplecopy(
    cl_kernel k,
    cl_int *errcode_ret,
    int num_calls)
{
    (void) k;
    (void) num_calls;

    // Test to verify case where empty string is returned - so size is 0
    if (errcode_ret != nullptr)
        *errcode_ret = CL_SUCCESS;
    return make_kernel(POOL_MAX);
}
#endif

void testCloneKernel(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 210
    clCloneKernel_StubWithCallback(clCloneKernel_simplecopy);
    clReleaseKernel_ExpectAndReturn(make_kernel(POOL_MAX), CL_SUCCESS);
    cl::Kernel clone = kernelPool[0].clone();
    TEST_ASSERT_EQUAL(clone(), make_kernel(POOL_MAX));
#endif
}

void testEnqueueMapSVM(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    std::vector<int> vec(7);
    clEnqueueSVMMap_ExpectAndReturn(commandQueuePool[0].get(), CL_TRUE, CL_MAP_READ|CL_MAP_WRITE, static_cast<void*>(vec.data()), vec.size()*sizeof(int), 0, nullptr, nullptr, CL_SUCCESS);
    TEST_ASSERT_EQUAL(commandQueuePool[0].enqueueMapSVM(vec, CL_TRUE, CL_MAP_READ|CL_MAP_WRITE), CL_SUCCESS);
#endif
}

void testMapSVM(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    std::vector<int> vec(1);
    clRetainCommandQueue_ExpectAndReturn(make_command_queue(1), CL_SUCCESS);
    clEnqueueSVMMap_ExpectAndReturn(make_command_queue(1), CL_TRUE, CL_MAP_READ|CL_MAP_WRITE, static_cast<void*>(vec.data()), vec.size()*sizeof(int), 0, nullptr, nullptr, CL_SUCCESS);
    clReleaseCommandQueue_ExpectAndReturn(make_command_queue(1), CL_SUCCESS);
    TEST_ASSERT_EQUAL(cl::mapSVM(vec), CL_SUCCESS);
#endif
}

// Run after other tests to clear the default state in the header
// using special unit test bypasses.
// We cannot remove the once_flag, so this is a hard fix
// but it means we won't hit cmock release callbacks at the end.
// This is a lot like tearDown but for the header default
// so we do not want to run it for every test.
// The alternative would be to manually modify the test runner
// but we avoid that for now.
void testCleanupHeaderState(void)
{
    clReleaseCommandQueue_ExpectAndReturn(make_command_queue(1), CL_SUCCESS);
    clReleaseContext_ExpectAndReturn(make_context(1), CL_SUCCESS);
    clReleaseDevice_ExpectAndReturn(make_device_id(1), CL_SUCCESS);

    cl::CommandQueue::unitTestClearDefault();
    cl::Context::unitTestClearDefault();
    cl::Device::unitTestClearDefault();
    cl::Platform::unitTestClearDefault();
}

// OpenCL 2.2 APIs:

#if CL_HPP_TARGET_OPENCL_VERSION >= 220
static void CL_CALLBACK test_program_release_callback(
    cl_program,
    void*)
{
}
#endif

#if CL_HPP_TARGET_OPENCL_VERSION >= 220
static cl_int clSetProgramReleaseCallback_set(
    cl_program program,
    void (CL_CALLBACK * pfn_notify)(cl_program program, void * user_data),
    void *user_data,
    int num_calls)
{
    (void) user_data;
    (void) num_calls;

    TEST_ASSERT_EQUAL_PTR(make_program(0), program);
    TEST_ASSERT_EQUAL_PTR(pfn_notify, test_program_release_callback);

    return CL_SUCCESS;
}
#endif

void testSetProgramReleaseCallback(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 220
    cl_program program = make_program(0);
    int user_data = 0;

    clSetProgramReleaseCallback_StubWithCallback(clSetProgramReleaseCallback_set);
    clReleaseProgram_ExpectAndReturn(program, CL_SUCCESS);

    cl::Program prog(program);

    prog.setReleaseCallback(test_program_release_callback, &user_data);
#endif
}

void testSetProgramSpecializationConstantScalar(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 220
    cl_program program = make_program(0);
    int sc = 0;

    clSetProgramSpecializationConstant_ExpectAndReturn(program, 0, sizeof(sc), &sc, CL_SUCCESS);
    clReleaseProgram_ExpectAndReturn(program, CL_SUCCESS);

    cl::Program prog(program);

    prog.setSpecializationConstant(0, sc);
#endif
}

#if CL_HPP_TARGET_OPENCL_VERSION >= 220
/// Stub for testing boolean specialization constants
static cl_int clSetProgramSpecializationConstant_testBool(
    cl_program program,
    cl_uint spec_id,
    size_t spec_size,
    const void* spec_value,
    int num_calls)
{
    (void) num_calls;

    TEST_ASSERT_EQUAL_PTR(make_program(0), program);
    TEST_ASSERT(spec_id == 0 || spec_id == 1);
    TEST_ASSERT_EQUAL(spec_size, 1);
    if (spec_id == 0)
    {
        const cl_uchar *uc_value = (const cl_uchar*)spec_value;
        TEST_ASSERT_EQUAL_HEX(uc_value[0], 0);
    }
    if (spec_id == 1)
    {
        const cl_uchar *uc_value = (const cl_uchar*)spec_value;
        TEST_ASSERT_EQUAL_HEX(uc_value[0], CL_UCHAR_MAX);
    }
    return CL_SUCCESS;
}
#endif

void testSetProgramSpecializationConstantBool(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 220
    // Spec constant "false" should turn into a call with size one and no bits set.
    // Spec constant "true" should turn into a call with size one and all bits set.
    cl_program program = make_program(0);
    bool scFalse = false;
    bool scTrue = true;

    clSetProgramSpecializationConstant_StubWithCallback(clSetProgramSpecializationConstant_testBool);

    clReleaseProgram_ExpectAndReturn(program, CL_SUCCESS);

    cl::Program prog(program);

    prog.setSpecializationConstant(0, scFalse);
    prog.setSpecializationConstant(1, scTrue);
#endif
}

void testSetProgramSpecializationConstantPointer(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 220
    cl_program program = make_program(0);
    int scArray[5] = {};

    clSetProgramSpecializationConstant_ExpectAndReturn(program, 0, sizeof(scArray), &scArray, CL_SUCCESS);
    clReleaseProgram_ExpectAndReturn(program, CL_SUCCESS);

    cl::Program prog(program);

    prog.setSpecializationConstant(0, sizeof(scArray), scArray);
#endif
}

// OpenCL 3.0 and cl_khr_extended_versioning Queries

// Assumes the core enums, structures, and macros exactly match
// the extension enums, structures, and macros:

static_assert(CL_PLATFORM_NUMERIC_VERSION == CL_PLATFORM_NUMERIC_VERSION_KHR,
    "CL_PLATFORM_NUMERIC_VERSION mismatch");
static_assert(CL_PLATFORM_EXTENSIONS_WITH_VERSION == CL_PLATFORM_EXTENSIONS_WITH_VERSION_KHR,
    "CL_PLATFORM_EXTENSIONS_WITH_VERSION mismatch");

static_assert(CL_DEVICE_NUMERIC_VERSION == CL_DEVICE_NUMERIC_VERSION_KHR,
    "CL_DEVICE_NUMERIC_VERSION mismatch");
static_assert(CL_DEVICE_EXTENSIONS_WITH_VERSION == CL_DEVICE_EXTENSIONS_WITH_VERSION_KHR,
    "CL_DEVICE_EXTENSIONS_WITH_VERSION mismatch");
static_assert(CL_DEVICE_ILS_WITH_VERSION == CL_DEVICE_ILS_WITH_VERSION_KHR,
    "CL_DEVICE_ILS_WITH_VERSION mismatch");
static_assert(CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION == CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION_KHR,
    "CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION mismatch");

static_assert(sizeof(cl_name_version) == sizeof(cl_name_version_khr),
    "cl_name_version mismatch");

static_assert(CL_MAKE_VERSION(1, 2, 3) == CL_MAKE_VERSION_KHR(1, 2, 3),
    "CL_MAKE_VERSION mismatch");

static cl_int clGetPlatformInfo_extended_versioning(
    cl_platform_id id,
    cl_platform_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    (void) id;
    (void) num_calls;

    switch (param_name) {
    case CL_PLATFORM_NUMERIC_VERSION:
    {
        if (param_value_size == sizeof(cl_version) && param_value) {
            *static_cast<cl_version*>(param_value) = CL_MAKE_VERSION(1, 2, 3);
        }
        if (param_value_size_ret) {
            *param_value_size_ret = sizeof(cl_version);
        }
        return CL_SUCCESS;
    }
    case CL_PLATFORM_EXTENSIONS_WITH_VERSION:
    {
        static cl_name_version extension = {
            CL_MAKE_VERSION(10, 11, 12),
            "cl_dummy_extension",
        };
        if (param_value_size == sizeof(cl_name_version) && param_value) {
            *static_cast<cl_name_version*>(param_value) = extension;
        }
        if (param_value_size_ret) {
            *param_value_size_ret = sizeof(extension);
        }
        return CL_SUCCESS;
    }
    default: break;
    }
    TEST_FAIL();
    return CL_INVALID_OPERATION;
}

void testPlatformExtendedVersioning_3_0(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 300
    cl::Platform p(make_platform_id(1));

    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_extended_versioning);

    cl_version platformVersion = p.getInfo<CL_PLATFORM_NUMERIC_VERSION>();
    TEST_ASSERT_EQUAL_HEX(platformVersion, CL_MAKE_VERSION(1, 2, 3));

    std::vector<cl_name_version> extensions = p.getInfo<CL_PLATFORM_EXTENSIONS_WITH_VERSION>();
    TEST_ASSERT_EQUAL(extensions.size(), 1);
    TEST_ASSERT_EQUAL_HEX(extensions[0].version, CL_MAKE_VERSION(10, 11, 12));
    TEST_ASSERT_EQUAL_STRING(extensions[0].name, "cl_dummy_extension");
#endif // CL_HPP_TARGET_OPENCL_VERSION >= 300
}

void testPlatformExtendedVersioning_KHR(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION < 300
    cl::Platform p(make_platform_id(1));

    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_extended_versioning);

    cl_version_khr platformVersion = p.getInfo<CL_PLATFORM_NUMERIC_VERSION_KHR>();
    TEST_ASSERT_EQUAL_HEX(platformVersion, CL_MAKE_VERSION_KHR(1, 2, 3));

    std::vector<cl_name_version_khr> extensions = p.getInfo<CL_PLATFORM_EXTENSIONS_WITH_VERSION_KHR>();
    TEST_ASSERT_EQUAL(extensions.size(), 1);
    TEST_ASSERT_EQUAL_HEX(extensions[0].version, CL_MAKE_VERSION_KHR(10, 11, 12));
    TEST_ASSERT_EQUAL_STRING(extensions[0].name, "cl_dummy_extension");
#endif // CL_HPP_TARGET_OPENCL_VERSION < 300
}


// Note: This assumes the core enums, structures, and macros exactly match
// the extension enums, structures, and macros.

static cl_int clGetDeviceInfo_extended_versioning(
    cl_device_id id,
    cl_device_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    (void) id;
    (void) num_calls;

    switch (param_name) {
    case CL_DEVICE_NUMERIC_VERSION:
    {
        if (param_value_size == sizeof(cl_version) && param_value) {
            *static_cast<cl_version*>(param_value) = CL_MAKE_VERSION(1, 2, 3);
        }
        if (param_value_size_ret) {
            *param_value_size_ret = sizeof(cl_version);
        }
        return CL_SUCCESS;
    }
    case CL_DEVICE_OPENCL_C_NUMERIC_VERSION_KHR:
    {
        if (param_value_size == sizeof(cl_version_khr) && param_value) {
            *static_cast<cl_version_khr*>(param_value) = CL_MAKE_VERSION_KHR(4, 5, 6);
        }
        if (param_value_size_ret) {
            *param_value_size_ret = sizeof(cl_version_khr);
        }
        return CL_SUCCESS;
    }
    case CL_DEVICE_EXTENSIONS_WITH_VERSION:
    {
        static cl_name_version extension = {
            CL_MAKE_VERSION(10, 11, 12),
            "cl_dummy_extension",
        };
        if (param_value_size == sizeof(cl_name_version) && param_value) {
            *static_cast<cl_name_version*>(param_value) = extension;
        }
        if (param_value_size_ret) {
            *param_value_size_ret = sizeof(extension);
        }
        return CL_SUCCESS;
    }
    case CL_DEVICE_ILS_WITH_VERSION:
    {
        static cl_name_version il = {
            CL_MAKE_VERSION(20, 21, 22),
            "DUMMY_IR",
        };
        if (param_value_size == sizeof(cl_name_version) && param_value) {
            *static_cast<cl_name_version*>(param_value) = il;
        }
        if (param_value_size_ret) {
            *param_value_size_ret = sizeof(il);
        }
        return CL_SUCCESS;
    }
    case CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION:
    {
        // Test no built-in kernels:
        if (param_value_size_ret) {
            *param_value_size_ret = 0;
        }
        return CL_SUCCESS;
    }
    case CL_DEVICE_OPENCL_C_ALL_VERSIONS:
    {
        static cl_name_version opencl_c = {
            CL_MAKE_VERSION(30, 31, 32),
            "OpenCL C",
        };
        if (param_value_size == sizeof(cl_name_version) && param_value) {
            *static_cast<cl_name_version*>(param_value) = opencl_c;
        }
        if (param_value_size_ret) {
            *param_value_size_ret = sizeof(opencl_c);
        }
        return CL_SUCCESS;
    }
    case CL_DEVICE_OPENCL_C_FEATURES:
    {
        static cl_name_version opencl_c_features[] = {
            {
                CL_MAKE_VERSION(40, 41, 42),
                "__opencl_c_feature",
            },
            {
                CL_MAKE_VERSION(40, 43, 44),
                "__opencl_c_fancy_feature",
            },
        };
        if (param_value_size == sizeof(opencl_c_features) && param_value) {
            cl_name_version* feature = static_cast<cl_name_version*>(param_value);
            const int numFeatures = ARRAY_SIZE(opencl_c_features);
            for (int i = 0; i < numFeatures; i++) {
                feature[i] = opencl_c_features[i];
            }
        }
        if (param_value_size_ret) {
            *param_value_size_ret = sizeof(opencl_c_features);
        }
        return CL_SUCCESS;
    }
    default: break;
    }
    TEST_FAIL();
    return CL_INVALID_OPERATION;
}

void testDeviceExtendedVersioning_3_0(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 300
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_3_0);
    clReleaseDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);

    cl::Device d0(make_device_id(0));

    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_extended_versioning);

    cl_version deviceVersion = d0.getInfo<CL_DEVICE_NUMERIC_VERSION>();
    TEST_ASSERT_EQUAL_HEX(deviceVersion, CL_MAKE_VERSION(1, 2, 3));

    std::vector<cl_name_version> extensions = d0.getInfo<CL_DEVICE_EXTENSIONS_WITH_VERSION>();
    TEST_ASSERT_EQUAL(extensions.size(), 1);
    TEST_ASSERT_EQUAL_HEX(extensions[0].version, CL_MAKE_VERSION(10, 11, 12));
    TEST_ASSERT_EQUAL_STRING(extensions[0].name, "cl_dummy_extension");

    std::vector<cl_name_version> ils = d0.getInfo<CL_DEVICE_ILS_WITH_VERSION>();
    TEST_ASSERT_EQUAL(ils.size(), 1);
    TEST_ASSERT_EQUAL_HEX(ils[0].version, CL_MAKE_VERSION(20, 21, 22));
    TEST_ASSERT_EQUAL_STRING(ils[0].name, "DUMMY_IR");

    std::vector<cl_name_version> opencl_c = d0.getInfo<CL_DEVICE_OPENCL_C_ALL_VERSIONS>();
    TEST_ASSERT_EQUAL(opencl_c.size(), 1);
    TEST_ASSERT_EQUAL_HEX(opencl_c[0].version, CL_MAKE_VERSION(30, 31, 32));
    TEST_ASSERT_EQUAL_STRING(opencl_c[0].name, "OpenCL C");

    std::vector<cl_name_version> opencl_c_features = d0.getInfo<CL_DEVICE_OPENCL_C_FEATURES>();
    TEST_ASSERT_EQUAL(opencl_c_features.size(), 2);
    TEST_ASSERT_EQUAL_HEX(opencl_c_features[0].version, CL_MAKE_VERSION(40, 41, 42));
    TEST_ASSERT_EQUAL_STRING(opencl_c_features[0].name, "__opencl_c_feature");
    TEST_ASSERT_EQUAL_HEX(opencl_c_features[1].version, CL_MAKE_VERSION(40, 43, 44));
    TEST_ASSERT_EQUAL_STRING(opencl_c_features[1].name, "__opencl_c_fancy_feature");

    std::vector<cl_name_version> builtInKernels = d0.getInfo<CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION>();
    TEST_ASSERT_EQUAL(builtInKernels.size(), 0);
#endif // CL_HPP_TARGET_OPENCL_VERSION >= 300
}

void testDeviceExtendedVersioning_KHR(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION < 300
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_2_0);
    clReleaseDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);

    cl::Device d0(make_device_id(0));

    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_extended_versioning);

    cl_version_khr deviceVersion = d0.getInfo<CL_DEVICE_NUMERIC_VERSION_KHR>();
    TEST_ASSERT_EQUAL_HEX(deviceVersion, CL_MAKE_VERSION_KHR(1, 2, 3));

    cl_version_khr cVersion = d0.getInfo<CL_DEVICE_OPENCL_C_NUMERIC_VERSION_KHR>();
    TEST_ASSERT_EQUAL_HEX(cVersion, CL_MAKE_VERSION_KHR(4, 5, 6));

    std::vector<cl_name_version_khr> extensions = d0.getInfo<CL_DEVICE_EXTENSIONS_WITH_VERSION_KHR>();
    TEST_ASSERT_EQUAL(extensions.size(), 1);
    TEST_ASSERT_EQUAL_HEX(extensions[0].version, CL_MAKE_VERSION_KHR(10, 11, 12));
    TEST_ASSERT_EQUAL_STRING(extensions[0].name, "cl_dummy_extension");

    std::vector<cl_name_version_khr> ils = d0.getInfo<CL_DEVICE_ILS_WITH_VERSION_KHR>();
    TEST_ASSERT_EQUAL(ils.size(), 1);
    TEST_ASSERT_EQUAL_HEX(ils[0].version, CL_MAKE_VERSION_KHR(20, 21, 22));
    TEST_ASSERT_EQUAL_STRING(ils[0].name, "DUMMY_IR");

    std::vector<cl_name_version_khr> builtInKernels = d0.getInfo<CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION_KHR>();
    TEST_ASSERT_EQUAL(builtInKernels.size(), 0);
#endif // CL_HPP_TARGET_OPENCL_VERSION < 300
}

static cl_int clGetDeviceInfo_uuid_pci_bus_info(
    cl_device_id id,
    cl_device_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int num_calls)
{
    (void) id;
    (void) num_calls;

    switch (param_name) {
#if defined(cl_khr_device_uuid)
    case CL_DEVICE_UUID_KHR:
    case CL_DRIVER_UUID_KHR:
    {
        if (param_value_size == CL_UUID_SIZE_KHR && param_value) {
            cl_uchar* pUUID = static_cast<cl_uchar*>(param_value);
            cl_uchar start =
                (param_name == CL_DEVICE_UUID_KHR) ? 1 :
                (param_name == CL_DRIVER_UUID_KHR) ? 2 :
                0;
            for (int i = 0; i < CL_UUID_SIZE_KHR; i++) {
                pUUID[i] = i + start;
            }
        }
        if (param_value_size_ret) {
            *param_value_size_ret = CL_UUID_SIZE_KHR;
        }
        return CL_SUCCESS;
    }
    case CL_DEVICE_LUID_VALID_KHR:
    {
        if (param_value_size == sizeof(cl_bool) && param_value) {
            *static_cast<cl_bool*>(param_value) = CL_TRUE;
        }
        if (param_value_size_ret) {
            *param_value_size_ret = sizeof(cl_bool);
        }
        return CL_SUCCESS;
    }
    case CL_DEVICE_LUID_KHR:
    {
        if (param_value_size == CL_LUID_SIZE_KHR && param_value) {
            cl_uchar* pLUID = static_cast<cl_uchar*>(param_value);
            cl_uchar start = 3;
            for (int i = 0; i < CL_LUID_SIZE_KHR; i++) {
                pLUID[i] = i + start;
            }
        }
        if (param_value_size_ret) {
            *param_value_size_ret = CL_LUID_SIZE_KHR;
        }
        return CL_SUCCESS;
    }
    case CL_DEVICE_NODE_MASK_KHR:
    {
        if (param_value_size == sizeof(cl_uint) && param_value) {
            *static_cast<cl_uint*>(param_value) = 0xA5A5;
        }
        if (param_value_size_ret) {
            *param_value_size_ret = sizeof(cl_uint);
        }
        return CL_SUCCESS;
    }
#endif
#if defined(cl_khr_pci_bus_info)
    case CL_DEVICE_PCI_BUS_INFO_KHR:
    {
        if (param_value_size == sizeof(cl_device_pci_bus_info_khr) && param_value) {
            cl_device_pci_bus_info_khr* pInfo = static_cast<cl_device_pci_bus_info_khr*>(param_value);
            pInfo->pci_domain = 0x11;
            pInfo->pci_bus = 0x22;
            pInfo->pci_device = 0x33;
            pInfo->pci_function = 0x44;
        }
        if (param_value_size_ret) {
            *param_value_size_ret = sizeof(cl_device_pci_bus_info_khr);
        }
        return CL_SUCCESS;
    }
#endif
    default: break;
    }
    TEST_FAIL();
    return CL_INVALID_OPERATION;
}

void testDeviceUUID_KHR(void)
{
#if defined(cl_khr_device_uuid)
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_2_0);
    clReleaseDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);

    cl::Device d0(make_device_id(0));

    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_uuid_pci_bus_info);

    std::array<cl_uchar, CL_UUID_SIZE_KHR> dev_uuid = d0.getInfo<CL_DEVICE_UUID_KHR>();
    for (int i = 0; i < CL_UUID_SIZE_KHR; i++) {
        TEST_ASSERT_EQUAL_UINT8(i + 1, dev_uuid[i]);
    }
    std::array<cl_uchar, CL_UUID_SIZE_KHR> drv_uuid = d0.getInfo<CL_DRIVER_UUID_KHR>();
    for (int i = 0; i < CL_UUID_SIZE_KHR; i++) {
        TEST_ASSERT_EQUAL_UINT8(i + 2, drv_uuid[i]);
    }

    cl_bool valid = d0.getInfo<CL_DEVICE_LUID_VALID_KHR>();
    TEST_ASSERT_EQUAL(CL_TRUE, valid);
    std::array<cl_uchar, CL_LUID_SIZE_KHR> luid = d0.getInfo<CL_DEVICE_LUID_KHR>();
    for (int i = 0; i < CL_LUID_SIZE_KHR; i++) {
        TEST_ASSERT_EQUAL_UINT8(i + 3, luid[i]);
    }

    cl_uint nodeMask = d0.getInfo<CL_DEVICE_NODE_MASK_KHR>();
    TEST_ASSERT_EQUAL(0xA5A5, nodeMask);
#endif
}

void testDevicePCIBusInfo_KHR(void)
{
#if defined(cl_khr_pci_bus_info)
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_2_0);
    clReleaseDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);

    cl::Device d0(make_device_id(0));

    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_uuid_pci_bus_info);

    cl_device_pci_bus_info_khr info = d0.getInfo<CL_DEVICE_PCI_BUS_INFO_KHR>();
    TEST_ASSERT_EQUAL_HEX(0x11, info.pci_domain);
    TEST_ASSERT_EQUAL_HEX(0x22, info.pci_bus);
    TEST_ASSERT_EQUAL_HEX(0x33, info.pci_device);
    TEST_ASSERT_EQUAL_HEX(0x44, info.pci_function);
#endif
}

static  cl_int clGetProgramInfo_testProgramGetContext(cl_program program,
    cl_program_build_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int /*num_calls*/)
{
    TEST_ASSERT_EQUAL_PTR(make_program(0), program);
    TEST_ASSERT_EQUAL_HEX(CL_PROGRAM_CONTEXT, param_name);
    TEST_ASSERT(param_value == nullptr || param_value_size >= sizeof(cl_context));
    if (param_value_size_ret != nullptr)
        *param_value_size_ret = sizeof(cl_context);
    if (param_value != nullptr)
        *static_cast<cl_context *>(param_value) = make_context(0);
    return CL_SUCCESS;
}

static cl_program clLinkProgram_testLinkProgram(cl_context context,
    cl_uint              num_devices,
    const cl_device_id * device_list,
    const char *         options,
    cl_uint              num_input_programs,
    const cl_program *   input_programs,
    void (CL_CALLBACK *  pfn_notify)(cl_program program, void * user_data),
    void *               user_data,
    cl_int *             errcode_ret,
    int                 /*num_calls*/)
{
    TEST_ASSERT_EQUAL_PTR(context, make_context(0));
    TEST_ASSERT_EQUAL(num_devices, 0);
    TEST_ASSERT_EQUAL(device_list, nullptr);
    TEST_ASSERT_EQUAL(options, nullptr);
    TEST_ASSERT_NOT_EQUAL(num_input_programs, 0);
    for (int i=0; i<(int)num_input_programs; i++)
        TEST_ASSERT_EQUAL_PTR(input_programs[i], make_program(i));
    TEST_ASSERT_EQUAL(pfn_notify, nullptr);
    TEST_ASSERT_EQUAL(user_data, nullptr);

    *errcode_ret = CL_SUCCESS;
    return make_program(0);
}

void testLinkProgram(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 120
    cl_int errcode;
    int refcount[] = {1,1};

    // verify if class cl::Program was not modified
    TEST_ASSERT_EQUAL(sizeof(cl_program), sizeof(cl::Program));

    clGetProgramInfo_StubWithCallback(clGetProgramInfo_testProgramGetContext);
    clLinkProgram_StubWithCallback(clLinkProgram_testLinkProgram);

    clRetainContext_ExpectAndReturn(make_context(0), CL_SUCCESS);
    clReleaseContext_ExpectAndReturn(make_context(0), CL_SUCCESS);
    prepare_programRefcounts(2, reinterpret_cast<cl_program *>(programPool), refcount);

    cl::Program prog = cl::linkProgram(cl::Program(make_program(0)), cl::Program(make_program(1)),
        nullptr, nullptr, nullptr, &errcode);

    TEST_ASSERT_EQUAL_PTR(prog(), make_program(0));
    TEST_ASSERT_EQUAL(errcode, CL_SUCCESS);

    prog() = nullptr;
#endif
}

void testLinkProgramWithVectorProgramInput(void)
{
#if CL_HPP_TARGET_OPENCL_VERSION >= 120
    cl_int errcode;
    VECTOR_CLASS<cl::Program> prog_vec;
    std::array<int, ARRAY_SIZE(programPool)> refcount;
    for (int i=0;i<(int)ARRAY_SIZE(programPool);i++) {
        prog_vec.push_back(cl::Program(programPool[i]()));
        refcount[i] = 1;
    }

    // verify if class cl::Program was not modified
    TEST_ASSERT_EQUAL(sizeof(cl_program), sizeof(cl::Program));

    clGetProgramInfo_StubWithCallback(clGetProgramInfo_testProgramGetContext);
    clLinkProgram_StubWithCallback(clLinkProgram_testLinkProgram);
    prepare_programRefcounts(prog_vec.size(), reinterpret_cast<cl_program *>(prog_vec.data()), refcount.data());

    clRetainContext_ExpectAndReturn(make_context(0), CL_SUCCESS);
    clReleaseContext_ExpectAndReturn(make_context(0), CL_SUCCESS);

    cl::Program prog = linkProgram(prog_vec, nullptr, nullptr, nullptr, &errcode);

    TEST_ASSERT_EQUAL_PTR(prog(), make_program(0));
    TEST_ASSERT_EQUAL(errcode, CL_SUCCESS);

    prog() = nullptr;
#endif
}

/****************************************************************************
 * Tests for cl::CommandBufferKhr
 ****************************************************************************/
#if defined(cl_khr_command_buffer)
void testMoveAssignCommandBufferKhrNonNull(void);
void testMoveAssignCommandBufferKhrNull(void);
void testMoveConstructCommandBufferKhrNonNull(void);
void testMoveConstructCommandBufferKhrNull(void);
MAKE_MOVE_TESTS(CommandBufferKhr, make_command_buffer_khr, clReleaseCommandBufferKHR, commandBufferKhrPool)
#else
void testMoveAssignCommandBufferKhrNonNull(void) {}
void testMoveAssignCommandBufferKhrNull(void) {}
void testMoveConstructCommandBufferKhrNonNull(void) {}
void testMoveConstructCommandBufferKhrNull(void) {}
#endif

// Stub for clGetCommandBufferInfoKHR that returns 1
static cl_int clGetCommandBufferInfoKHR_testCommandBufferKhrGetNumQueues(
    cl_command_buffer_khr command_buffer,
    cl_command_buffer_info_khr param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int /*num_calls*/)
{
    TEST_ASSERT_EQUAL_PTR(make_command_buffer_khr(0), command_buffer);
    TEST_ASSERT_EQUAL_HEX(CL_COMMAND_BUFFER_NUM_QUEUES_KHR, param_name);
    TEST_ASSERT(param_value == nullptr || param_value_size >= sizeof(cl_uint));
    if (param_value_size_ret != nullptr)
        *param_value_size_ret = sizeof(cl_uint);
    if (param_value != nullptr)
        *static_cast<cl_uint *> (param_value) = 1;
    return CL_SUCCESS;
}

void testCommandBufferInfoKHRNumQueues(void)
{
#if defined(cl_khr_command_buffer)
    cl_uint expected = 1;

    clGetCommandBufferInfoKHR_StubWithCallback(clGetCommandBufferInfoKHR_testCommandBufferKhrGetNumQueues);

    cl_uint num = commandBufferKhrPool[0].getInfo<CL_COMMAND_BUFFER_NUM_QUEUES_KHR>();
    TEST_ASSERT_EQUAL_HEX(expected, num);
#endif
}

// Stub for clGetCommandBufferInfoKHR that returns command queues array
static cl_int clGetCommandBufferInfoKHR_testCommandBufferKhrGetCommandQueues(
    cl_command_buffer_khr command_buffer,
    cl_command_buffer_info_khr param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret,
    int /*num_calls*/)
{
    TEST_ASSERT_EQUAL_PTR(make_command_buffer_khr(0), command_buffer);
    TEST_ASSERT_EQUAL_HEX(CL_COMMAND_BUFFER_QUEUES_KHR, param_name);
    TEST_ASSERT(param_value == nullptr || param_value_size >= 3 * sizeof(cl_command_queue));
    if (param_value_size_ret != nullptr)
        *param_value_size_ret = 3 * sizeof(cl_command_queue);
    if (param_value != nullptr)
    {
        cl_command_queue *command_queues = static_cast<cl_command_queue *> (param_value);
        command_queues[0] = make_command_queue(0);
        command_queues[1] = make_command_queue(1);
        command_queues[2] = make_command_queue(2);
    }
    return CL_SUCCESS;
}

void testCommandBufferInfoKHRCommandQueues(void)
{
#if defined(cl_khr_command_buffer)
    // creat expected values for refcounter
    VECTOR_CLASS<cl_command_queue> expected_queue_vec;
    std::array<int, 3> refcount;
    for (int i=0;i<3;i++) {
        expected_queue_vec.push_back(commandQueuePool[i]());
        refcount[i] = 1;
    }

    clGetCommandBufferInfoKHR_StubWithCallback(clGetCommandBufferInfoKHR_testCommandBufferKhrGetCommandQueues);
    prepare_commandQueueRefcounts(expected_queue_vec.size(), expected_queue_vec.data(), refcount.data());

    VECTOR_CLASS<cl::CommandQueue> command_queues = commandBufferKhrPool[0].getInfo<CL_COMMAND_BUFFER_QUEUES_KHR>();
    TEST_ASSERT_EQUAL(3, command_queues.size());
    TEST_ASSERT_EQUAL_PTR(make_command_queue(0), command_queues[0]());
    TEST_ASSERT_EQUAL_PTR(make_command_queue(1), command_queues[1]());
    TEST_ASSERT_EQUAL_PTR(make_command_queue(2), command_queues[2]());
#endif
}

/****************************************************************************
 * Tests for cl::Semaphore
 ****************************************************************************/
#if defined(cl_khr_semaphore)
void testMoveAssignSemaphoreNonNull(void);
void testMoveAssignSemaphoreNull(void);
void testMoveConstructSemaphoreNonNull(void);
void testMoveConstructSemaphoreNull(void);
MAKE_MOVE_TESTS(Semaphore, make_semaphore_khr, clReleaseSemaphoreKHR, semaphorePool);
#else
void testMoveAssignSemaphoreNonNull(void) {}
void testMoveAssignSemaphoreNull(void) {}
void testMoveConstructSemaphoreNonNull(void) {}
void testMoveConstructSemaphoreNull(void) {}
#endif

#if defined(cl_khr_semaphore)
static cl_int clEnqueueWaitSemaphoresKHR_testEnqueueWaitSemaphores(
    cl_command_queue command_queue,
    cl_uint num_sema_objects,
    const cl_semaphore_khr* sema_objects,
    const cl_semaphore_payload_khr* sema_payload_list,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event,
    int num_calls)
{
    TEST_ASSERT_EQUAL(0, num_calls);
    TEST_ASSERT_EQUAL_PTR(commandQueuePool[1](), command_queue);
    TEST_ASSERT_EQUAL(1, num_sema_objects);
    TEST_ASSERT_NOT_NULL(sema_objects);
    TEST_ASSERT_EQUAL(make_semaphore_khr(1), *sema_objects);
    TEST_ASSERT_NOT_NULL(sema_payload_list);
    TEST_ASSERT_EQUAL(0, num_events_in_wait_list);
    TEST_ASSERT_NULL(event_wait_list);

    if (event != nullptr)
    {
        *event = make_event(1);
    }

    return CL_SUCCESS;
}

void testEnqueueWaitSemaphores(void)
{
    clEnqueueWaitSemaphoresKHR_StubWithCallback(clEnqueueWaitSemaphoresKHR_testEnqueueWaitSemaphores);

    VECTOR_CLASS<cl::Semaphore> sema_objects;
    sema_objects.emplace_back(make_semaphore_khr(1));
    VECTOR_CLASS<cl_semaphore_payload_khr> sema_payloads(1);
    cl::Event event;

    cl_int status = commandQueuePool[1].enqueueWaitSemaphores(sema_objects, sema_payloads, nullptr, &event);
    TEST_ASSERT_EQUAL(CL_SUCCESS, status);
    TEST_ASSERT_EQUAL_PTR(make_event(1), event());

    // prevent destructor from interfering with the test
    event() = nullptr;
    sema_objects[0]() = nullptr;
}

static cl_int clEnqueueSignalSemaphoresKHR_testEnqueueSignalSemaphores(
    cl_command_queue command_queue,
    cl_uint num_sema_objects,
    const cl_semaphore_khr* sema_objects,
    const cl_semaphore_payload_khr* sema_payload_list,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event,
    int num_calls)
{
    TEST_ASSERT_EQUAL(0, num_calls);
    TEST_ASSERT_EQUAL_PTR(commandQueuePool[1](), command_queue);
    TEST_ASSERT_EQUAL(1, num_sema_objects);
    TEST_ASSERT_NOT_NULL(sema_objects);
    TEST_ASSERT_EQUAL(make_semaphore_khr(2), *sema_objects);
    TEST_ASSERT_NOT_NULL(sema_payload_list);
    TEST_ASSERT_EQUAL(0, num_events_in_wait_list);
    TEST_ASSERT_NULL(event_wait_list);

    if (event != nullptr)
    {
        *event = make_event(2);
    }

    return CL_SUCCESS;
}

void testEnqueueSignalSemaphores(void)
{
    clEnqueueSignalSemaphoresKHR_StubWithCallback(clEnqueueSignalSemaphoresKHR_testEnqueueSignalSemaphores);

    VECTOR_CLASS<cl::Semaphore> sema_objects;
    sema_objects.emplace_back(make_semaphore_khr(2));
    VECTOR_CLASS<cl_semaphore_payload_khr> sema_payloads(1);
    cl::Event event;

    cl_int status = commandQueuePool[1].enqueueSignalSemaphores(sema_objects, sema_payloads, nullptr, &event);
    TEST_ASSERT_EQUAL(CL_SUCCESS, status);
    TEST_ASSERT_EQUAL_PTR(make_event(2), event());

    // prevent destructor from interfering with the test
    event() = nullptr;
    sema_objects[0]() = nullptr;
}

cl_semaphore_khr clCreateSemaphoreWithProperties_testSemaphoreWithProperties(
    cl_context context,
    const cl_semaphore_properties_khr* sema_props,
    cl_int* errcode_ret,
    int num_calls)
{
    TEST_ASSERT_EQUAL(0, num_calls);
    TEST_ASSERT_EQUAL_PTR(context, contextPool[0]());
    TEST_ASSERT_NOT_NULL(sema_props);
    TEST_ASSERT_EQUAL(CL_SEMAPHORE_TYPE_KHR, *sema_props);
    TEST_ASSERT_NOT_NULL(errcode_ret);
    *errcode_ret = CL_SUCCESS;
    return make_semaphore_khr(1);
}

void testSemaphoreWithProperties(void)
{
    cl_device_id expected_device = make_device_id(0);
    int device_refcount = 1;

    clGetContextInfo_StubWithCallback(clGetContextInfo_device);
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_1);
    prepare_deviceRefcounts(1, &expected_device, &device_refcount);

    clCreateSemaphoreWithPropertiesKHR_StubWithCallback(clCreateSemaphoreWithProperties_testSemaphoreWithProperties);

    VECTOR_CLASS<cl_semaphore_properties_khr> sema_props{CL_SEMAPHORE_TYPE_KHR};
    cl_int err = CL_INVALID_OPERATION;
    cl::Semaphore sem(contextPool[0], sema_props, &err);

    TEST_ASSERT_EQUAL(CL_SUCCESS, err);
    TEST_ASSERT_EQUAL_PTR(make_semaphore_khr(1), sem());

    // prevent destructor from interfering with the test
    sem() = nullptr;
}

static cl_int clGetSemaphoreInfoKHR_testSemaphoreGetContext(
    cl_semaphore_khr sema_object,
    cl_semaphore_info_khr param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret,
    int num_calls)
{
    TEST_ASSERT_EQUAL_PTR(semaphorePool[0](), sema_object);
    TEST_ASSERT_EQUAL_HEX(CL_SEMAPHORE_CONTEXT_KHR, param_name);
    TEST_ASSERT(param_value == nullptr || param_value_size >= sizeof(cl_context));
    if (param_value_size_ret != nullptr)
        *param_value_size_ret = sizeof(cl_context);
    if (param_value != nullptr)
        *static_cast<cl_context *>(param_value) = make_context(0);

    return CL_SUCCESS;
}

void testSemaphoreGetInfoContext(void)
{
    cl_context expected_context = make_context(0);
    int context_refcount = 1;
    clGetSemaphoreInfoKHR_StubWithCallback(clGetSemaphoreInfoKHR_testSemaphoreGetContext);
    prepare_contextRefcounts(1, &expected_context, &context_refcount);

    cl_int err = CL_INVALID_OPERATION;

    cl::Context ctx = semaphorePool[0].getInfo<CL_SEMAPHORE_CONTEXT_KHR>(&err);

    TEST_ASSERT_EQUAL(CL_SUCCESS, err);
    TEST_ASSERT_EQUAL_PTR(make_context(0), ctx());
}

static cl_int clGetSemaphoreInfoKHR_testSemaphoreGetReferenceCount(
    cl_semaphore_khr sema_object,
    cl_semaphore_info_khr param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret,
    int num_calls)
{
    TEST_ASSERT_EQUAL_PTR(semaphorePool[0](), sema_object);
    TEST_ASSERT_EQUAL_HEX(CL_SEMAPHORE_REFERENCE_COUNT_KHR, param_name);
    TEST_ASSERT(param_value == nullptr || param_value_size >= sizeof(cl_uint));
    if (param_value_size_ret != nullptr)
        *param_value_size_ret = sizeof(cl_uint);
    if (param_value != nullptr)
        *static_cast<cl_uint *>(param_value) = 1;

    return CL_SUCCESS;
}

void testSemaphoreGetInfoReferenceCount(void)
{
    clGetSemaphoreInfoKHR_StubWithCallback(clGetSemaphoreInfoKHR_testSemaphoreGetReferenceCount);

    cl_int err = CL_INVALID_OPERATION;

    cl_uint ret = semaphorePool[0].getInfo<CL_SEMAPHORE_REFERENCE_COUNT_KHR>(&err);

    TEST_ASSERT_EQUAL(CL_SUCCESS, err);
    TEST_ASSERT_EQUAL(1, ret);
}

static cl_int clGetSemaphoreInfoKHR_testSemaphoreGetProperties(
    cl_semaphore_khr sema_object,
    cl_semaphore_info_khr param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret,
    int num_calls)
{
    static const cl_semaphore_properties_khr test_properties[] =
        {CL_SEMAPHORE_TYPE_KHR,
         CL_SEMAPHORE_TYPE_BINARY_KHR};
    TEST_ASSERT_EQUAL_PTR(semaphorePool[0](), sema_object);
    TEST_ASSERT_EQUAL_HEX(CL_SEMAPHORE_PROPERTIES_KHR, param_name);
    TEST_ASSERT(param_value == nullptr || param_value_size >= sizeof(test_properties));
    if (param_value_size_ret != nullptr)
        *param_value_size_ret = sizeof(test_properties);
    if (param_value != nullptr) {
        static_cast<cl_semaphore_properties_khr *>(param_value)[0] = test_properties[0];
        static_cast<cl_semaphore_properties_khr *>(param_value)[1] = test_properties[1];
    }

    return CL_SUCCESS;
}

void testSemaphoreGetInfoProperties(void)
{
    clGetSemaphoreInfoKHR_StubWithCallback(clGetSemaphoreInfoKHR_testSemaphoreGetProperties);

    cl_int err = CL_INVALID_OPERATION;

    VECTOR_CLASS<cl_semaphore_properties_khr> ret = semaphorePool[0].getInfo<CL_SEMAPHORE_PROPERTIES_KHR>(&err);

    TEST_ASSERT_EQUAL(CL_SUCCESS, err);
    TEST_ASSERT_EQUAL(2, ret.size());
    TEST_ASSERT_EQUAL(CL_SEMAPHORE_TYPE_KHR, ret[0]);
    TEST_ASSERT_EQUAL(CL_SEMAPHORE_TYPE_BINARY_KHR, ret[1]);
}

static cl_int clGetSemaphoreInfoKHR_testSemaphoreGetType(
    cl_semaphore_khr sema_object,
    cl_semaphore_info_khr param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret,
    int num_calls)
{
    TEST_ASSERT_EQUAL_PTR(semaphorePool[0](), sema_object);
    TEST_ASSERT_EQUAL_HEX(CL_SEMAPHORE_TYPE_KHR, param_name);
    TEST_ASSERT(param_value == nullptr || param_value_size >= sizeof(cl_semaphore_type_khr));
    if (param_value_size_ret != nullptr)
        *param_value_size_ret = sizeof(cl_semaphore_type_khr);
    if (param_value != nullptr)
        *static_cast<cl_semaphore_type_khr *>(param_value) = CL_SEMAPHORE_TYPE_BINARY_KHR;

    return CL_SUCCESS;
}

void testSemaphoreGetInfoType(void)
{
    clGetSemaphoreInfoKHR_StubWithCallback(clGetSemaphoreInfoKHR_testSemaphoreGetType);

    cl_int err = CL_INVALID_OPERATION;

    cl_semaphore_type_khr ret = semaphorePool[0].getInfo<CL_SEMAPHORE_TYPE_KHR>(&err);

    TEST_ASSERT_EQUAL(CL_SUCCESS, err);
    TEST_ASSERT_EQUAL(CL_SEMAPHORE_TYPE_BINARY_KHR, ret);
}

static cl_int clGetSemaphoreInfoKHR_testSemaphoreGetPayload(
    cl_semaphore_khr sema_object,
    cl_semaphore_info_khr param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret,
    int num_calls)
{
    TEST_ASSERT_EQUAL_PTR(semaphorePool[0](), sema_object);
    TEST_ASSERT_EQUAL_HEX(CL_SEMAPHORE_PAYLOAD_KHR, param_name);
    TEST_ASSERT(param_value == nullptr || param_value_size >= sizeof(cl_semaphore_payload_khr));
    if (param_value_size_ret != nullptr)
        *param_value_size_ret = sizeof(cl_semaphore_payload_khr);
    if (param_value != nullptr)
        *static_cast<cl_semaphore_payload_khr *>(param_value) = 1;

    return CL_SUCCESS;
}

void testSemaphoreGetInfoPayload(void)
{
    clGetSemaphoreInfoKHR_StubWithCallback(clGetSemaphoreInfoKHR_testSemaphoreGetPayload);

    cl_int err = CL_INVALID_OPERATION;

    cl_semaphore_payload_khr ret = semaphorePool[0].getInfo<CL_SEMAPHORE_PAYLOAD_KHR>(&err);

    TEST_ASSERT_EQUAL(CL_SUCCESS, err);
    TEST_ASSERT_EQUAL(1, ret);
}

static cl_int clGetSemaphoreInfoKHR_testSemaphoreGetDevices(
    cl_semaphore_khr sema_object,
    cl_semaphore_info_khr param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret,
    int num_calls)
{
    static const cl_device_id test_devices[] =
        {make_device_id(0), make_device_id(1)};
    TEST_ASSERT_EQUAL_PTR(semaphorePool[0](), sema_object);
    TEST_ASSERT_EQUAL_HEX(CL_DEVICE_HANDLE_LIST_KHR, param_name);
    TEST_ASSERT(param_value == nullptr || param_value_size >= sizeof(test_devices));
    if (param_value_size_ret != nullptr)
        *param_value_size_ret = sizeof(test_devices);
    if (param_value != nullptr) {
        static_cast<cl_device_id *>(param_value)[0] = test_devices[0];
        static_cast<cl_device_id *>(param_value)[1] = test_devices[1];
    }

    return CL_SUCCESS;
}

void testSemaphoreGetInfoDevicesList(void)
{
    cl_device_id expected_devices[] = {make_device_id(0), make_device_id(1)};
    int device_refcounts[] = {1, 1};

    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_1);
    prepare_deviceRefcounts(ARRAY_SIZE(expected_devices), expected_devices, device_refcounts);

    clGetSemaphoreInfoKHR_StubWithCallback(clGetSemaphoreInfoKHR_testSemaphoreGetDevices);

    cl_int err = CL_INVALID_OPERATION;

    VECTOR_CLASS<cl::Device> ret = semaphorePool[0].getInfo<CL_DEVICE_HANDLE_LIST_KHR>(&err);

    TEST_ASSERT_EQUAL(CL_SUCCESS, err);
    TEST_ASSERT_EQUAL(2, ret.size());
    TEST_ASSERT_EQUAL(make_device_id(0), ret[0]());
    TEST_ASSERT_EQUAL(make_device_id(1), ret[1]());
}

void testSemaphoreRetain(void)
{
    clRetainSemaphoreKHR_ExpectAndReturn(semaphorePool[0](), CL_SUCCESS);

    cl_int status = semaphorePool[0].retain();
    TEST_ASSERT_EQUAL(CL_SUCCESS, status);
}

void testSemaphoreRelease(void)
{
    clReleaseSemaphoreKHR_ExpectAndReturn(semaphorePool[0](), CL_SUCCESS);

    cl_int status = semaphorePool[0].release();
    TEST_ASSERT_EQUAL(CL_SUCCESS, status);
}

#else
void testEnqueueWaitSemaphores(void) {}
void testEnqueueSignalSemaphores(void) {}
void testSemaphoreWithProperties(void) {}
void testSemaphoreGetInfoContext(void) {}
void testSemaphoreGetInfoReferenceCount(void) {}
void testSemaphoreGetInfoProperties(void) {}
void testSemaphoreGetInfoType(void) {}
void testSemaphoreGetInfoPayload(void) {}
void testSemaphoreGetInfoDevicesList(void) {}
void testSemaphoreRetain(void) {}
void testSemaphoreRelease(void) {}
#endif // cl_khr_semaphore
// Tests for external semaphores:
#if defined(cl_khr_external_semaphore)

static void* make_external_semaphore_handle(
    cl_external_semaphore_handle_type_khr handle_type )
{
    return (void*)(uintptr_t)(handle_type << 16 | 0x1111);
}

static int make_external_semaphore_fd(
    cl_external_semaphore_handle_type_khr handle_type)
{
    return (int)(handle_type << 16 | 0x2222);
}

static cl_int clGetSemaphoreHandleForTypeKHR_GetHandles(
    cl_semaphore_khr sema_object,
    cl_device_id device,
    cl_external_semaphore_handle_type_khr handle_type,
    size_t handle_size,
    void* handle_ptr,
    size_t* handle_size_ret,
    int num_calls)
{
    (void) sema_object;
    (void) device;

    switch (handle_type) {
#if defined(cl_khr_external_semaphore_dx_fence)
    case CL_SEMAPHORE_HANDLE_D3D12_FENCE_KHR:
    {
        void* ret = make_external_semaphore_handle(handle_type);
        if (handle_size == sizeof(ret) && handle_ptr) {
            void** pHandle = static_cast<void**>(handle_ptr);
            *pHandle = ret;
        }
        if (handle_size_ret) {
            *handle_size_ret = sizeof(ret);
        }
        return CL_SUCCESS;
    }
#endif
#if defined(cl_khr_external_semaphore_win32)
    case CL_SEMAPHORE_HANDLE_OPAQUE_WIN32_KHR:
    case CL_SEMAPHORE_HANDLE_OPAQUE_WIN32_KMT_KHR:
    {
        void* ret = make_external_semaphore_handle(handle_type);
        if (handle_size == sizeof(ret) && handle_ptr) {
            void** pHandle = static_cast<void**>(handle_ptr);
            *pHandle = ret;
        }
        if (handle_size_ret) {
            *handle_size_ret = sizeof(ret);
        }
        return CL_SUCCESS;
    }
#endif
#if defined(cl_khr_external_semaphore_opaque_fd)
    case CL_SEMAPHORE_HANDLE_OPAQUE_FD_KHR:
    {
        int ret = make_external_semaphore_fd(handle_type);
        if (handle_size == sizeof(ret) && handle_ptr) {
            int* pHandle = static_cast<int*>(handle_ptr);
            *pHandle = ret;
        }
        if (handle_size_ret) {
            *handle_size_ret = sizeof(ret);
        }
        return CL_SUCCESS;
    }
#endif
#if defined(cl_khr_external_semaphore_opaque_fd)
    case CL_SEMAPHORE_HANDLE_SYNC_FD_KHR:
    {
        int ret = make_external_semaphore_fd(handle_type);
        if (handle_size == sizeof(ret) && handle_ptr) {
            int* pHandle = static_cast<int*>(handle_ptr);
            *pHandle = ret;
        }
        if (handle_size_ret) {
            *handle_size_ret = sizeof(ret);
        }
        return CL_SUCCESS;
    }
#endif
    default: break;
    }
    TEST_FAIL();
    return CL_INVALID_OPERATION;
}

void testTemplateGetSemaphoreHandleForTypeKHR()
{
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_2_0);
    clReleaseDevice_ExpectAndReturn(make_device_id(0), CL_SUCCESS);

    cl::Device device(make_device_id(0));

    clGetSemaphoreHandleForTypeKHR_StubWithCallback(clGetSemaphoreHandleForTypeKHR_GetHandles);

    cl::Semaphore semaphore;
#if defined(cl_khr_external_semaphore_dx_fence)
    {
        auto handle0 = semaphore.getHandleForTypeKHR<cl::ExternalSemaphoreType::D3D12Fence>(device);
        TEST_ASSERT_EQUAL(handle0, make_external_semaphore_handle(cl::ExternalSemaphoreType::D3D12Fence));
    }
#endif
#if defined(cl_khr_external_semaphore_opaque_fd)
    {
        auto fd0 = semaphore.getHandleForTypeKHR<cl::ExternalSemaphoreType::OpaqueFd>(device);
        TEST_ASSERT_EQUAL(fd0, make_external_semaphore_fd(cl::ExternalSemaphoreType::OpaqueFd));
    }
#endif
#if defined(cl_khr_external_semaphore_sync_fd)
    {
        auto fd1 = semaphore.getHandleForTypeKHR<cl::ExternalSemaphoreType::SyncFd>(device);
        TEST_ASSERT_EQUAL(fd1, make_external_semaphore_fd(cl::ExternalSemaphoreType::SyncFd));
    }
#endif
#if defined(cl_khr_external_semaphore_win32)
    {
        auto handle1 = semaphore.getHandleForTypeKHR<cl::ExternalSemaphoreType::OpaqueWin32>(device);
        TEST_ASSERT_EQUAL(handle1, make_external_semaphore_handle(cl::ExternalSemaphoreType::OpaqueWin32));
        auto handle2 = semaphore.getHandleForTypeKHR<cl::ExternalSemaphoreType::OpaqueWin32Kmt>(device);
        TEST_ASSERT_EQUAL(handle2, make_external_semaphore_handle(cl::ExternalSemaphoreType::OpaqueWin32Kmt));
    }
#endif
}

#endif // defined(cl_khr_external_semaphore)

/****************************************************************************
 * Tests for cl_khr_external_memory
 ****************************************************************************/
#ifdef cl_khr_external_memory
static cl_int clEnqueueAcquireExternalMemObjectsKHR_testEnqueueAcquireExternalMemObjects(
    cl_command_queue command_queue,
    cl_uint num_mem_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event,
    int num_calls)
{
    TEST_ASSERT_EQUAL(0, num_calls);
    TEST_ASSERT_EQUAL_PTR(commandQueuePool[0](), command_queue);
    TEST_ASSERT_EQUAL(1, num_mem_objects);
    TEST_ASSERT_EQUAL(make_mem(0), *mem_objects);
    TEST_ASSERT_EQUAL(0, num_events_in_wait_list);
    TEST_ASSERT_NULL(event_wait_list);

    if (event != nullptr)
    {
        *event = make_event(0);
    }

    return CL_SUCCESS;
}

void testEnqueueAcquireExternalMemObjects(void)
{
    clGetCommandQueueInfo_StubWithCallback(clGetCommandQueueInfo_testCommandQueueGetDevice);
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_1);

    clEnqueueAcquireExternalMemObjectsKHR_StubWithCallback(clEnqueueAcquireExternalMemObjectsKHR_testEnqueueAcquireExternalMemObjects);

    VECTOR_CLASS<cl::Memory> mem_objects;
    mem_objects.emplace_back(make_mem(0), false);
    cl::Event event;

    cl_int status = commandQueuePool[0].enqueueAcquireExternalMemObjects(mem_objects, nullptr, &event);

    TEST_ASSERT_EQUAL(CL_SUCCESS, status);
    TEST_ASSERT_EQUAL_PTR(make_event(0), event());

    // prevent destructor from interfering with the test
    event() = nullptr;
    mem_objects[0]() = nullptr;
}

static cl_int clEnqueueReleaseExternalMemObjectsKHR_testEnqueueReleaseExternalMemObjects(
    cl_command_queue command_queue,
    cl_uint num_mem_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event,
    int num_calls)
{
    TEST_ASSERT_EQUAL(0, num_calls);
    TEST_ASSERT_EQUAL_PTR(commandQueuePool[0](), command_queue);
    TEST_ASSERT_EQUAL(1, num_mem_objects);
    TEST_ASSERT_EQUAL(make_mem(0), *mem_objects);
    TEST_ASSERT_EQUAL(0, num_events_in_wait_list);
    TEST_ASSERT_NULL(event_wait_list);

    if (event != nullptr)
    {
        *event = make_event(0);
    }

    return CL_SUCCESS;
}

void testEnqueueReleaseExternalMemObjects(void)
{
    clGetCommandQueueInfo_StubWithCallback(clGetCommandQueueInfo_testCommandQueueGetDevice);
    clGetDeviceInfo_StubWithCallback(clGetDeviceInfo_platform);
    clGetPlatformInfo_StubWithCallback(clGetPlatformInfo_version_1_1);

    clEnqueueReleaseExternalMemObjectsKHR_StubWithCallback(clEnqueueReleaseExternalMemObjectsKHR_testEnqueueReleaseExternalMemObjects);

    VECTOR_CLASS<cl::Memory> mem_objects;
    mem_objects.emplace_back(make_mem(0), false);
    cl::Event event;

    cl_int status = commandQueuePool[0].enqueueReleaseExternalMemObjects(mem_objects, nullptr, &event);

    TEST_ASSERT_EQUAL(CL_SUCCESS, status);
    TEST_ASSERT_EQUAL_PTR(make_event(0), event());

    // prevent destructor from interfering with the test
    event() = nullptr;
    mem_objects[0]() = nullptr;
}

#else
void testEnqueueAcquireExternalMemObjects(void) {}
void testEnqueueReleaseExternalMemObjects(void) {}
#endif // cl_khr_external_memory

} // extern "C"
