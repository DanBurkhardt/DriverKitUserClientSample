/*
See LICENSE folder for this sample’s licensing information.

Abstract:
A DriverKit null driver implementation that only logs interactions with its client.
*/

#include <os/log.h>

#include <DriverKit/IOLib.h>
#include <DriverKit/IOMemoryMap.h>
#include <DriverKit/IOTimerDispatchSource.h>
#include <DriverKit/IOUserClient.h>
#include <DriverKit/OSData.h>

#include <time.h>

#include "NullDriver.h"

// This log to makes it easier to parse out individual logs from the driver, since all logs will be prefixed with the same word/phrase.
// DriverKit logging has no logging levels; some developers might want to prefix errors differently than info messages.
// Another option is to #define a "debug" case where some log messages only exist when doing a debug build.
// To search for logs from this driver, use either: `sudo dmesg | grep NullDriver` or use Console.app search to find messages that start with "NullDriver -".
#define Log(fmt, ...) os_log(OS_LOG_DEFAULT, "NullDriver - " fmt "\n", ##__VA_ARGS__)

// This struct should ideally be shared across all applications using the driver, as the integer values need to be the same across all programs.
typedef enum
{
    ExternalMethodType_Scalar = 0,
    ExternalMethodType_Struct = 1,
    ExternalMethodType_CheckedScalar = 2,
    ExternalMethodType_CheckedStruct = 3,
    ExternalMethodType_RegisterAsyncCallback = 4,
    ExternalMethodType_AsyncRequest = 5,
    NumberOfExternalMethods // Has to be last
} ExternalMethodType;

// These "DataStruct" structures are what used to discuss with our Dext.
typedef struct
{
    uint64_t foo;
    uint64_t bar;
} DataStruct;

typedef struct {
    uint64_t foo;
    uint64_t bar;
    uint64_t largeArray[511];
} OversizedDataStruct;


const IOUserClientMethodDispatch externalMethodChecks[NumberOfExternalMethods] = {
    // ExternalMethodType_Scalar and ExternalMethodType_Struct are intentionally omitted.
    // This is so they can be called directly, which is not recommended, but provided for ease of understanding.
    // Instead, prefer the "_Checked" methods, as they are safer and less prone to attacks.
    //
    // When possible choose exact sizes for all variables for security reasons, but if a size must be variable, use kIOUserClientVariableStructureSize.
    /// - Tag: ClientMethodDispatch_CheckedScalar
    [ExternalMethodType_CheckedScalar] =
    {
        .function = (IOUserClientMethodFunction) &NullDriver::StaticHandleExternalCheckedScalar,
        .checkCompletionExists = false, // Since this call doesn't use a callback, this value is false and IOUserClientMethodArguments.completion must be 0.
        .checkScalarInputCount = 16,
        .checkStructureInputSize = 0,
        .checkScalarOutputCount = 16,
        .checkStructureOutputSize = 0,
    },
    [ExternalMethodType_CheckedStruct] =
    {
        .function = (IOUserClientMethodFunction) &NullDriver::StaticHandleExternalCheckedStruct,
        .checkCompletionExists = false,
        .checkScalarInputCount = 0, // No scalar inputs
        .checkStructureInputSize = sizeof(DataStruct), // Avoid using variables unless absolutely required. Using the size of a structure is safer.
        .checkScalarOutputCount = 0,
        .checkStructureOutputSize = sizeof(DataStruct),
    },
    // The async methods follow the same flow as the checked methods, so the checks and function implementations are similar.
    [ExternalMethodType_RegisterAsyncCallback] =
    {
        .function = (IOUserClientMethodFunction) &NullDriver::StaticRegisterAsyncCallback,
        .checkCompletionExists = true,
        .checkScalarInputCount = 0,
        .checkStructureInputSize = sizeof(DataStruct),
        .checkScalarOutputCount = 0,
        .checkStructureOutputSize = sizeof(DataStruct),
    },
    [ExternalMethodType_AsyncRequest] =
    {
        .function = (IOUserClientMethodFunction) &NullDriver::StaticHandleAsyncRequest,
        .checkCompletionExists = -1U, // Don't care about completion
        .checkScalarInputCount = 0,
        .checkStructureInputSize = sizeof(DataStruct),
        .checkScalarOutputCount = 0,
        .checkStructureOutputSize = 0,
    },
};

