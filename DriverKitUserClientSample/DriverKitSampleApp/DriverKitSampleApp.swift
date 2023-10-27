/*
See LICENSE folder for this sample’s licensing information.

Abstract:
The SwiftUI scene builder that sets up the driver installation UI.
*/

import SwiftUI

@main
struct DriverKitSampleApp: App {
    var body: some Scene {
        WindowGroup {
            #if os(macOS)
            DriverLoadingView()
            #else
            DriverLoadingView_iOS()
            #endif
        }
    }
}
