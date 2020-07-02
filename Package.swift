// swift-tools-version:5.2
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "zbar",
    products: [
        // Products define the executables and libraries produced by a package, and make them visible to other packages.
        .library(
            name: "zbar",
            targets: ["zbar-core", "zbar-ios"]),
    ],
    dependencies: [
        // Dependencies declare other packages that this package depends on.
        // .package(url: /* package url */, from: "1.0.0"),
    ],
    targets: [
        .target(
            name: "zbar-core",
            dependencies: [],
            path: "zbar",
            exclude: [
                "convert.c",
                "jpeg.c",
                "svg.c",
                "processor.c",
                "video.c",
                "window.c",
                "processor",
                "video",
                "window",
                "decoder/pdf417.c",
            ],
            cxxSettings:[
                .headerSearchPath("../include"),
                .headerSearchPath("../iphone/include"),
                .headerSearchPath("../zbar"),
            ]),
        .target(
            name: "zbar-ios",
            dependencies: [],
            path: "iphone",
            exclude: [
                "ZBarCameraSimulator.m",
                "bin",
                "doc",
                "examples",
                "res",
            ],
            cxxSettings:[
                .headerSearchPath("../include"),
                .headerSearchPath("../iphone/include"),
                .headerSearchPath("../zbar"),
            ]),
    ]
)