/// - Tag: Struct_NullDriver_IVars
struct NullDriver_IVars {
    OSAction* callbackAction = nullptr;
    IODispatchQueue* dispatchQueue = nullptr;
    IOTimerDispatchSource* dispatchSource = nullptr;
    OSAction* simulatedAsyncDeviceResponseAction = nullptr;
};


// MARK: Dext Lifecycle Management
bool NullDriver::init(void)
{
    bool result = false;

    Log("init()");

    result = super::init();
    if (result != true)
    {
        Log("init() - super::init failed.");
        goto Exit;
    }

    ivars = IONewZero(NullDriver_IVars, 1);
    if (ivars == nullptr)
    {
        Log("init() - Failed to allocate memory for ivars.");
        goto Exit;
    }

    Log("init() - Finished.");
    return true;

Exit:
    return false;
}

kern_return_t IMPL(NullDriver, Start)
{
    kern_return_t ret = kIOReturnSuccess;

    ret = Start(provider, SUPERDISPATCH);
    if (ret != kIOReturnSuccess)
    {
        Log("Start() - super::Start failed with error: 0x%08x.", ret);
        goto Exit;
    }

    ret = IODispatchQueue::Create("NullDriverDispatchQueue", 0, 0, &ivars->dispatchQueue);
    if (ret != kIOReturnSuccess)
    {
        Log("Start() - Failed to create dispatch queue with error: 0x%08x.", ret);
        goto Exit;
    }

    ret = IOTimerDispatchSource::Create(ivars->dispatchQueue, &ivars->dispatchSource);
    if (ret != kIOReturnSuccess)
    {
        Log("Start() - Failed to create dispatch source with error: 0x%08x.", ret);
        goto Exit;
    }

    // This sample simulates async events using the IOTimerDispatchSource, which calls the "SimulatedAsyncEvent" method.
    // The size is to store all of the necessary data for the responses.
    // "CreateActionSimulatedAsyncEvent" was created automatically by the TYPE macro in the .iig file.
    /// - Tag: Start_InitializeSimulatedAsyncDeviceResponseAction
    ret = CreateActionSimulatedAsyncEvent(sizeof(DataStruct), &ivars->simulatedAsyncDeviceResponseAction);
    if (ret != kIOReturnSuccess)
    {
        Log("Start() - Failed to create action for simulated async event with error: 0x%08x.", ret);
        goto Exit;
    }
    
    // Set up our IOTimerDispatchSource to call our "SimulatedAsyncEvent" method through our OSAction
    ret = ivars->dispatchSource->SetHandler(ivars->simulatedAsyncDeviceResponseAction);
    if (ret != kIOReturnSuccess)
    {
        Log("Start() - Failed to assign simulated action to handler with error: 0x%08x.", ret);
        goto Exit;
    }

    ret = RegisterService();
    if (ret != kIOReturnSuccess)
    {
        Log("Start() - Failed to register service with error: 0x%08x.", ret);
        goto Exit;
    }

    Log("Start() - Finished.");
    ret = kIOReturnSuccess;

Exit:
    return ret;
}

