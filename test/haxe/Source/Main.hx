import utest.Runner;
import utest.ui.Report;

import TestSample;

class Main {
    public static function main() {
        utest.UTest.run( [ new TestSample() ] );
    }
}
