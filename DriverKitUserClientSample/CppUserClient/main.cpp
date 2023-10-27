/*
See LICENSE folder for this sample’s licensing information.

Abstract:
An interactive command-line client for calling the installed null driver.
*/

#include <iostream>
#include <IOKit/usb/USB.h>
#include <IOKit/IOReturn.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hidsystem/IOHIDShared.h>

typedef struct {
    uint64_t foo;
    uint64_t bar;
} DataStruct;

typedef struct {
    uint64_t foo;
    uint64_t bar;
    uint64_t largeArray[511];
} OversizedDataStruct;

CFRunLoopRef globalRunLoop = nullptr;

inline void PrintArray(const uint64_t* ptr, const uint32_t length)
{
    printf("{ ");
    for (uint32_t idx = 0; idx < length; ++idx)
    {
        printf("%llu ", ptr[idx]);
    }
    printf("} \n");
}

inline void PrintStruct(const DataStruct* ptr)
{
    printf("{\n");
    printf("\t.foo = %llu,\n", ptr->foo);
    printf("\t.bar = %llu,\n", ptr->bar);
    printf("}\n");
}

inline void PrintStruct(const OversizedDataStruct* ptr)
{
    printf("{\n");
    printf("\t.foo = %llu,\n", ptr->foo);
    printf("\t.bar = %llu,\n", ptr->bar);
    printf("\t.largeArray[0] = %llu,\n", ptr->largeArray[0]);
    printf("}\n");
}

inline void PrintErrorDetails(kern_return_t ret)
{
    printf("\tSystem: 0x%02x\n", err_get_system(ret));
    printf("\tSubsystem: 0x%03x\n", err_get_sub(ret));
    printf("\tCode: 0x%04x\n", err_get_code(ret));
}

// For more detail on this callback format, view the format of:
// IOAsyncCallback, IOAsyncCallback0, IOAsyncCallback1, IOAsyncCallback2
// Note that the variant of IOAsyncCallback called is based on the number of arguments being returned
// 0 - IOAsyncCallback0
// 1 - IOAsyncCallback1
// 2 - IOAsyncCallback2
// 3+ - IOAsyncCallback
// This is an example of the "IOAsyncCallback" format.
// refcon will be the value you placed in asyncRef[kIOAsyncCalloutRefconIndex]
static void AsyncCallback(void* refcon, IOReturn result, void** args, uint32_t numArgs)
{
    const char* funcName = nullptr;
    uint64_t* arrArgs = (uint64_t*)args;
    DataStruct* output = (DataStruct*)(arrArgs + 1);

    switch (arrArgs[0])
    {
        case 1:
        {
            funcName = "'Register Async Callback'";
        } break;

        case 2:
        {
            funcName = "'Async Request'";
        } break;

        default:
        {
            funcName = "UNKNOWN";
        } break;
    }

    printf("Got callback of %s from dext with returned data ", funcName);
    PrintStruct(output);
    printf("with return code: 0x%08x.\n", result);

    // Stop the run loop so our program can return to normal processing.
    CFRunLoopStop(globalRunLoop);
}