kern_return_t IMPL(NullDriver, Stop)
{
    kern_return_t ret = kIOReturnSuccess;
    __block _Atomic uint32_t cancelCount = 0;

    Log("Stop()");

    // Add a cancel count for each of these items that needs to be cancelled.
    if (ivars->simulatedAsyncDeviceResponseAction != nullptr)
    {
        ++cancelCount;
    }

    if (ivars->dispatchSource != nullptr)
    {
        ++cancelCount;
    }

    if (ivars->dispatchQueue != nullptr)
    {
        ++cancelCount;
    }

    if (ivars->callbackAction != nullptr)
    {
        ++cancelCount;
    }

    // If there's somehow nothing to cancel, "Stop" quickly and exit.
    if (cancelCount == 0)
    {
        ret = Stop(provider, SUPERDISPATCH);
        if (ret != kIOReturnSuccess)
        {
            Log("Stop() - super::Stop failed with error: 0x%08x.", ret);
        }

        Log("Stop() - Finished.");

        return ret;
    }
    // Otherwise, wait for some Cancels to get completed.

    // Retain the driver instance and the provider so the finalization can properly stop the driver
    this->retain();
    provider->retain();

    // Re-use this block, with each cancel action taking a count off, until the last cancel stops the dext
    void (^finalize)(void) = ^{

        if (__c11_atomic_fetch_sub(&cancelCount, 1U, __ATOMIC_RELAXED) <= 1) {

            kern_return_t status = Stop(provider, SUPERDISPATCH);
            if (status != kIOReturnSuccess)
            {
                Log("Stop() - super::Stop failed with error: 0x%08x.", status);
            }

            Log("Stop() - Finished.");

            this->release();
            provider->release();
        }
    };

    // All of these will call the "finalize" block, but only the final one to finish canceling will stop the dext

    if (ivars->simulatedAsyncDeviceResponseAction != nullptr)
    {
        ivars->simulatedAsyncDeviceResponseAction->Cancel(finalize);
    }

    if (ivars->dispatchSource != nullptr)
    {
        ivars->dispatchSource->Cancel(finalize);
    }

    if (ivars->dispatchQueue != nullptr)
    {
        ivars->dispatchQueue->Cancel(finalize);
    }

    if (ivars->callbackAction != nullptr)
    {
        ivars->callbackAction->Cancel(finalize);
    }
    
    Log("Stop() - Cancels started, they will stop the dext later.");

    return ret;
}

void NullDriver::free(void)
{
    Log("free()");

    OSSafeReleaseNULL(ivars->simulatedAsyncDeviceResponseAction);
    OSSafeReleaseNULL(ivars->dispatchSource);
    OSSafeReleaseNULL(ivars->dispatchQueue);
    OSSafeReleaseNULL(ivars->callbackAction);

    IOSafeDeleteNULL(ivars, NullDriver_IVars, 1);

    super::free();
}

// When an application attaches to the dext via IOServiceOpen, this method is called
kern_return_t IMPL(NullDriver, NewUserClient)
{
    kern_return_t ret = kIOReturnSuccess;
    IOService* client = nullptr;

    Log("NewUserClient()");

    ret = Create(this, "UserClientProperties", &client);
    if (ret != kIOReturnSuccess)
    {
        Log("NewUserClient() - Failed to create UserClientProperties with error: 0x%08x.", ret);
        goto Exit;
    }

    *userClient = OSDynamicCast(IOUserClient, client);
    if (*userClient == NULL)
    {
        Log("NewUserClient() - Failed to cast new client.");
        client->release();
        ret = kIOReturnError;
        goto Exit;
    }

    Log("NewUserClient() - Finished.");

Exit:
    return ret;
}

// MARK: ExternalMethod Handler
// This method is called when an application calls IOConnectCall...Method.
// All of the passed inputs and outputs are accessible through the "arguments" variable.
// Never trust any of the data that has been passed in, such as sizes, as these can be used as attack vectors on your code.
// For example, an incorrect size may allow for a buffer overflow attack on the dext.
kern_return_t NullDriver::ExternalMethod(uint64_t selector, IOUserClientMethodArguments* arguments, const IOUserClientMethodDispatch* dispatch, OSObject* target, void* reference)
{
    kern_return_t ret = kIOReturnSuccess;

    // Check to make sure that the call doesn't interfere with the minimum of the un-checked methods, for the sake of this example.
    // Always check to make sure that the selector is not greater than the number of options in the IOUserClientMethodDispatch.
    if (selector >= ExternalMethodType_CheckedScalar)
    {
        if (selector < NumberOfExternalMethods)
        {
            dispatch = &externalMethodChecks[selector];
            if (!target)
            {
                target = this;
            }
        }

        // This will call the functions as defined in the IOUserClientMethodDispatch.
        return super::ExternalMethod(selector, arguments, dispatch, target, reference);
    }

    switch (selector)
    {
        case ExternalMethodType_Scalar:
        {
            ret = HandleExternalScalar(arguments);
        } break;

        case ExternalMethodType_Struct:
        {
            ret = HandleExternalStruct(arguments);
        } break;

        default:
        {
            ret = kIOReturnBadArgument;
        } break;
    }

    return ret;
}

