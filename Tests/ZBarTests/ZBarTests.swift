import XCTest
@testable import ZBar

final class ZBarTests: XCTestCase {
    func testExample() {
        // This is an example of a functional test case.
        // Use XCTAssert and related functions to verify your tests produce the correct
        // results.
        XCTAssertEqual(ZBar().text, "Hello, World!")
    }

    static var allTests = [
        ("testExample", testExample),
    ]
}
