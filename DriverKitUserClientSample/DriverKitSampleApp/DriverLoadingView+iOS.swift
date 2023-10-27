//
//  DriverLoadingView+iOS.swift
//  DriverKitSampleApp
//
//  Created by Dan on 10/10/23.
//  Copyright © 2023 Apple. All rights reserved.
//

import SwiftUI

struct DriverLoadingView_iOS: View {
    var body: some View {
        VStack(alignment: .center) {
            Text("Driver Manager")
                .padding()
                .font(.title)
            HStack {
                Button(
                    action: {
                        UIApplication.shared.open(URL(string: UIApplication.openSettingsURLString)!)
                    }, label: {
                        Text("Open settings to enable driver")
                    }
                )
            }
        }.frame(width: 500, height: 200, alignment: .center)
    }
}

#Preview {
    DriverLoadingView_iOS()
}