// MARK: Unsafe External Handlers
kern_return_t NullDriver::HandleExternalScalar(IOUserClientMethodArguments* arguments)
{
    kern_return_t ret = kIOReturnSuccess;
    uint8_t inputCount = 0;
    uint8_t outputCount = 0;
    uint8_t iterCount = 0;

    Log("Got action type scalar");

    if (arguments == nullptr)
    {
        Log("Arguments were null.");
        ret = kIOReturnBadArgument;
        goto Exit;
    }

    // Input could be spoofed by an attacker. Always make sure that it is a reasonable value.
    if (arguments->scalarInput == nullptr)
    {
        Log("Scalar input memory is null.");
        ret = kIOReturnBadArgument;
        goto Exit;
    }

    // Input count could be spoofed by an attacker. Make sure to limit to a reasonable amount of values.
    inputCount = arguments->scalarInputCount;
    if (inputCount > 16)
    {
        Log("Only allowing a maximum number of 16 inputs.");
        inputCount = 16;
    }

    if (arguments->scalarOutput == nullptr)
    {
        Log("Scalar output memory is null.");
        ret = kIOReturnBadArgument;
        goto Exit;
    }

    // Output count could be spoofed by an attacker. Make sure to limit to a reasonable amount of values.
    outputCount = arguments->scalarOutputCount;
    if (outputCount > 16)
    {
        Log("Only allowing a maximum number of 16 outputs.");
        outputCount = 16;
    }

    // Limit iterations to the smaller of the two counts, to prevent a buffer overrun on either side.
    iterCount = (inputCount < outputCount) ? inputCount : outputCount;
    for (int16_t index = 0; index < iterCount; ++index)
    {
        arguments->scalarOutput[index] = arguments->scalarInput[index] + 1;
        Log("%llu %llu", arguments->scalarInput[index], arguments->scalarOutput[index]);
    }

Exit:
    return ret;
}

kern_return_t NullDriver::HandleExternalStruct(IOUserClientMethodArguments* arguments)
{
    kern_return_t ret = kIOReturnSuccess;

    DataStruct* input = nullptr;
    DataStruct output = {};

    IOMemoryMap* inputMap = nullptr;
    IOMemoryMap* outputMap = nullptr;


    Log("Got action type struct");

    if (arguments == nullptr)
    {
        Log("Arguments were null.");
        ret = kIOReturnBadArgument;
        goto Exit;
    }

    if (arguments->structureInput != nullptr)
    {
        input = (DataStruct*)arguments->structureInput->getBytesNoCopy();
    }
    else if (arguments->structureInputDescriptor != nullptr)
    {
        ret = arguments->structureInputDescriptor->CreateMapping(0, 0, 0, 0, 0, &inputMap);
        if (ret != kIOReturnSuccess)
        {
            Log("Failed to create mapping for descriptor with error: 0x%08x", ret);
            PrintExtendedErrorInfo(ret);
            ret = kIOReturnBadArgument;
            goto Exit;
        }

        input = (DataStruct*)inputMap->GetAddress();
    }
    else
    {
        Log("Both structureInput and structureInputDescriptor were null.");
        ret = kIOReturnBadArgument;
        goto Exit;
    }

    if (input == nullptr)
    {
        Log("Failed to get input.");
        ret = kIOReturnBadArgument;
        goto Exit;
    }
    Log("Input - %llu, %llu", input->foo, input->bar);

    output.foo = input->foo + 1;
    output.bar = input->bar + 10;
    Log("Output - %llu, %llu", output.foo, output.bar);

    // Memory is not passed from the caller into the dext.
    // The dext needs to create its own OSData to hold this information in order to pass it back to the caller.
    if (arguments->structureOutputDescriptor != nullptr)
    {
        if (sizeof(OversizedDataStruct) > arguments->structureOutputMaximumSize)
        {
            Log("Required output size of %lu is larger than the given maximum size of %llu. Failing.", sizeof(OversizedDataStruct), arguments->structureOutputMaximumSize);
        }

        arguments->structureOutputDescriptor->CreateMapping(0, 0, 0, 0, 0, &outputMap);
        uint8_t* outputPtr = (uint8_t*)outputMap->GetAddress();

        // Copy the data from DataStruct over and then fill the rest with zeroes.
        memcpy(outputPtr, &output, sizeof(DataStruct));
        memset(outputPtr + sizeof(DataStruct), 0, sizeof(OversizedDataStruct) - sizeof(DataStruct));
    }
    else
    {
        arguments->structureOutput = OSData::withBytes(&output, sizeof(DataStruct));
    }

Exit:
    OSSafeReleaseNULL(inputMap);
    OSSafeReleaseNULL(outputMap);

    return ret;
}

