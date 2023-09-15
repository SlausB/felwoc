import utest.Assert;
import utest.Async;
import openfl.Assets;
import openfl.utils.IDataInput;

import design.Infos;

class TestSample extends utest.Test {
    var infos : Infos;
    var data : IDataInput;

    @:timeout(5000)
    public function setupClass( async : Async ) {
        data = Assets.getBytes( "assets/xls2xj.bin" );
        infos = new Infos(
            data,
            () -> {
                trace( 'Done.', data.bytesAvailable );
                async.done();
            },
            ( progress : Float ) -> {
                trace( "Progress: ", progress );
            },
        );
    }

    function specData() {
        data;
        data.bytesAvailable == 0;
    }
}