int main(int argc, const char* argv[])
{
    bool runProgram = true;
    
    // If you don't know what value to use here, if should be identical to the IOUserClass value in your UserClientProperties.
    // You can double check by searching with the `ioreg` command in your terminal.
    // It will be of type "IOUserService" not "IOUserServer"
    /// - Tag: ClientApp_Declarations
    static const char* dextIdentifier = "NullDriver";

    kern_return_t ret = kIOReturnSuccess;
    io_iterator_t iterator = IO_OBJECT_NULL;
    io_service_t service = IO_OBJECT_NULL;
    io_connect_t connection = IO_OBJECT_NULL;

    // Async required variables
    IONotificationPortRef notificationPort = nullptr;
    mach_port_t machNotificationPort = NULL;
    CFRunLoopSourceRef runLoopSource = nullptr;
    io_async_ref64_t asyncRef = {};

    /// - Tag: ClientApp_Connect
    ret = IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceNameMatching(dextIdentifier), &iterator);
    if (ret != kIOReturnSuccess)
    {
        printf("Unable to find service for identifier with error: 0x%08x.\n", ret);
        PrintErrorDetails(ret);
    }

    printf("Searching for dext service...\n");
    while ((service = IOIteratorNext(iterator)) != IO_OBJECT_NULL)
    {
        // Open a connection to this user client as a server to that client, and store the instance in "service"
        ret = IOServiceOpen(service, mach_task_self_, kIOHIDServerConnectType, &connection);

        if (ret == kIOReturnSuccess)
        {
            printf("\tOpened service.\n");
            break;
        }
        else
        {
            printf("\tFailed opening service with error: 0x%08x.\n", ret);
        }

        IOObjectRelease(service);
    }
    IOObjectRelease(iterator);

    if (service == IO_OBJECT_NULL)
    {
        printf("Failed to match to device.\n");
        return EXIT_FAILURE;
    }


    // Async initialization
    globalRunLoop = CFRunLoopGetCurrent();
    CFRetain(globalRunLoop);

    notificationPort = IONotificationPortCreate(kIOMasterPortDefault);
    if (notificationPort == nullptr)
    {
        printf("Failed to create notification port for application.\n");
        return EXIT_FAILURE;
    }

    machNotificationPort = IONotificationPortGetMachPort(notificationPort);
    if (machNotificationPort == 0)
    {
        printf("Failed to get mach notification port for application.\n");
        return EXIT_FAILURE;
    }

    runLoopSource = IONotificationPortGetRunLoopSource(notificationPort);
    if (runLoopSource == nullptr)
    {
        printf("Failed to get run loop for application.\n");
        return EXIT_FAILURE;
    }

    // Establish our notifications in the run loop, so we can get callbacks.
    CFRunLoopAddSource(globalRunLoop, runLoopSource, kCFRunLoopDefaultMode);

    // Establish our "AsyncCallback" function as the function that will be called by our Dext when it calls its "AsyncCompletion" function.
    // We'll use kIOAsyncCalloutFuncIndex and kIOAsyncCalloutRefconIndex to define the parameters for our async callback
    // This is your callback function. Check the definition for more details.
    asyncRef[kIOAsyncCalloutFuncIndex] = (io_user_reference_t)AsyncCallback;
    // Use this for context on the return. For example you might pass "this". But since this example is entirely static, we'll skip that step.
    asyncRef[kIOAsyncCalloutRefconIndex] = (io_user_reference_t)nullptr;


    // Main input loop of our program
    while (runProgram)
    {
        constexpr uint32_t MessageType_Scalar = 0;
        constexpr uint32_t MessageType_Struct = 1;
        constexpr uint32_t MessageType_CheckedScalar = 2;
        constexpr uint32_t MessageType_CheckedStruct = 3;
        constexpr uint32_t MessageType_RegisterAsyncCallback = 4;
        constexpr uint32_t MessageType_AsyncRequest = 5;

        uint64_t inputSelection = 0;

        printf("1. Scalar\n");
        printf("2. Struct\n");
        printf("3. Large Struct (structureInputDescriptor flow)\n");
        printf("4. Checked Scalar\n");
        printf("5. Checked Struct\n");
        printf("6. Assign Callback to Dext\n");
        printf("7. Async Action\n");
        printf("0. Exit\n");
        printf("Select a message type to send: ");
        scanf("%llu", &inputSelection);

        switch (inputSelection)
        {
            case 0: // "Exit"
            {
                runProgram = false;
            } break;

            case 1: // "Scalar"
            {
                /// - Tag: ClientApp_CallUncheckedScalar
                kern_return_t ret = kIOReturnSuccess;

                // IOConnectCallScalarMethod will fail intentionally for any inputCount or outputCount greater than 16
                const uint32_t arraySize = 16;
                const uint64_t input[arraySize] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

                uint32_t outputArraySize = arraySize;
                uint64_t output[arraySize] = {};

                ret = IOConnectCallScalarMethod(connection, MessageType_Scalar, input, arraySize, output, &outputArraySize);
                if (ret != kIOReturnSuccess)
                {
                    printf("IOConnectCallScalarMethod failed with error: 0x%08x.\n", ret);
                    PrintErrorDetails(ret);
                }

                printf("Input of size %u: ", arraySize);
                PrintArray(input, arraySize);
                printf("Output of size %u: ", outputArraySize);
                PrintArray(output, outputArraySize);
            } break;

            case 2: // "Struct"
            {
                kern_return_t ret = kIOReturnSuccess;

                const size_t inputSize = sizeof(DataStruct);
                const DataStruct input = { .foo = 300, .bar = 70000 };

                size_t outputSize = sizeof(DataStruct);
                DataStruct output = { .foo = 0, .bar = 0 };

                ret = IOConnectCallStructMethod(connection, MessageType_Struct, &input, inputSize, &output, &outputSize);
                if (ret != kIOReturnSuccess)
                {
                    printf("IOConnectCallStructMethod failed with error: 0x%08x.\n", ret);
                    PrintErrorDetails(ret);
                }

                printf("Input: \n");
                PrintStruct(&input);
                printf("Output: \n");
                PrintStruct(&output);
            } break;

            case 3: // "Large Struct"
            {
                kern_return_t ret = kIOReturnSuccess;

                const size_t inputSize = sizeof(OversizedDataStruct);
                const OversizedDataStruct input = { };

                size_t outputSize = sizeof(OversizedDataStruct);
                OversizedDataStruct output = { };

                ret = IOConnectCallStructMethod(connection, MessageType_Struct, &input, inputSize, &output, &outputSize);
                if (ret != kIOReturnSuccess)
                {
                    printf("IOConnectCallStructMethod failed with error: 0x%08x.\n", ret);
                    PrintErrorDetails(ret);
                }

                printf("Input: \n");
                PrintStruct(&input);
                printf("Output: \n");
                PrintStruct(&output);
            } break;

            case 4: // "Checked Scalar"
            {
                /// - Tag: ClientApp_CallCheckedScalar
                kern_return_t ret = kIOReturnSuccess;

                // IOConnectCallScalarMethod will fail intentionally for any inputCount or outputCount other than 16, due to our strict checking in the dext
                const uint32_t arraySize = 16;
                const uint64_t input[arraySize] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

                uint32_t outputArraySize = arraySize;
                uint64_t output[arraySize] = {};

                ret = IOConnectCallScalarMethod(connection, MessageType_CheckedScalar, input, arraySize, output, &outputArraySize);
                if (ret != kIOReturnSuccess)
                {
                    printf("IOConnectCallScalarMethod failed with error: 0x%08x.\n", ret);
                    PrintErrorDetails(ret);
                }

                printf("Input of size %u: ", arraySize);
                PrintArray(input, arraySize);
                printf("Output of size %u: ", outputArraySize);
                PrintArray(output, outputArraySize);
            } break;

            case 5: // "Checked Struct"
            {
                kern_return_t ret = kIOReturnSuccess;

                const size_t inputSize = sizeof(DataStruct);
                const DataStruct input = { .foo = 300, .bar = 70000 };

                size_t outputSize = sizeof(DataStruct);
                DataStruct output = { .foo = 0, .bar = 0 };

                ret = IOConnectCallStructMethod(connection, MessageType_CheckedStruct, &input, inputSize, &output, &outputSize);
                if (ret != kIOReturnSuccess)
                {
                    printf("IOConnectCallStructMethod failed with error: 0x%08x.\n", ret);
                    PrintErrorDetails(ret);
                }

                printf("Input: \n");
                PrintStruct(&input);
                printf("Output: \n");
                PrintStruct(&output);
            } break;

            case 6: // "Assign Callback to Dext"
            {
                kern_return_t ret = kIOReturnSuccess;

                const size_t inputSize = sizeof(DataStruct);
                const DataStruct input = { .foo = 300, .bar = 70000 };

                size_t outputSize = sizeof(DataStruct);
                DataStruct output = { .foo = 0, .bar = 0 };

                ret = IOConnectCallAsyncStructMethod(connection, MessageType_RegisterAsyncCallback, machNotificationPort, asyncRef, kIOAsyncCalloutCount, &input, inputSize, &output, &outputSize);
                if (ret != kIOReturnSuccess)
                {
                    printf("IOConnectCallStructMethod failed with error: 0x%08x.\n", ret);
                    PrintErrorDetails(ret);
                }

                printf("Input: \n");
                PrintStruct(&input);
                printf("Output: \n");
                PrintStruct(&output);

                printf("Async result should match output result.\n");
                printf("Assigned callback to dext. Async actions can now be executed.\n");
                printf("Executing run loop while waiting for callback...\n");
                CFRunLoopRun();
                printf("Run loop terminated, returning to standard program flow.\n");
            } break;

            case 7: // "Async Action"
            {
                kern_return_t ret = kIOReturnSuccess;

                const size_t inputSize = sizeof(DataStruct);
                const DataStruct input = { .foo = 300, .bar = 70000 };

                ret = IOConnectCallAsyncStructMethod(connection, MessageType_AsyncRequest, machNotificationPort, asyncRef, kIOAsyncCalloutCount, &input, inputSize, nullptr, nullptr);
                if (ret == kIOReturnNotReady)
                {
                    printf("No callback has been assigned to the dext, so it cannot respond to the async action.\n");
                    printf("Execute the action to assign a callback to the dext before calling this action.\n");
                    break;
                }
                else if (ret != kIOReturnSuccess)
                {
                    printf("IOConnectCallStructMethod failed with error: 0x%08x.\n", ret);
                    PrintErrorDetails(ret);
                }

                printf("Executing run loop while waiting for callback...\n");
                CFRunLoopRun();
                printf("Run loop terminated, returning to standard program flow.\n");
            } break;

            default:
            {
                printf("Invalid input, try again.\n");
            } break;
        }

        printf("\n");
    }

    CFRelease(globalRunLoop);
    return EXIT_SUCCESS;
}