//MARK: Static External Handlers
kern_return_t NullDriver::StaticHandleExternalCheckedScalar(OSObject* target, void* reference, IOUserClientMethodArguments* arguments)
{
    if (target == nullptr)
    {
        return kIOReturnError;
    }

    return ((NullDriver*)target)->HandleExternalCheckedScalar(reference, arguments);
}

kern_return_t NullDriver::StaticHandleExternalCheckedStruct(OSObject* target, void* reference, IOUserClientMethodArguments* arguments)
{
    if (target == nullptr)
    {
        return kIOReturnError;
    }

    return ((NullDriver*)target)->HandleExternalCheckedStruct(reference, arguments);
}

kern_return_t NullDriver::StaticRegisterAsyncCallback(OSObject* target, void* reference, IOUserClientMethodArguments* arguments)
{
    if (target == nullptr)
    {
        return kIOReturnError;
    }

    return ((NullDriver*)target)->RegisterAsyncCallback(reference, arguments);
}

kern_return_t NullDriver::StaticHandleAsyncRequest(OSObject* target, void* reference, IOUserClientMethodArguments* arguments)
{
    if (target == nullptr)
    {
        return kIOReturnError;
    }

    return ((NullDriver*)target)->HandleAsyncRequest(reference, arguments);
}

// MARK: Safer External Handlers
kern_return_t NullDriver::HandleExternalCheckedScalar(void* reference, IOUserClientMethodArguments* arguments)
{
    // This function was checked by IOUserClientMethodDispatch, so it doesn't need to validate the arguments.

    kern_return_t ret = kIOReturnSuccess;

    Log("Got action type checked scalar");

    for (int16_t index = 0; index < arguments->scalarOutputCount; ++index)
    {
        arguments->scalarOutput[index] = arguments->scalarInput[index] + 1;
        Log("%llu %llu", arguments->scalarInput[index], arguments->scalarOutput[index]);
    }

    return ret;
}

kern_return_t NullDriver::HandleExternalCheckedStruct(void* reference, IOUserClientMethodArguments* arguments)
{
    // This function was checked by IOUserClientMethodDispatch, so it doesn't need to validate the arguments.

    kern_return_t ret = kIOReturnSuccess;

    DataStruct* input = nullptr;
    DataStruct output = {};

    Log("Got action type checked struct");

    input = (DataStruct*)arguments->structureInput->getBytesNoCopy();

    output.foo = input->foo + 1;
    output.bar = input->bar + 10;

    arguments->structureOutput = OSData::withBytes(&output, sizeof(DataStruct));

    return ret;
}

