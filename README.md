# DriverKitUserClientSample

An updated version of Apple's DriverKit sample project "DriverKitUserClientSample" with iPadOS support.

Updates:
- Outdated DriverKit Framework usage updates
- Implemented suport for iPadOS targets
- Implemented WWDC '22 Swift UI approach on adding iPad support to macOS DriverKit driver

Bug Fixes / Enhancements
- Implemented default running of client in terminal vs Xcode
    - A workaround that fixes an issue where you can't send text input to C++ client via command line while running it.

This project was adapted from Apple's WWDC [sample project](https://developer.apple.com/documentation/driverkit/communicating_between_a_driverkit_extension_and_a_client_app) with many, many updates, fixes, and with iPadOS implementation added.

## Usage



### System Extension Utils

- Check if SIP is enabled or disabled (disable if you don't have a DriverKit entitlement from Apple)
    - `csrutil status`

- Enable developer mode:
    - `systemextensionsctl developer on`

- list all currently installed system extensions with status
    - `systemextensionsctl list`

- uninstall a system extension
    - `systemextensionsctl uninstall $(DEV_TEAM_ID) $(com.yourext.bundleid.app)`



## Troubleshooting, Tips, & Credits

### Tips

- Keep in mind that the published Example Projects are super out of date (as of at least Oct 2023).

- Highly recommend that if you follow the example / guide "[Connecting a network driver](https://developer.apple.com/documentation/networkingdriverkit/connecting_a_network_driver?language=objc)" that you do NOT do any of the manual steps related to registering and generating a provisioning profile
  - It WILL work if you do it but you do not need to (the documentation is outdated) 
  - Since Xcode 14, Xcode is able to do this automatically for DriverKit, just as it would for any other type of provisioning profile

- BIG side note / tip for iPadOS drivers:
    - If you plan to embed your DriverKit Driver in an iPadOS app, your bundle naming convention MUST follow this exmple, or it will not compile:
        - Application target bundle id: `com.reversedomainscheme.exampleapp`
        - DriverKit target bundle id: `com.reversedomainscheme.exampleapp.driver` 
        - (notice that the app target's bundle ID is used as a prefix for the driver target's bundle id)
          
    - TL;DR: the driverkit bundle ID is required to use the application bundle id as a prefix. 
            - Note: You don't have to use ".driver" as a suffix, you can use whatever you want to call it, just remember that your driver will be referred to by the driver bundle id in future operations.

### Source project from Apple:
- https://developer.apple.com/documentation/driverkit/communicating_between_a_driverkit_extension_and_a_client_app
- Note: this walkthrough has a LOT of inconsistencies, outdated implementation stuff, and is missing things like iPad support being enabled.

### WWDC References:  
- '21: [What's New in DriverKit](https://developer.apple.com/news/?id=6bhgzi3v)
- '22: [Bring your Driver to iPad with DriverKit](https://developer.apple.com/wwdc22/110373)

### Apple Developer Forum: Issues and Resolutions

- Issue: Unable to run existing application after adding and embedding a DriverKit target
    - I had [this exact issue](https://developer.apple.com/forums/thread/738247), same as posted on the official developer forums
    - Local env: macOS Ventura 13.5.1 (22G90), Xcode 15 (latest, and latest Swift version), macOS app target version: 12.*^
    - Solution: 
        - I tried every single possible thing to fix this and nothing at all worked. It was maddening-- I have NO IDEA what was proventing me from just embedding a new DriverKit driver target into my app-- I tried EVERYTHING possible, including re-installing Xcode completely, with a full purge and re-load, full reload of `xcode-select --install` as well.
        - The only thing that worked create a new application target, embed / link the new DriverKit target to the new application target, and then literally just select every single file from the original project and add them all to the new application target. 
        - After adding all source code and manually updating the application info.plist, entitlements, etc. it was fine, everything worked. I must underline again, everything you can possibly check between these two targets is configured identically, this is just a troll in the machine somewhere.

- Issue: Unable to type text to your client when running it with Xcode
    - [Can't type in input in Xcode 10.1 using C++](https://developer.apple.com/forums/thread/110849?answerId=767941022#767941022)

### Youtube References:
- From apple dev forum on "can't type input into Xcode" (for C++ app)
    - https://www.youtube.com/watch?v=l_h3P9VbsJc


### Credits

- **A sincere thanks to Apple** for publishing all of this content, and for having and maintaing by far the best developer resources and documentation in the industry.
    - It is understandably difficult to keep everything up to date, please don't interpret anything here as unfair criticism.
    - I do, however, think that it would be great if we devs could just do a PR on these example projects to help you all keep them updated. 
    - Please consider publishing and sharing via Github vs the static DocC documentation site. 
        - The example projects are helpful, but they are also sometimes a hinderence when they fall behind the latest changes & improvements.

<br>
// ********************************<br>
// Apple's Original Documentation<br>
// *************************************

(Everything below was the original content of the example project readme)
<br>

# Communicating Between a DriverKit Extension and a Client App

Send and receive different kinds of data securely by validating inputs and asynchronously by storing and using a callback.

[link_kIOReturnBadArgument]:https://developer.apple.com/documentation/driverkit/kioreturnbadargument?language=objc
[link_article_TestingSystemExtensions]:https://developer.apple.com/documentation/driverkit/debugging_and_testing_system_extensions
[link_article_DisablingEnablingSystemIntegrityProtection]:https://developer.apple.com/documentation/security/disabling_and_enabling_system_integrity_protection
[link_article_PreparingYourAppForDistribution]:https://developer.apple.com/documentation/xcode/preparing_your_app_for_distribution
[link_article_RequestingEntitlmentsForDriverKitDevelopment]:https://developer.apple.com/documentation/driverkit/requesting_entitlements_for_driverkit_development
[link_entitlement_system-extension_install]:https://developer.apple.com/documentation/bundleresources/entitlements/com_apple_developer_system-extension_install
[link_entitlement_driverkit]:https://developer.apple.com/documentation/bundleresources/entitlements/com_apple_developer_driverkit?language=objc
[link_entitlement_driverkit_userclient-access]:https://developer.apple.com/documentation/bundleresources/entitlements/com_apple_developer_driverkit_userclient-access
[link_framework_SystemExtensions]:https://developer.apple.com/documentation/systemextensions
[link_article_InstallingSystemExtensionsAndDrivers]:https://developer.apple.com/documentation/systemextensions/installing_system_extensions_and_drivers
[link_ExternalMethod]:https://developer.apple.com/documentation/driverkit/iouserclient/3325619-externalmethod
[link_OSAction]:https://developer.apple.com/documentation/driverkit/osaction
[link_IOUserClientMethodDispatch]:https://developer.apple.com/documentation/driverkit/iouserclientmethoddispatch
[link_IOUserClientMethodArguments]:https://developer.apple.com/documentation/driverkit/iouserclientmethodarguments
[link_IOTimerDispatchSourceTimerOccurred]:https://developer.apple.com/documentation/driverkit/iotimerdispatchsource/3180736-timeroccurred
[link_DriverKit_TYPE]:https://developer.apple.com/documentation/driverkit/type
[link_IOServiceGetMatchingServices]:https://developer.apple.com/documentation/iokit/1514494-ioservicegetmatchingservices?language=objc
[link_IOServiceOpen]:https://developer.apple.com/documentation/iokit/1514515-ioserviceopen?language=objc
[link_IOConnectCallScalarMethod]:https://developer.apple.com/documentation/iokit/1514793-ioconnectcallscalarmethod?language=objc
[link_IOConnectCallStructMethod]:https://developer.apple.com/documentation/iokit/1514274-ioconnectcallstructmethod?language=objc
[link_IOConnectCallAsyncStructMethod]:https://developer.apple.com/documentation/iokit/1514403-ioconnectcallasyncstructmethod?language=objc
[link_IOConnectCallMethod]:https://developer.apple.com/documentation/iokit/1514240-ioconnectcallmethod?language=objc
[link_IOConnectCallAsyncMethod]:https://developer.apple.com/documentation/iokit/1514418-ioconnectcallasyncmethod?language=objc


## Overview

This sample code project shows how a DriverKit extension (dext) receives data from a C++ client process. The sample handles both scalar data and structures, and has two code paths for each type: an insecure version, and a "checked" version that validates traits like data size and input count.

The sample also demonstrates registering and executing a callback function, so the driver can call the client asynchronously.

The sample project contains three targets:
* `DriverKitSampleApp` - A macOS app, written with SwiftUI, to install or update the driver.
* `NullDriver` - The dext itself, which responds to client calls and optionally checks that each call sends the expected data.
* `CppUserClient` - A terminal-based client application that calls the driver.

## Configure the Sample Code Project

To run the sample code project, you first need to build and run `DriverKitSampleApp`, which installs the driver. After that, you can build and run `CppUserClient`, which calls the newly-installed driver.

You can set up the project to build with or without entitlements. To build without entitlements, do the following:

1. Temporarily turn off SIP, as described in the article [Disabling and Enabling System Integrity Protection][link_article_DisablingEnablingSystemIntegrityProtection]. After you've done this, confirm that SIP is disabled with the Terminal command `csrutil status`, and enter dext development mode with `systemextensionsctl developer on`, as described in the article [Debugging and Testing System Extensions][link_article_TestingSystemExtensions].
1. Select the `DriverKitUserClientSample` project and use the "Signing & Capabilities" tab to set the `DriverKitSampleApp` and `CppUserClient` targets to automatically managed code signing.
1. While still in the "Signing & Capabilities" tab, set the `NullDriver` target to manual code signing.
1. In the "Build Settings" tab, change the "Code Signing Identity" value to "Sign to Run Locally" for all three targets.
       
If, instead, you want to build manually with entitlements, do the following:     

1. Choose new bundle identifiers for the app, driver, and client. The bundle identifiers included with the project already have App IDs associated with them, so you need unique identifiers to create your own App IDs. Use a reverse-DNS format for your identifier, as described in [Preparing Your App For Distribution][link_article_PreparingYourAppForDistribution].
1. In `DriverLoadingViewModel.swift`, edit the definition of `dextIdentifier` to use the string you chose for your driver's bundle identifier.
1. In Xcode's Project navigator, choose the project and use the Signing & Capabilities tab to replace the existing bundle identfier for each of the targets with your chosen identifier.
1. Request entitlements, as described in  [Requesting Entitlements for DriverKit Development][link_article_RequestingEntitlmentsForDriverKitDevelopment]. For `DriverKitSampleApp`, you need the [`com.apple.developer.system-extension.install`][link_entitlement_system-extension_install] entitlement. For `NullDriver`, you need the [`com.apple.developer.driverkit`][link_entitlement_driverkit] entitlement. For `CppUserClient` you need  [`com.apple.developer.driverkit.userclient-access`][link_entitlement_driverkit_userclient-access] entitlement, for which you need to need to provide your driver's chosen bundle identifier when you make the request.
1. On `developer.apple.com`, select Account and visit the "Certificates, Identifiers, and Profiles" section. Select "Identifiers" and create new App IDs for `DriverKitSampleApp`, `NullDriver`, and `CppUserClient`. For the Bundle ID, choose "explicit", and use the names you chose in the first step. When you reach the "Capabilities" step, `DriverKitSampleApp` needs the "System Extension" capability, and both `NullDriver` and `CppUserClient` need the "DriverKit" capability.
1. For each of the App IDs you created in the previous step, select "Profiles" to create a new provisioning profile. Start by choosing "macOS App Development," and then "Mac" for the profile type. Next, add any certificates and devices you want to include in the profile. Finally, add the DriverKit entitlement to the profile.
1. Download each profile and add it to Xcode. 
1. In the "Signing & Capabilities" tab, set each target to manual code signing and select the newly-created profile.
1. In the `CppUserClient.entitlements` file, edit the key `com.apple.developer.driverkit.userclient-access`. For the key's value, enter the bundle identifier you chose for your driver, either as a string value or a one-item array of strings.
1. If you want to run `DriverKitSampleApp` directly from Xcode, enter dext development mode with `systemextensionsctl developer on`, as described in the article [Debugging and Testing System Extensions][link_article_TestingSystemExtensions]. Alternately, you can drag the built `DriverKitSample.app` from the build directory into the `/Applications` directory and run it from there.
       
## Use the System Extensions Framework to Install the Driver Extension

The `DriverKitSampleApp` target declares the `NullDriver` as a dependency, so building the app target builds the dext and its installer together. When run, the `DriverKitSampleApp` shows a single window with an "Install Dext" button. 

To install the dext, the app uses the [System Extensions][link_framework_SystemExtensions] framework to install and activate the dext, as described in [Installing System Extensions and Drivers][link_article_InstallingSystemExtensionsAndDrivers].

``` swift
let request = OSSystemExtensionRequest
    .activationRequest(forExtensionWithIdentifier: dextIdentifier,
                       queue: .main)
request.delegate = self
OSSystemExtensionManager.shared.submitRequest(request)
```
[View in Source](x-source-tag://ActivateExtension)

- Note: This call may prompt a "System Extension Blocked" dialog, which explains that `DriverKitSampleApp` tried to install a new system extension. To complete the installation, open System Preferences and go to the Security & Privacy pane. Unlock the pane if necessary, and click "Allow" to complete the installation. To confirm installation of the `NullDriver` extension, run `systemextensionsctl list` in Terminal.

## Call the Driver from the Client

The `CppUserClient` target is a command-line app that connects to and communicates with the dext. When run from Xcode, it accepts input from inside the Xcode Console. The client app, in `main.cpp`, consists of a  `main()` function that creates a connection to the driver, receives keyboard input with `scanf`, and makes calls to the driver. 

To connect to the driver, the client starts by declaring the name of the driver to search for, as well as variables for discovering over services, iterating over them, and establishing the connection to the driver.

``` other
static const char* dextIdentifier = "NullDriver";

kern_return_t ret = kIOReturnSuccess;
io_iterator_t iterator = IO_OBJECT_NULL;
io_service_t service = IO_OBJECT_NULL;
io_connect_t connection = IO_OBJECT_NULL;
```
[View in Source](x-source-tag://ClientApp_Declarations)

The app then uses [`IOServiceGetMatchingServices`][link_IOServiceGetMatchingServices ] to get an iterator of services matching the dext identifier. It iterates over matching services until it finds one that it can connect to with [`IOServiceOpen`][link_IOServiceOpen].

``` other
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
```
[View in Source](x-source-tag://ClientApp_Connect)

As soon as the client populates its `connection` variable, it can accept user commands from the keyboard input. The command menu looks like the following:

```other
1. Scalar
2. Struct
3. Large Struct (structureInputDescriptor flow)
4. Checked Scalar
5. Checked Struct
6. Assign Callback to Dext
7. Async Action
0. Exit
Select a message type to send: 
```

Options 1 through 5 exercise different code paths that send scalar values and structures to the dext. Note that these are synchronous calls that block until the driver returns a result. Options 6 and 7 perform asynchronous operations that allow the driver to call back to the client after a delay.

Each of these options uses the connection in calls to [`IOConnectCallScalarMethod`][link_IOConnectCallScalarMethod], [`IOConnectCallStructMethod`][link_IOConnectCallStructMethod], and [`IOConnectCallAsyncStructMethod`][link_IOConnectCallAsyncStructMethod] (or [`IOConnectCallMethod`][link_IOConnectCallMethod] and [`IOConnectCallAsyncMethod`][link_IOConnectCallAsyncMethod], which this sample doesn't use). For example, the following listing shows the scalar call, option 1 in the menu, which sends an array of 16 `uint64_t` values, and receives a different array back.

``` other
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
```
[View in Source](x-source-tag://ClientApp_CallUncheckedScalar)

The other options are all similar, differing only in which `IOConnect...` function they call and the type of data they send.

## Validate Arguments to Driver Function Calls

The `NullDriver` receives calls from the client in its overridden [`ExternalMethod`][link_ExternalMethod] method. Options 1 through 3 in the client app perform calls that the driver passes unchecked to its [`ExternalMethod`][link_ExternalMethod] implementation. In practice, it's important that a driver validates its inputs before passing them along, to make sure the data is the expected size and contains reasonable values.  `NullDriver` has functions that check scalar and struct calls, which are exercised by options 4 and 5 in the client app.

The "checked" methods in  `NullDriver` use an [`IOUserClientMethodDispatch`][link_IOUserClientMethodDispatch] instance to describe the expected fields of the [`IOUserClientMethodArguments`][link_IOUserClientMethodArguments]. The sample code stores these dispatch instances in an array called `externalMethodChecks`. For example, the dispatch instance for the checked scalar call (option 4 in the client) expects to receive and return 16 scalar values, as seen below:

``` other
[ExternalMethodType_CheckedScalar] =
{
    .function = (IOUserClientMethodFunction) &NullDriver::StaticHandleExternalCheckedScalar,
    .checkCompletionExists = false, // Since this call doesn't use a callback, this value is false and IOUserClientMethodArguments.completion must be 0.
    .checkScalarInputCount = 16,
    .checkStructureInputSize = 0,
    .checkScalarOutputCount = 16,
    .checkStructureOutputSize = 0,
},
```
[View in Source](x-source-tag://ClientMethodDispatch_CheckedScalar)

After fetching the appropriate  [`IOUserClientMethodDispatch`][link_IOUserClientMethodDispatch] instance from the array, the driver passes it in its call to the superclass's [`ExternalMethod`][link_ExternalMethod], along with the method selector and its arguments. If the number of arguments or return values don't match what's in the dispatch instance, the call fails and returns [`kIOReturnBadArgument`][link_kIOReturnBadArgument]. Checking client calls like this prevents a malicious call to the driver from using attack vectors like buffer overruns.

## Prepare the Driver's Instance Variables to Perform Driver-to-Client Callbacks

`CppUserClient` also shows how to communicate from the driver to the client by using a callback function. Option 6 sets up a callback to make an asynchronous call to the client, and then invokes the callback after a short delay to simulate the driver acting on its own. After registering a callback with option 6, calls to option 7 re-invoke the callback.

The `NullDriver` class defines `NullDriver_IVars`, the DriverKit structure that holds the driver's instance variables. `NullDriver_IVars` stores the callback action, as well as a dispatch queue and a timer dispatch source to use when calling back to the client.

``` other
struct NullDriver_IVars {
    OSAction* callbackAction = nullptr;
    IODispatchQueue* dispatchQueue = nullptr;
    IOTimerDispatchSource* dispatchSource = nullptr;
    OSAction* simulatedAsyncDeviceResponseAction = nullptr;
};
```
[View in Source](x-source-tag://Struct_NullDriver_IVars)

`NullDriver` initializes the `dispatchQueue` and `dispatchSource` in its `Start` implementation.

The driver's implementation of `Start` also sets up the `ivars` member `simulatedAsyncDeviceResponsAction`, which the example uses to simulate asynchronous processing that happens on real hardware. This  [`OSAction`][link_OSAction] refers to an asynchronous timer callback to the `SimulatedAsyncEvent` function defined in the `.iig` file:

``` other
virtual void SimulatedAsyncEvent(OSAction* action, uint64_t time) TYPE(IOTimerDispatchSource::TimerOccurred);
```
[View in Source](x-source-tag://IIG_SimulatedAsyncEvent)

This declaration takes the same arguments as the [`IOTimerDispatchSource::TimerOccurred`][link_IOTimerDispatchSourceTimerOccurred] method that that the [`TYPE`][link_DriverKit_TYPE]  macro wraps. By declaring the callback's name as `SimulatedAsyncEvent`, the [`TYPE`][link_DriverKit_TYPE] macro synthesizes `CreateActionSimulatedAsyncEvent`, the function that creates the  [`OSAction`][link_OSAction]. The driver's `Start` implementation can then call this synthesized method to initialize the `simulatedAsyncDeviceResponseAction` member of the `ivars` structure:

``` other
ret = CreateActionSimulatedAsyncEvent(sizeof(DataStruct), &ivars->simulatedAsyncDeviceResponseAction);
if (ret != kIOReturnSuccess)
{
    Log("Start() - Failed to create action for simulated async event with error: 0x%08x.", ret);
    goto Exit;
}
```
[View in Source](x-source-tag://Start_InitializeSimulatedAsyncDeviceResponseAction)

## Retain and Use the Callback to Notify the Client

When the driver is running and it receives a request from the client to register a callback, it calls `NullDriver::RegisterAsyncCallback`. This method stores the completion, if it exists, in the `ivars` structure, like this:

``` other
if (arguments->completion == nullptr)
{
    Log("Got a null completion.");
    return kIOReturnBadArgument;
}

// Save the completion for later.
// If not saved, then it might be freed before the asychronous return.
ivars->callbackAction = arguments->completion;
ivars->callbackAction->retain();
```
[View in Source](x-source-tag://RegisterAsyncCallback_StoreCompletion)

Next, the `NullDriver::RegisterAsyncCallback` method sets up a delayed callback to the client to simulate a hardware delay, allowing it to return quickly, by using the `simulatedAsyncDeviceResponseAction`:

``` other
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
```
[View in Source](x-source-tag://RegisterAsyncCallback_CallCompletion)

After the driver stores the callback, the client app can perform multiple simulated callbacks with option 7. This calls `NullDriver::HandleAsyncRequest`, which is largely similar to the delayed call perfomed in the previous listing.

- Important: The driver must register the callback function before the client makes an asynchronous request, or the kernel may panic.
