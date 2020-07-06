// swift-tools-version:5.2
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "ZBar",
    platforms: [
        .iOS(.v8),
    ],
    products: [
        .library(
            name: "ZBarSDK",
            targets: [
                "ZBarSDK",
                ]),
    ],
    dependencies: [
        // Dependencies declare other packages that this package depends on.
        // .package(url: /* package url */, from: "1.0.0"),
    ],
    targets: [
        .target(
            name: "ZBarSDK",
            dependencies: [],
            path: ".",
            exclude: [
                "android",
                "config",
                "doc",
                "examples",
                "gtk",
                "iphone/bin",
                "iphone/doc",
                "iphone/examples",
                "iphone/res",
                "java",
                "perl",
                "plugin",
                "pygtk",
                "python",
                "qt",
                "test",
                "zbar/convert.c",
                "zbar/jpeg.c",
                "zbar/svg.c",
                "zbar/processor.c",
                "zbar/video.c",
                "zbar/window.c",
                "zbar/processor",
                "zbar/video",
                "zbar/window",
                "zbar/decoder/pdf417.c",
                "zbarcam",
                "zbarimg",
                "Package.swift",
            ],
            publicHeadersPath: "SwiftPM/include/",
            cxxSettings:[
                .headerSearchPath("include"),
                .headerSearchPath("iphone/include"),
                .headerSearchPath("zbar"),
            ],
            linkerSettings: [
                .linkedFramework("AVFoundation"),
            ]),
    ]
)