kern_return_t NullDriver::RegisterAsyncCallback(void* reference, IOUserClientMethodArguments* arguments)
{
    Log("Got new async callback");

    DataStruct* input = nullptr;
    DataStruct output = {};

    /// - Tag: RegisterAsyncCallback_StoreCompletion
    if (arguments->completion == nullptr)
    {
        Log("Got a null completion.");
        return kIOReturnBadArgument;
    }

    // Save the completion for later.
    // If not saved, then it might be freed before the asychronous return.
    ivars->callbackAction = arguments->completion;
    ivars->callbackAction->retain();

    // All of this is returned synchronously.
    // This is provided for the sake of example.
    // Generally a dext would want to return from an async method as fast as possible.
    /// - Tag: RegisterAsyncCallback_CallCompletion
    input = (DataStruct*)arguments->structureInput->getBytesNoCopy();

    // Retain action memory for later work.
    void* osActionRetainedMemory = ivars->simulatedAsyncDeviceResponseAction->GetReference();
    memcpy(osActionRetainedMemory, input, sizeof(DataStruct));

    output.foo = input->foo + 1;
    output.bar = input->bar + 10;

    arguments->structureOutput = OSData::withBytes(&output, sizeof(DataStruct));

    // Dispatch action that waits five to seven seconds and then calls the callback.
    const uint64_t fiveSecondsInNanoSeconds = 5000000000;
    const uint64_t twoSecondsInNanoSeconds = 2000000000;
    uint64_t currentTime = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);

    Log("Sleeping async...");
    ivars->dispatchSource->WakeAtTime(kIOTimerClockMonotonicRaw, currentTime + fiveSecondsInNanoSeconds, twoSecondsInNanoSeconds);

    return kIOReturnSuccess;
}

kern_return_t NullDriver::HandleAsyncRequest(void* reference, IOUserClientMethodArguments* arguments)
{
    Log("Got action type async.");

    // This function executes synchronously and blocks the caller,
    // so it needs to check its inputs as fast as possible,
    // and then spawn a worker thread to take care of any nonblocking work.
    DataStruct* inputPtr = (DataStruct*)arguments->structureInput->getBytesNoCopy();

    if (ivars->callbackAction == nullptr)
    {
        Log("Callback action not available.");
        return kIOReturnError;
    }

    // Retain action memory for later work.
    void* osActionRetainedMemory = ivars->simulatedAsyncDeviceResponseAction->GetReference();
    memcpy(osActionRetainedMemory, inputPtr, sizeof(DataStruct));

    // Dispatch action that waits five to seven seconds and then calls the callback.
    const uint64_t fiveSecondsInNanoSeconds = 5000000000;
    const uint64_t twoSecondsInNanoSeconds = 2000000000;
    uint64_t currentTime = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);

    Log("Sleeping async...");
    ivars->dispatchSource->WakeAtTime(kIOTimerClockMonotonicRaw, currentTime + fiveSecondsInNanoSeconds, twoSecondsInNanoSeconds);

    return kIOReturnSuccess;
}

// MARK: SimulatedAsyncEvent Callback
void IMPL(NullDriver, SimulatedAsyncEvent)
{
    Log("Woke async at time: %llu!", time);

    // Get back our data previously stored in OSAction.
    DataStruct* input = (DataStruct*)action->GetReference();

    DataStruct output = {};
    output.foo = input->foo + 1;
    output.bar = input->bar + 10;

    uint64_t asyncData[3] = { 2 };
    memcpy(asyncData + 1, &output, sizeof(DataStruct));

    if (ivars->callbackAction != nullptr)
    {
        // 3 is the 1 leading "type" message plus the two elements of the DataStruct.
        AsyncCompletion(ivars->callbackAction, kIOReturnSuccess, asyncData, 3);
    }
}

// MARK: Detail Helpers
void NullDriver::PrintExtendedErrorInfo(kern_return_t ret)
{
    Log("\tSystem: 0x%02x", err_get_system(ret));
    Log("\tSubsystem: 0x%03x", err_get_sub(ret));
    Log("\tCode: 0x%04x", err_get_code(ret));
}
